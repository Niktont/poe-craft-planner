#ifndef TRADEITEMDATA_H
#define TRADEITEMDATA_H

#include "TradeRequestKey.h"
#include "ItemTime.h"
#include <QString>

namespace planner {
class TradeRequestCache;

class TradeItemData
{
public:
    TradeItemData() = default;
    TradeItemData(const QJsonObject& item_o)
        : name{item_o["name"].toString()}
        , request_key{TradeRequestKey::fromJson(item_o["trade_request"].toObject())}
    {
        if (auto time_v = item_o["time"]; !time_v.isUndefined())
            time = ItemTime(time_v.toDouble());
    }

    void toJson(QJsonObject& item_o) const
    {
        item_o["name"] = name;
        item_o["trade_request"] = request_key.toJson();
        if (time)
            item_o["time"] = time->count();
    }

    void exportJson(QJsonObject& item_o, TradeRequestCache& cache) const;

    QString name;

    TradeRequestKey request_key;

    std::optional<ItemTime> time;
};
} // namespace planner

#endif // TRADEITEMDATA_H
