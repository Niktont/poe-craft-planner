#include "ItemCost.h"

namespace planner {

QJsonObject ItemCost::toJson() const
{
    QJsonObject cost_o;
    cost_o["cost"] = cost_in_primary.toJson();
    cost_o["gold"] = gold;
    cost_o["time"] = time.count();
    return cost_o;
}

ItemCost ItemCost::fromJson(const QJsonObject& cost_o, const ExchangeRequestCache& cache)
{
    ItemCost cost;
    cost.cost_in_primary = CurrencyCost::fromJson(cost_o["cost"].toObject(), cache);
    cost.gold = cost_o["gold"].toDouble();
    cost.time = ItemTime{cost_o["time"].toDouble()};
    return cost;
}
} // namespace planner
