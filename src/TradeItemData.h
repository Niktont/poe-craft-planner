#ifndef TRADEITEMDATA_H
#define TRADEITEMDATA_H

#include "TradeRequestCache.h"
#include "TradeRequestKey.h"
#include "ItemTime.h"
#include <QString>

namespace planner {

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

    void exportJson(QJsonObject& item_o, TradeRequestCache& cache) const
    {
        toJson(item_o);
        cache.export_requests.emplace(request_key);
        if (name.isEmpty()) {
            if (auto it = cache.requestData(request_key); it != cache.cache.end())
                item_o["name"] = it->second.name();
        }
        if (!time)
            item_o["time"] = cache.time(*this).count();
    }

    QString name;

    TradeRequestKey request_key;

    std::optional<ItemTime> time;
};
} // namespace planner

#endif // TRADEITEMDATA_H
