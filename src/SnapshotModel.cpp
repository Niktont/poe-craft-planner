#include "SnapshotModel.h"
#include "Database.h"
#include "ExchangeRequestCache.h"
#include "TradeRequestCache.h"
#include <QAbstractProxyModel>
#include <QCompleter>

namespace planner {

SnapshotModel::SnapshotModel(ExchangeRequestCache& exchange_cache,
                             TradeRequestCache& trade_cache,
                             QObject* parent)
    : QAbstractTableModel{parent}
    , game{exchange_cache.game}
    , completer{new QCompleter{{}, this}}
    , exchange_cache{exchange_cache}
    , trade_cache{trade_cache}
{
    completer->setCompletionColumn(static_cast<int>(SnapshotColumn::Name));
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCompletionRole(Qt::DisplayRole);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setModel(this);

    connect(completer,
            qOverload<const QModelIndex&>(&QCompleter::activated),
            this,
            [this](const QModelIndex& idx) {
                auto proxy = qobject_cast<QAbstractProxyModel*>(completer->completionModel());
                setCurrent(proxy->mapToSource(idx));
            });
}

bool SnapshotModel::readDatabase()
{
    auto query = Database::selectSnapshots(game);
    if (!query.exec())
        return false;

    beginResetModel();
    while (query.next()) {
        auto id = query.value(0).toUuid();
        snapshots.try_emplace(snapshots.end(),
                              id,
                              id,
                              query.value(1).toString(),
                              query.value(2).toString());
    }
    endResetModel();

    return true;
}

QString SnapshotModel::currentName() const
{
    if (current) {
        if (auto it = snapshots.find(current->id); it != snapshots.end())
            return it->second.name;
    }
    return {};
}

void SnapshotModel::setCurrent(const QUuid& id)
{
    if ((current && current->id == id) || !snapshots.contains(id))
        return;

    if (auto cache_it = std::ranges::find(cache, id, &Snapshot::id); cache_it != cache.end())
        current = &(*cache_it);
    else {
        auto query = Database::selectSnapshotCosts(game, id);
        if (!query.exec() || !query.next())
            return;

        if (cache.size() >= 10)
            cache.pop_front();

        current = &cache.emplace_back();
        current->id = id;
        current->exchange = ExchangeCostData::fromJson(QJsonDocument::fromJson(
                                                           query.value(0).toByteArray())
                                                           .object(),
                                                       exchange_cache);
        current->trade
            = TradeCostData::fromJson(QJsonDocument::fromJson(query.value(1).toByteArray()).object(),
                                      exchange_cache);
    }

    exchange_cache.setSnapshot(current);
    trade_cache.setSnapshot(current);

    emit currentChanged(game, current);
}

void SnapshotModel::setCurrent(const QModelIndex& index)
{
    if (!index.isValid() || index.row() >= std::ssize(snapshots))
        return;

    auto& id = snapshots.nth(index.row())->first;
    setCurrent(id);
}

void SnapshotModel::clearCurrent()
{
    if (!current)
        return;

    current = nullptr;
    exchange_cache.setSnapshot(current);
    trade_cache.setSnapshot(current);

    emit currentChanged(game, current);
}

QVariant SnapshotModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation == Qt::Vertical)
        return {};

    auto col = static_cast<SnapshotColumn>(section);
    switch (col) {
    case SnapshotColumn::Name:
        return tr("Name");
    case SnapshotColumn::Date:
        return tr("Date");
    case SnapshotColumn::League:
        return tr("League");
    }

    return {};
}

int SnapshotModel::rowCount(const QModelIndex& parent) const
{
    return !parent.isValid() ? snapshots.size() : 0;
}

int SnapshotModel::columnCount(const QModelIndex& parent) const
{
    return !parent.isValid() ? static_cast<int>(SnapshotColumn::last) + 1 : 0;
}

QVariant SnapshotModel::data(const QModelIndex& index, int role) const
{
    auto snapshot = snapshots.nth(index.row());
    auto col = static_cast<SnapshotColumn>(index.column());
    switch (col) {
    case SnapshotColumn::Name:
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return snapshot->second.name;
        }
        return {};
    case SnapshotColumn::Date:
        switch (role) {
        case Qt::DisplayRole:
            return snapshot->second.date.date().toString(Qt::DateFormat::ISODate);
        }
        return {};
    case SnapshotColumn::League:
        switch (role) {
        case Qt::DisplayRole:
            return snapshot->second.league;
        }
        return {};
    }
    return {};
}

bool SnapshotModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    auto col = static_cast<SnapshotColumn>(index.column());
    if (col != SnapshotColumn::Name || role != Qt::EditRole)
        return false;

    auto snapshot = snapshots.nth(index.row());
    auto name = value.toString();
    if (Database::updateSnapshotName(game, snapshot->first, name)) {
        snapshot->second.name = name;
        emit dataChanged(index, index, {Qt::DisplayRole});
        return true;
    }
    return false;
}

Qt::ItemFlags SnapshotModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return {Qt::NoItemFlags};

    auto default_flags = QAbstractTableModel::flags(index);
    auto col = static_cast<SnapshotColumn>(index.column());
    if (col == SnapshotColumn::Name)
        return default_flags | Qt::ItemIsEditable;
    return default_flags;
}

bool SnapshotModel::createSnapshot(QString name, QString league)
{
    auto exchange_it = exchange_cache.cost_cache.find(league);
    if (exchange_it == exchange_cache.cost_cache.end())
        return false;
    auto trade_it = trade_cache.cost_cache.find(league);
    if (trade_it == trade_cache.cost_cache.end())
        return false;

    auto query = Database::insertSnapshot(game);
    auto id = QUuid::createUuidV7();
    query.addBindValue(id);
    query.addBindValue(name);
    query.addBindValue(league);
    query.addBindValue(QJsonDocument{exchange_it->second.toJson()}.toJson(QJsonDocument::Compact));
    query.addBindValue(QJsonDocument{trade_it->second.toJson()}.toJson(QJsonDocument::Compact));
    if (!query.exec())
        return false;

    auto it = snapshots.lower_bound(id);
    beginInsertRows({}, snapshots.index_of(it), snapshots.index_of(it));
    snapshots.try_emplace(it, id, id, name, league);
    endInsertRows();

    return true;
}

void SnapshotModel::deleteSnapshots(int top, int bottom)
{
    auto query = Database::deleteSnapshot(game);
    for (int i = top; i <= bottom; ++i) {
        query.addBindValue(snapshots.nth(i)->first.toString());
        query.exec();
    }

    if (current) {
        auto pos = static_cast<int>(snapshots.index_of(snapshots.find(current->id)));
        if (top <= pos && pos <= bottom)
            clearCurrent();
    }
    beginRemoveRows({}, top, bottom);
    snapshots.erase(snapshots.nth(top), snapshots.nth(bottom + 1));
    endRemoveRows();
}

void SnapshotModel::deleteSnapshot(Snapshots::const_iterator it)
{
    auto query = Database::deleteSnapshot(game);
    query.addBindValue(it->first.toString());
    query.exec();

    if (current && it->first == current->id)
        clearCurrent();

    beginRemoveRows({}, snapshots.index_of(it), snapshots.index_of(it));
    snapshots.erase(it);
    endRemoveRows();
}

} // namespace planner
