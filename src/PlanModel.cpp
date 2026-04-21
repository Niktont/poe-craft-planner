#include "PlanModel.h"
#include "AppState.h"
#include "Database.h"
#include "ImportException.h"
#include "ImportOverwriteDialog.h"
#include "ImportOverwriteModel.h"
#include "Plan.h"
#include "PlanItem.h"
#include "PlanSearchModel.h"
#include "PlanTreeView.h"
#include "Settings.h"
#include "TradeRequestCache.h"
#include "UpdateCostDialog.h"
#include <boost/container/flat_set.hpp>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMimeData>

namespace planner {

const QString PlanModel::move_mime_poe1{u"application/x-moveplanitem1"};
const QString PlanModel::move_mime_poe2{u"application/x-moveplanitem2"};

const QString& moveMime(Game game)
{
    return game == Game::Poe1 ? PlanModel::move_mime_poe1 : PlanModel::move_mime_poe2;
};

PlanModel::PlanModel(Game game, QObject* parent)
    : QAbstractItemModel{parent}
    , game{game}
    , search_model{new PlanSearchModel{*this}}
    , root{std::make_unique<PlanItem>(nullptr, *this, nullptr)}
    , base_plan_name{tr("New plan")}
    , base_folder_name{tr("New folder")}
{}

PlanModel::~PlanModel() noexcept
{
    saveFoldersTransaction();
}

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

    search_model->insertPlan(plan);

    return plan_index;
}

QModelIndex PlanModel::insertFolder(const QModelIndex& dest)
{
    auto parent_index = dest.parent();
    auto parent_item = internalPtr(parent_index);
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

QVariant PlanModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Vertical)
        return {};

    switch (static_cast<PlanItemColumn>(section)) {
    case PlanItemColumn::Name:
        return tr("Name");
    case PlanItemColumn::Cost:
        return tr("Cost");
    }
    return {};
}

int PlanModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;

    return constInternalPtr(parent)->childCount();
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
    if (source != destination) {
        if (destination->parent() == source) {
            auto row = destination->row();
            if (source_row <= row && row < source_row + count)
                return false;
        }
        type = 1;
    } else if (dest < first)
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

        for (auto it = first_moved; it < first_moved + count; ++it) {
            (*it)->parent_ = destination;
            if ((*it)->plan())
                search_model->updatePath(*(*it)->plan());
        }
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

    auto parent_item = internalPtr(parent);
    if ((row + count) > parent_item->childCount())
        return false;

    beginRemoveRows(parent, row, row + count - 1);

    auto db = QSqlDatabase::database();
    db.transaction();

    auto query = Database::deletePlan(game);
    auto first = parent_item->childs.begin() + row;
    auto last = first + count;
    for (auto it = first; it < last; ++it)
        (*it)->remove(query);
    parent_item->childs.erase(first, last);
    changed_folders.insert(parent_item);

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
    bool changed = item->setData(index.column(), value, role);
    if (changed) {
        if (item->plan())
            emit planRenamed(*item->plan());
        saveName(*item);
        emit dataChanged(index, index, {Qt::DisplayRole});
    }

    return changed;
}

QStringList PlanModel::mimeTypes() const
{
    static QStringList types{moveMime(game)};
    return types;
}

QMimeData* PlanModel::mimeData(const QModelIndexList& indexes) const
{
    if (indexes.empty())
        return nullptr;

    auto mime_data = new QMimeData{};

    QByteArray encodedData;
    QDataStream stream{&encodedData, QDataStream::WriteOnly};
    boost::container::flat_set<PlanItem*> items;
    for (const QModelIndex& index : indexes)
        items.insert(internalPtr(index));

    for (auto ptr : items)
        stream << std::bit_cast<size_t>(ptr);

    mime_data->setData(moveMime(game), encodedData);

    if (QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier))
        emit currentNeedsReselecting(game);

    return mime_data;
}

bool PlanModel::canDropMimeData(const QMimeData* data,
                                Qt::DropAction /*action*/,
                                int /*row*/,
                                int /*column*/,
                                const QModelIndex& /*parent*/) const
{
    if (data->hasFormat(moveMime(game)))
        return true;

    return false;
}

bool PlanModel::dropMimeData(
    const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    if (action == Qt::IgnoreAction)
        return true;

    auto parent_item = internalPtr(parent);
    int dest_row;
    if (row != -1)
        dest_row = row;
    else if (parent.isValid())
        dest_row = parent_item->childCount();
    else
        dest_row = rowCount({});

    auto items = decodePlanItemsMime(game, *data);
    std::erase(items, parent_item);
    if (items.empty())
        return false;

    for (auto item : items) {
        auto idx = item->index();
        moveRows(idx.parent(), item->row(), 1, parent, dest_row);
        ++dest_row;
    }
    return true;
}

std::vector<PlanItem*> PlanModel::decodePlanItemsMime(Game game, const QMimeData& data)
{
    auto encodedData = data.data(moveMime(game));
    QDataStream stream{&encodedData, QDataStream::ReadOnly};

    std::vector<PlanItem*> items;
    while (!stream.atEnd()) {
        size_t ptr;
        stream >> ptr;
        items.push_back(std::bit_cast<PlanItem*>(ptr));
    }
    return items;
}

std::vector<Plan*> PlanModel::decodeMimeToPlans(Game game, const QMimeData& data)
{
    auto items = decodePlanItemsMime(game, data);

    std::vector<Plan*> plans;
    for (auto plan_item : items) {
        if (plan_item->isFolder()) {
            for (auto& child : plan_item->childs) {
                if (child->plan())
                    plans.push_back(child->plan());
            }
        } else
            plans.push_back(plan_item->plan());
    }
    return plans;
}

QVariant PlanModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    return constInternalPtr(index)->data(index.column(), role);
}

Qt::ItemFlags PlanModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return {Qt::ItemIsDropEnabled};

    auto default_flags = QAbstractItemModel::flags(index) | Qt::ItemIsDragEnabled;
    auto item = constInternalPtr(index);

    auto col = static_cast<PlanItemColumn>(index.column());
    if (col == PlanItemColumn::Name)
        default_flags |= Qt::ItemIsEditable;

    if (item->isFolder())
        return default_flags | Qt::ItemIsDropEnabled;

    return default_flags | Qt::ItemNeverHasChildren;
}

QModelIndex PlanModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    return createIndex(row, column, &internalPtr(parent)->child(row));
}

QModelIndex PlanModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return {};

    auto item = static_cast<PlanItem*>(index.internalPointer());
    auto parent_item = item->parent();

    return parent_item != root.get() ? createIndex(parent_item->row(), 0, parent_item)
                                     : QModelIndex{};
}

PlanItem* PlanModel::internalPtr(const QModelIndex& index) const
{
    return index.isValid() ? static_cast<PlanItem*>(index.internalPointer()) : root.get();
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
    root = std::make_unique<PlanItem>(QUuid{}, select, *this, nullptr);
    endResetModel();

    search_model->reset();

    return true;
}

bool PlanModel::importItem(const QJsonObject& export_o, QWidget* dialog_parent)
{
    std::unique_ptr<PlanItem> import_root;
    TradeRequestCache::Cache import_requests;
    try {
        auto plan_v = export_o["plan"];
        bool is_folder = plan_v.isUndefined();
        if (is_folder)
            import_root = std::make_unique<PlanItem>(is_folder,
                                                     export_o["folder"].toObject(),
                                                     *this,
                                                     nullptr);
        else
            import_root = std::make_unique<PlanItem>(is_folder, plan_v.toObject(), *this, nullptr);

        import_requests = TradeRequestCache::requestsFromJson(export_o["trade_requests"].toArray());
    } catch (ImportException& e) {
        auto msg = new QMessageBox{dialog_parent};
        msg->setAttribute(Qt::WA_DeleteOnClose);
        msg->setWindowTitle(tr("Import Failed"));
        switch (e.type) {
        case ImportError::InvalidPlanId:
            msg->setText(tr("Failed to import plans."));
            break;
        case ImportError::InvalidTradeRequest:
            msg->setText(tr("Failed to import trade searches."));
            break;
        }
        msg->open();
        import_plans.clear();
        return false;
    }
    if (!handleOverwrite(dialog_parent)) {
        import_plans.clear();
        return false;
    }

    AppState::tradeCache(game)->mergeImportRequests(std::move(import_requests));

    if (!import_plans.empty()) {
        if (Settings::get<Settings::import_add_prefix>()) {
            auto root_name = import_root->name();
            if (!root_name.startsWith("(I) "))
                root_name.prepend("(I) ");
            import_root->setName(root_name);
        }
        for (auto& [id, plan] : import_plans)
            search_model->insertPlan(plan);

        import_root->setItemChanged(true);
        plans.merge(std::move(import_plans));
        beginInsertRows({}, root->childCount(), root->childCount());
        root->appendChild(std::move(import_root));
        endInsertRows();
    }

    import_plans.clear();
    return true;
}

QString PlanModel::exportFileName(const QModelIndex& index) const
{
    auto name = internalPtr(index)->name();
    if (name.isEmpty())
        name = game == Game::Poe1 ? tr("PoE 1 Plans") : tr("PoE 2 Plans");

    return name;
}

QJsonDocument PlanModel::exportItem(const QModelIndex& index,
                                    bool without_deps,
                                    bool without_requests) const
{
    auto item = internalPtr(index);

    emit descriptionsNeeded(game, nullptr);

    QJsonObject export_o;
    export_o["game"] = gameStr(game);

    auto trade_cache = AppState::tradeCache(game);
    trade_cache->include_requests_for_export = !without_requests
                                               && Settings::get<settings::export_with_requests>();

    QJsonObject item_o;
    std::vector<QUuid> dependencies;
    if (!without_deps && Settings::get<settings::export_with_dependencies>())
        item_o = item->exportJson(*AppState::exchangeCache(game), *trade_cache, &dependencies);
    else
        item_o = item->exportJson(*AppState::exchangeCache(game), *trade_cache, nullptr);

    if (dependencies.empty() || !gatherDependencies(export_o, item_o, dependencies)) {
        if (item->isFolder())
            export_o["folder"] = item_o;
        else
            export_o["plan"] = item_o;
    }

    if (trade_cache->include_requests_for_export)
        export_o["trade_requests"] = trade_cache->exportRequests();

    QJsonDocument json;
    json.setObject(export_o);

    return json;
}
bool PlanModel::handleOverwrite(QWidget* dialog_parent)
{
    ImportOverwriteModel overwrite_model{import_plans, plans};
    if (overwrite_model.plans_for_overwrite.empty())
        return true;

    ImportOverwriteDialog dialog{overwrite_model, dialog_parent};
    auto res = dialog.exec();

    boost::container::flat_map<QUuid, QUuid> changed_ids;
    switch (res) {
    case QDialogButtonBox::NoRole:
        for (auto& p : overwrite_model.plans_for_overwrite) {
            auto node = import_plans.extract(p.first);
            node.key() = node.mapped().changeId();
            changed_ids.emplace(p.first, node.key());

            import_plans.insert(std::move(node));
        }
        for (auto& p : import_plans) {
            for (auto& step : p.second.steps)
                step.updatePlanIds(changed_ids);
        }
        return true;
    case QDialogButtonBox::YesRole:
        for (auto& p : overwrite_model.plans_for_overwrite) {
            if (p.second.second)
                continue;

            auto node = import_plans.extract(p.first);
            node.key() = node.mapped().changeId();
            changed_ids.emplace(p.first, node.key());

            import_plans.insert(std::move(node));
        }
        if (!changed_ids.empty()) {
            for (auto& p : import_plans) {
                for (auto& step : p.second.steps)
                    step.updatePlanIds(changed_ids);
            }
        }

        for (auto& p : overwrite_model.plans_for_overwrite) {
            if (!p.second.second)
                continue;

            auto plan_for_overwrite = p.second.first;
            auto item = plan_for_overwrite->item();
            auto idx = item->index();
            auto parent = item->parent();

            auto import_it = import_plans.find(p.first);
            auto import_item = import_it->second.item();

            if (Settings::get<Settings::import_overwrite_names>()) {
                parent->replacePlan(item->row(), std::move(import_it->second));
                saveName(*item);
                search_model->updatePath(*plan_for_overwrite);
                emit dataChanged(idx, idx, {Qt::DisplayRole});
            } else {
                import_it->second.name = plan_for_overwrite->name;
                parent->replacePlan(item->row(), std::move(import_it->second));
            }
            auto cost_idx = idx.siblingAtColumn(static_cast<int>(PlanItemColumn::Cost));
            emit dataChanged(cost_idx, cost_idx, {Qt::DisplayRole, Qt::DecorationRole});
            emit planUpdated(*item->plan());

            if (auto import_parent = import_item->parent())
                import_parent->childs.erase(import_parent->childs.begin() + import_item->row());
            import_plans.erase(import_it);
        }
        return true;
    case QDialog::Rejected:
    default:
        return false;
    }
}

void PlanModel::savePlan(const QModelIndex& index)
{
    auto item = internalPtr(index);

    savePlan(*item);
}

void PlanModel::savePlan(const PlanItem& item)
{
    bool is_changed = !item.isFolder() && item.plan()->is_changed;

    if (!changed_folders.empty()) {
        auto db = QSqlDatabase::database();
        db.transaction();

        auto save_query = Database::savePlan(game);
        if (is_changed) {
            savePlanItem(item, save_query);
            changed_plans.erase(&item);
        }
        saveFolders(save_query);

        db.commit();

        if (is_changed)
            emit planSaved(game, item.plan());
    } else if (is_changed) {
        auto save_query = Database::savePlan(game);
        savePlanItem(item, save_query);
        changed_plans.erase(&item);
        emit planSaved(game, item.plan());
    }
}

void PlanModel::saveAllPlans()
{
    if (changed_plans.empty())
        return;

    auto single_plan = changed_plans.size() == 1 ? changed_plans.begin()->first : nullptr;
    auto db = QSqlDatabase::database();
    db.transaction();

    auto save_query = Database::savePlan(game);
    for (auto& item : changed_plans)
        savePlanItem(*item.first, save_query);
    changed_plans.clear();

    saveFolders(save_query);

    db.commit();

    if (single_plan)
        emit planSaved(game, single_plan->plan());
    else
        emit planSaved(game, nullptr);
}

void PlanModel::restorePlan(const QModelIndex& index)
{
    auto item = internalPtr(index);
    if (item->isFolder() || !changed_plans.contains(item))
        return;

    saveFoldersTransaction();

    auto parent_item = item->parent();
    if (auto new_item = parent_item->restorePlan(index.row())) {
        search_model->updatePath(*new_item->plan());
        emit dataChanged(index,
                         index.siblingAtColumn(static_cast<int>(PlanItemColumn::Cost)),
                         {Qt::DisplayRole, Qt::DecorationRole});

        emit planUpdated(*new_item->plan());
    }
}

QModelIndex PlanModel::duplicateItem(const QModelIndex& idx)
{
    if (!idx.isValid())
        return {};

    auto parent = idx.parent();
    auto parent_item = internalPtr(parent);
    return insertCopy(parent, idx.row() + 1, parent_item->child(idx.row()));
}

void PlanModel::copyItem(const QModelIndex& idx) const
{
    if (!idx.isValid())
        return;

    auto item = internalPtr(idx);
    item_copy_state = item;
}

QModelIndex PlanModel::pasteItem(const QModelIndex& idx)
{
    if (!item_copy_state)
        return {};

    auto copy_row = idx.isValid() ? idx.row() + 1 : root->childCount();
    auto parent = idx.parent();
    return insertCopy(parent, copy_row, *item_copy_state);
}

void PlanModel::updateCosts(const QModelIndex& index, bool send_requests)
{
    AppState::state.update_cost_dialog->updatePlanItem(*internalPtr(index), send_requests);
}

bool PlanModel::isNewPlan(const QModelIndex& index) const
{
    auto item = internalPtr(index);
    if (item->isFolder())
        return false;

    auto it = changed_plans.find(item);
    return it != changed_plans.end() && it->second;
}

bool PlanModel::canRestorePlan(const QModelIndex& index) const
{
    auto item = internalPtr(index);
    if (item->isFolder())
        return false;

    auto it = changed_plans.find(item);
    return it != changed_plans.end() && !it->second;
}

void PlanModel::updateCost(const QModelIndex& index)
{
    auto cost_idx = index.siblingAtColumn(static_cast<int>(PlanItemColumn::Cost));
    emit dataChanged(cost_idx, cost_idx, {Qt::DisplayRole, Qt::DecorationRole});
}

void PlanModel::setPlanChanged(const PlanItem& item)
{
    changed_plans.emplace(&item, false);
    auto idx = item.index();
    emit dataChanged(idx, idx, {Qt::DisplayRole});
}

void PlanModel::saveName(const PlanItem& item) const
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
        savePlanItem(*item, save_query);
    changed_folders.clear();
}

void PlanModel::savePlanItem(const PlanItem& item, QSqlQuery& save_query)
{
    save_query.addBindValue(item.id.toString());
    save_query.addBindValue(item.name());
    save_query.addBindValue(item.isFolder());

    if (item.plan())
        emit descriptionsNeeded(game, item.plan());

    save_query.addBindValue(QJsonDocument{item.saveJson()}.toJson(QJsonDocument::Compact));
    save_query.exec();
    if (item.plan()) {
        item.plan()->is_changed = false;
        auto idx = item.index();
        emit dataChanged(idx, idx, {Qt::DisplayRole});
    }
}

bool PlanModel::gatherDependencies(QJsonObject& export_o,
                                   QJsonObject& item_o,
                                   std::vector<QUuid>& dependencies) const
{
    size_t export_end = dependencies.size();
    for (size_t i = 0; i < export_end; ++i) {
        if (auto it = plans.find(dependencies[i]); it != plans.end())
            it->second.gatherDependencies(dependencies);
    }
    if (export_end == dependencies.size())
        return false;

    auto exchange_cache = AppState::exchangeCache(game);
    auto trade_cache = AppState::tradeCache(game);

    QJsonArray dependencies_a;
    for (size_t i = export_end; i < dependencies.size(); ++i) {
        if (auto it = plans.find(dependencies[i]); it != plans.end()) {
            dependencies_a.push_back(it->second.exportJson(*exchange_cache, *trade_cache));
            it->second.gatherDependencies(dependencies);
        }
    }
    if (dependencies_a.empty())
        return false;

    QJsonObject dependencies_o;
    dependencies_o["is_folder"] = true;
    dependencies_o["name"] = "Dependencies";
    dependencies_o["childs"] = dependencies_a;

    if (item_o["is_folder"].toBool()) {
        auto childs_a = item_o.take("childs").toArray();
        childs_a.append(dependencies_o);
        item_o["childs"] = childs_a;

        export_o["folder"] = item_o;
    } else {
        QJsonArray childs_a;
        childs_a.push_back(item_o);
        childs_a.push_back(dependencies_o);

        QJsonObject export_folder;
        export_folder["is_folder"] = true;
        export_folder["name"] = item_o["name"];
        export_folder["childs"] = childs_a;

        export_o["folder"] = export_folder;
    }

    return true;
}

QModelIndex PlanModel::insertCopy(const QModelIndex& parent, int row, const PlanItem& item)
{
    auto parent_item = internalPtr(parent);
    beginInsertRows(parent, row, row);
    parent_item->insertCopy(row, item);
    endInsertRows();

    return index(row, 0, parent);
}

} // namespace planner
