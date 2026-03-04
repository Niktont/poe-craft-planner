#include "TradeCostData.h"
#include "ExchangeRequestCache.h"
#include <QJsonArray>

namespace planner {

QJsonObject TradeCostData::toJson() const
{
    QJsonObject obj;

    QJsonArray costs_a;
    for (auto& [key, data] : costs) {
        QJsonObject cost_o;
        cost_o["id"] = key.request_id;
        cost_o["domain"] = static_cast<std::underlying_type_t<Domain>>(key.domain);
        cost_o["updated"] = data.updated.toSecsSinceEpoch();
        cost_o["cost"] = data.cost.toJson();
        cost_o["fee"] = data.gold_fee;
        cost_o["available"] = data.total_available;
        costs_a.append(cost_o);
    }
    obj["costs"] = costs_a;
    return obj;
}

TradeCostData TradeCostData::fromJson(const QJsonObject& cost_data_o,
                                      const ExchangeRequestCache& cache)
{
    TradeCostData data;

    const auto costs_a{cost_data_o["costs"].toArray()};
    for (auto cost_v : costs_a) {
        auto cost_o{cost_v.toObject()};

        QString id{cost_o["id"].toString()};
        if (id.isEmpty())
            continue;
        Domain domain{static_cast<std::underlying_type_t<Domain>>(cost_o["domain"].toInt())};
        TradeRequestKey key{id, domain};

        auto& cost_data = data.costs.try_emplace(std::move(key)).first->second;
        cost_data.updated = QDateTime::fromSecsSinceEpoch(cost_o["updated"].toInteger());
        cost_data.cost = CurrencyCost::fromJson(cost_o["cost"].toObject(), cache);
        cost_data.gold_fee = cost_o["fee"].toDouble();
        cost_data.total_available = cost_o["available"].toInt();

        cache.prepareCurrency(cost_data.cost.currency);
    }
    data.is_changed = false;

    return data;
}

} // namespace planner
