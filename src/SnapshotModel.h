#ifndef SNAPSHOTMODEL_H
#define SNAPSHOTMODEL_H

#include "Game.h"
#include "Snapshot.h"
#include <boost/container/flat_map.hpp>
#include <deque>
#include <QAbstractTableModel>
#include <QUuid>

class QCompleter;

namespace planner {
class ExchangeRequestCache;
class TradeRequestCache;

enum class SnapshotColumn {
    Name,
    Date,
    League,

    last = League,
};

class SnapshotModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit SnapshotModel(ExchangeRequestCache& exchange_cache,
                           TradeRequestCache& trade_cache,
                           QObject* parent = nullptr);

    bool readDatabase();

    const Game game;

    using Snapshots = boost::container::flat_map<QUuid, SnapshotData>;
    Snapshots snapshots;

    QCompleter* completer;

    QString currentName() const;

    Snapshot* current{};

    void setCurrent(const QUuid& id);
    void clearCurrent();

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool createSnapshot(QString name, QString league);

    void deleteSnapshots(int top, int bottom);
    void deleteSnapshot(Snapshots::const_iterator it);

signals:
    void currentChanged(planner::Game game, planner::Snapshot* current);

private:
    ExchangeRequestCache& exchange_cache;
    TradeRequestCache& trade_cache;

    void setCurrent(const QModelIndex& index);
    std::deque<Snapshot> cache;
};

} // namespace planner

#endif // SNAPSHOTMODEL_H
