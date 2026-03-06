#include "PlanModel.h"
#include "Database.h"
#include "ImportOverwriteDialog.h"
#include "ImportOverwriteModel.h"
#include "MainWindow.h"
#include "Plan.h"
#include "PlanWidget.h"
#include "Settings.h"
#include "ImportException.h"
#include <boost/container/flat_set.hpp>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QIODevice>
#include <QMessageBox>
#include <QMimeData>

namespace planner {

static const QString move_mime_poe1{u"application/x-moveplanitem1"};
static const QString move_mime_poe2{u"application/x-moveplanitem2"};

PlanModel::PlanModel(Game game, MainWindow& mw)
    : QAbstractItemModel{&mw}
    , game{game}
    , root{std::make_unique<PlanItem>(nullptr, this, nullptr)}
    , base_plan_name{tr("New Plan")}
    , base_folder_name{tr("New Folder")}
{}

QModelIndex PlanModel::insertPlan(const QModelIndex& dest)
{
    QModelIndex parent_index;
    PlanItem* parent_item;
    int row;
    auto dest_item = internalPtr(dest);
    if (dest_item->isFolder()) {
        parent_index = dest;
        parent_item = dest_item;
        row = dest_item->childCount();
    } else {
        parent_index = dest.parent();
        parent_item = internalPtr(parent_index);
        row = dest.row();
    }

    auto new_plan_name = base_plan_name;
    int i = 0;
    while (!parent_item->checkPlanName(new_plan_name))
        new_plan_name = base_plan_name + QString::number(++i);

    auto id = QUuid::createUuidV7();
    auto& plan = plans.try_emplace(id, id, new_plan_name).first->second;
    plan.game = game;
    plan.league = Settings::currentLeague(game);

    beginInsertRows(parent_index, row, row);
    auto plan_index = parent_item->insertPlan(plan, row, parent_index);
    endInsertRows();

    return plan_index;
}

QModelIndex PlanModel::insertFolder(const QModelIndex& dest)
{
    QModelIndex parent_index = dest.parent();
    PlanItem* parent_item = internalPtr(parent_index);
    int row = dest.isValid() ? dest.row() : parent_item->childCount();

    auto new_folder_name = base_folder_name;
    int i = 0;
    while (!parent_item->checkFolderName(new_folder_name))
        new_folder_name = base_folder_name + QString::number(++i);

    beginInsertRows(parent_index, row, row);
    auto folder_index = parent_item->insertFolder(new_folder_name, row, parent_index);
    endInsertRows();

    return folder_index;
}

QVariant PlanModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    switch (static_cast<PlanItemColumn>(section)) {
    case PlanItemColumn::Name:
        return tr("Name");
    }
    return {};
}

int PlanModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;

    return constInternalPtr(parent)->childCount();
}

int PlanModel::columnCount(const QModelIndex& parent) const
{
    return constInternalPtr(parent)->columnCount();
}

bool PlanModel::moveRows(const QModelIndex& source_idx,
                         int source_row,
                         int count,
                         const QModelIndex& dest_idx,
                         int dest_row)
{
    if (count == 0)
        return false;

    auto source = internalPtr(source_idx);
    auto destination = internalPtr(dest_idx);
    if (source_row < 0 || (source_row + count) > source->childCount() || dest_row < 0
        || dest_row > destination->childCount())
        return false;

    auto first = source->childs.begin() + source_row;
    auto last = first + count;
    auto dest = destination->childs.begin() + dest_row;
    int type;
    if (source != destination)
        type = 1;
    else if (dest < first)
        type = 2;
    else if (dest > last)
        type = 3;
    else
        return false;

    beginMoveRows(source_idx, source_row, source_row + count - 1, dest_idx, dest_row);

    if (type == 1) {
        auto move_first = std::make_move_iterator(first);
        auto move_last = std::make_move_iterator(last);
        auto first_moved = destination->childs.insert(dest, move_first, move_last);
        source->childs.erase(first, last);

        for (auto it = first_moved; it < first_moved + count; ++it)
            it->get()->setParent(destination);
    } else if (type == 2)
        std::rotate(dest, first, last);
    else
        std::rotate(first, last, dest);

    changed_folders.insert(source);
    changed_folders.insert(destination);

    endMoveRows();

    return true;
}

bool PlanModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if (count == 0 || row < 0)
        return false;

    auto item = internalPtr(parent);
    if ((row + count) > item->childCount())
        return false;

    beginRemoveRows(parent, row, row + count - 1);

    auto db = QSqlDatabase::database();
    db.transaction();

    auto query = Database::deletePlan(game);
    auto first = item->childs.begin() + row;
    auto last = first + count;
    for (auto it = first; it < last; ++it)
        (*it)->deleteFromDb(query);
    item->childs.erase(first, first + count);
    changed_folders.insert(item);

    query = Database::savePlan(game);
    saveFolders(query);

    db.commit();
    endRemoveRows();

    return true;
}

bool PlanModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;

    auto item = internalPtr(index);
    auto result = item->setData(index.column(), value, role);
    if (result) {
        if (!item->isFolder())
            emit planRenamed(item->plan());
        saveName(*item);
        emit dataChanged(index, index, {role});
    }

    return result;
}

QStringList PlanModel::mimeTypes() const
{
    QStringList types;
    if (game == Game::Poe1)
        types << move_mime_poe1;
    else
        types << move_mime_poe2;
    return types;
}

QMimeData* PlanModel::mimeData(const QModelIndexList& indexes) const
{
    if (indexes.empty())
        return nullptr;

    auto mime_data = new QMimeData{};

    QByteArray encodedData;
    QDataStream stream{&encodedData, QIODevice::WriteOnly};
    boost::container::flat_set<PlanItem*> ptrs;
    for (const QModelIndex& index : indexes) {
        if (index.isValid())
            ptrs.insert(internalPtr(index));
    }
    for (auto ptr : ptrs)
        stream << std::bit_cast<size_t>(ptr);

    if (game == Game::Poe1)
        mime_data->setData(move_mime_poe1, encodedData);
    else
        mime_data->setData(move_mime_poe2, encodedData);

    return mime_data;
}

bool PlanModel::canDropMimeData(const QMimeData* data,
                                Qt::DropAction /*action*/,
                                int /*row*/,
                                int /*column*/,
                                const QModelIndex& /*parent*/) const
{
    if (game == Game::Poe1) {
        if (!data->hasFormat(move_mime_poe1))
            return false;
    } else if (!data->hasFormat(move_mime_poe2))
        return false;

    return true;
}
bool PlanModel::dropMimeData(
    const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    if (action == Qt::IgnoreAction)
        return true;

    int dest_row;
    if (row != -1)
        dest_row = row;
    else if (parent.isValid())
        dest_row = internalPtr(parent)->childCount();
    else
        dest_row = rowCount({});

    QByteArray encodedData = game == Game::Poe1 ? data->data(move_mime_poe1)
                                                : data->data(move_mime_poe2);
    QDataStream stream{&encodedData, QIODevice::ReadOnly};

    std::vector<PlanItem*> items;
    while (!stream.atEnd()) {
        size_t ptr;
        stream >> ptr;
        items.push_back(std::bit_cast<PlanItem*>(ptr));
    }

    for (auto item : items) {
        auto idx = item->index();
        moveRows(idx.parent(), item->row(), 1, parent, dest_row);
        ++dest_row;
    }
    return true;
}
QVariant PlanModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    const auto* item = static_cast<const PlanItem*>(index.internalPointer());
    return item->data(index.column(), role);
}

Qt::ItemFlags PlanModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return {Qt::ItemIsDropEnabled};

    auto default_flags = QAbstractItemModel::flags(index) | Qt::ItemIsDragEnabled
                         | Qt::ItemIsEditable;
    auto item = constInternalPtr(index);
    if (item->isFolder())
        return default_flags | Qt::ItemIsDropEnabled;

    return default_flags | Qt::ItemNeverHasChildren;
}

QModelIndex PlanModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    if (auto item = internalPtr(parent)->child(row))
        return createIndex(row, column, item);

    return {};
}

QModelIndex PlanModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return {};

    auto item = static_cast<PlanItem*>(index.internalPointer());
    PlanItem* parent_item = item->parent();

    return parent_item != root.get() ? createIndex(parent_item->row(), 0, parent_item)
                                     : QModelIndex{};
}

PlanItem* PlanModel::internalPtr(const QModelIndex& index) const
{
    return index.isValid() ? static_cast<PlanItem*>(index.internalPointer()) : root.get();
}

const PlanItem* PlanModel::constInternalPtr(const QModelIndex& index) const
{
    return static_cast<const PlanItem*>(internalPtr(index));
}

bool PlanModel::readDatabase()
{
    auto select = Database::selectPlan(game);
    select.addBindValue(QUuid{}.toString());

    if (!select.exec())
        return false;
    if (!select.next())
        return true;

    beginResetModel();
    root = std::make_unique<PlanItem>(QUuid{}, select, this, nullptr);
    endResetModel();

    return true;
}

void PlanModel::importItem(const QJsonObject& export_o)
{
    auto plan_v = export_o["plan"];

    std::unique_ptr<PlanItem> import_root;
    bool is_folder = plan_v.isUndefined();
    try {
        if (is_folder)
            import_root = std::make_unique<PlanItem>(is_folder,
                                                     export_o["folder"].toObject(),
                                                     this,
                                                     nullptr);
        else
            import_root = std::make_unique<PlanItem>(is_folder, plan_v.toObject(), this, nullptr);

        auto trade_cache = mw()->tradeCache(game);
        auto requests_a = export_o["trade_requests"].toArray();
        auto import_requests = trade_cache->requestsFromJson(requests_a);

        ImportOverwriteModel overwrite_model{import_plans, plans};
        if (!overwrite_model.plans_for_overwrite.empty()) {
            ImportOverwriteDialog dialog{overwrite_model, mw()};
            auto res = dialog.exec();
            switch (res) {
            case QDialogButtonBox::NoRole:
                trade_cache->mergeImportRequests(std::move(import_requests));
                for (auto& p : overwrite_model.plans_for_overwrite) {
                    auto node = import_plans.extract(p.first);
                    node.key() = node.mapped().changeId();
                    import_plans.insert(std::move(node));
                }
                break;
            case QDialogButtonBox::YesRole:
                trade_cache->mergeImportRequests(std::move(import_requests));
                for (auto& p : overwrite_model.plans_for_overwrite) {
                    if (!p.second.second) {
                        auto node = import_plans.extract(p.first);
                        node.key() = node.mapped().changeId();
                        import_plans.insert(std::move(node));
                    } else {
                        auto old_plan = p.second.first;
                        auto old_item = old_plan->item();
                        auto parent = old_item->parent();

                        auto import_it = import_plans.find(p.first);
                        auto import_item = import_it->second.item();

                        auto import_parent = import_item->parent();
                        auto new_item = parent->replacePlan(old_item->row(),
                                                            std::move(import_it->second));

                        emit planUpdated(new_item->plan(), old_plan);

                        import_plans.erase(import_it);
                        import_item->plan_ = nullptr;
                        if (import_parent)
                            import_parent->childs.erase(import_parent->childs.begin()
                                                        + import_item->row());
                    }
                }
                break;
            case QDialog::Rejected:
            default:
                import_plans.clear();
                return;
            }
        } else
            trade_cache->mergeImportRequests(std::move(import_requests));

        if (!import_plans.empty()) {
            auto root_name = import_root->name();
            if (!root_name.startsWith("(I) "))
                root_name.prepend("(I) ");
            import_root->setName(root_name);

            import_root->setItemChanged(true);
            plans.merge(std::move(import_plans));
            beginInsertRows({}, root->childCount(), root->childCount());
            root->appendChild(std::move(import_root));
            changed_folders.insert(root.get());
            endInsertRows();
        }
    } catch (ImportException& e) {
        QMessageBox msg;
        msg.setWindowTitle(tr("Import Failed"));
        switch (e.type) {
        case ImportError::InvalidPlanId:
            msg.setText(tr("Failed to import plans."));
            break;
        case ImportError::InvalidTradeRequest:
            msg.setText(tr("Failed to import trade searches."));
            break;
        }
        msg.exec();
    }

    import_plans.clear();
}

void PlanModel::exportItem(const QModelIndex& index, bool to_clipboard) const
{
    auto item = internalPtr(index);

    auto trade_cache = mw()->tradeCache(game);

    if (auto plan = mw()->planWidget()->plan(); plan && plan->game == game)
        mw()->planWidget()->setDescriptions(nullptr);

    QJsonObject export_o;
    export_o["game"] = gameStr(game);
    if (item->isFolder())
        export_o["folder"] = item->exportJson(*mw()->exchangeCache(game), *trade_cache);
    else
        export_o["plan"] = item->exportJson(*mw()->exchangeCache(game), *trade_cache);
    export_o["trade_requests"] = trade_cache->exportRequests();

    QJsonDocument json;
    json.setObject(export_o);
    if (to_clipboard)
        qApp->clipboard()->setText(json.toJson(QJsonDocument::Compact));
    else {
        auto name = item->parent() ? item->name() : gameStr(game);
        auto filename = QFileDialog::getSaveFileName(mw(),
                                                     tr("Export"),
                                                     name + ".json",
                                                     tr("JSON file (*.json)"));
        if (filename.isEmpty())
            return;

        QFile file{filename};
        if (file.open(QFile::WriteOnly))
            file.write(json.toJson(QJsonDocument::Compact));
        else {
            QMessageBox msg;
            msg.setWindowTitle(tr("Export Failed"));
            msg.setText(tr("Failed to write file \"%1\".").arg(filename));
            msg.exec();
        }
    }
}

void PlanModel::savePlan(const QModelIndex& index)
{
    auto item = internalPtr(index);

    savePlan(item);
}

void PlanModel::savePlan(PlanItem* item)
{
    bool is_changed = item && !item->isFolder() && item->plan()->is_changed;

    if (!changed_folders.empty()) {
        auto db = QSqlDatabase::database();
        db.transaction();

        auto save_query = Database::savePlan(game);
        if (is_changed) {
            savePlanItem(item, save_query);
            changed_plans.erase(item);
        }
        saveFolders(save_query);

        db.commit();
    } else if (is_changed) {
        auto save_query = Database::savePlan(game);
        savePlanItem(item, save_query);
        changed_plans.erase(item);
    }
}

void PlanModel::saveAllPlans()
{
    if (changed_plans.empty())
        return;

    auto db = QSqlDatabase::database();
    db.transaction();

    auto save_query = Database::savePlan(game);
    for (auto& item : changed_plans)
        savePlanItem(item.first, save_query);
    changed_plans.clear();

    saveFolders(save_query);

    db.commit();
}

void PlanModel::restorePlan(const QModelIndex& index)
{
    auto item = internalPtr(index);
    if (item->isFolder() || !changed_plans.contains(item))
        return;

    saveFoldersTransaction();

    auto old_plan = item->plan_;
    auto parent_item = item->parent();
    if (auto new_item = parent_item->restoreChild(index.row())) {
        changed_plans.erase(item);

        emit dataChanged(index, index, {Qt::DisplayRole});

        emit planUpdated(new_item->plan_, old_plan);
    }
}

void PlanModel::duplicateItem(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    auto parent = index.parent();
    auto parent_item = internalPtr(parent);
    beginInsertRows(parent, index.row() + 1, index.row() + 1);
    parent_item->duplicateChild(index.row());
    endInsertRows();
}

bool PlanModel::isNewPlan(const QModelIndex& index) const
{
    auto item = internalPtr(index);
    if (item->isFolder())
        return false;

    auto it = changed_plans.find(item);
    return it != changed_plans.end() && it->second;
}

void PlanModel::setPlanChanged(PlanItem* item)
{
    changed_plans.insert({item, false});
    auto idx = item->index();
    emit dataChanged(idx, idx, {Qt::DisplayRole});
}

void PlanModel::saveName(PlanItem& item)
{
    auto rename_query = Database::renamePlan(game);
    rename_query.addBindValue(item.name());
    rename_query.addBindValue(item.id);
    rename_query.exec();
}

void PlanModel::saveFoldersTransaction()
{
    if (changed_folders.empty())
        return;

    auto db = QSqlDatabase::database();
    db.transaction();

    auto save_query = Database::savePlan(game);
    saveFolders(save_query);

    db.commit();
}

void PlanModel::saveFolders(QSqlQuery& save_query)
{
    for (auto item : changed_folders)
        savePlanItem(item, save_query);
    changed_folders.clear();
}

void PlanModel::savePlanItem(PlanItem* item, QSqlQuery& save_query)
{
    save_query.addBindValue(item->id.toString());
    save_query.addBindValue(item->name());
    save_query.addBindValue(item->isFolder());

    if (item->plan())
        mw()->planWidget()->setDescriptions(item->plan());
    QJsonDocument json{item->saveJson()};
    save_query.addBindValue(json.toJson(QJsonDocument::Compact));
    save_query.exec();
    if (item->plan()) {
        item->plan()->is_changed = false;
        auto idx = item->index();
        emit dataChanged(idx, idx, {Qt::DisplayRole});
    }
}

MainWindow* PlanModel::mw() const
{
    return static_cast<MainWindow*>(QObject::parent());
}

} // namespace planner
