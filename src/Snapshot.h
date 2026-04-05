#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "ExchangeCostData.h"
#include "TradeCostData.h"
#include <QDateTime>
#include <QUuid>

namespace planner {
class Snapshot
{
public:
    QUuid id;
    ExchangeCostData exchange;
    TradeCostData trade;
};
class SnapshotData
{
public:
    SnapshotData() = default;
    SnapshotData(const QUuid& id_v7, QString name, QString league)
        : name{std::move(name)}
        , league{std::move(league)}
    {
        qint64 date_ms = id_v7.data1;
        date_ms = date_ms << 16;
        date_ms |= id_v7.data2;
        date = QDateTime::fromMSecsSinceEpoch(date_ms);
    }

    QString name;
    QDateTime date;
    QString league;
};

} // namespace planner

#endif // SNAPSHOT_H
