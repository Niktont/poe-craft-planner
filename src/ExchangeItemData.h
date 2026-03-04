#ifndef EXCHANGEITEMDATA_H
#define EXCHANGEITEMDATA_H

#include "CurrencyCost.h"
#include "ExchangeRequestCache.h"
#include "ItemTime.h"
#include <QString>

namespace planner {

class ExchangeItemData
{
public:
    ExchangeItemData() = default;
    ExchangeItemData(const QJsonObject& item_o, const ExchangeRequestCache& cache)
        : currency{Currency::fromJson(item_o["currency_id"], cache)}
    {
        if (auto time_v = item_o["time"]; !time_v.isUndefined())
            time = ItemTime(time_v.toDouble());
    }
    void toJson(QJsonObject& item_o) const
    {
        item_o["currency_id"] = currency.toJson();
        if (time)
            item_o["time"] = time->count();
    }
    void exportJson(QJsonObject& item_o, const ExchangeRequestCache& cache) const
    {
        toJson(item_o);
        if (!time)
            item_o["time"] = cache.time(*this).count();
    }

    Currency currency;

    std::optional<ItemTime> time;
};

} // namespace planner

#endif // EXCHANGEITEMDATA_H
