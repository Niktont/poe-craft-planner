#include "ExchangeCostData.h"

#include "ExchangeRequestCache.h"
#include <QJsonArray>

namespace planner {

std::optional<double> ExchangeCostData::primaryValue(QString id) const
{
    auto it = std::ranges::find_if(core_currencies,
                                   [&id](const auto& core) { return core.currency.id == id; });
    if (it == core_currencies.end())
        return {};

    return 1.0 / it->ratio_to_primary;
}

QJsonObject ExchangeCostData::toJson() const
{
    QJsonObject obj;

    obj["league_url"] = league_url;

    QJsonArray costs_a;
    for (auto& [id, data] : costs) {
        QJsonObject cost_o;
        cost_o["id"] = id;
        cost_o["updated"] = data.updated.toSecsSinceEpoch();
        cost_o["popular"] = data.popular.toJson();
        costs_a.append(cost_o);
    }
    obj["costs"] = costs_a;

    QJsonArray core_currencies_a;
    for (auto& core : core_currencies)
        core_currencies_a.append(core.toJson());

    obj["cores"] = core_currencies_a;

    return obj;
}

ExchangeCostData ExchangeCostData::fromJson(const QJsonObject& cost_data_o,
                                            const ExchangeRequestCache& cache)
{
    ExchangeCostData data;

    data.league_url = cost_data_o["league_url"].toString();

    const auto costs_a{cost_data_o["costs"].toArray()};
    data.costs.reserve(costs_a.size());
    for (auto& cost : costs_a) {
        QJsonObject cost_o{cost.toObject()};

        QString id{cost_o["id"].toString()};
        if (id.isEmpty())
            continue;
        cache.shareCurrencyId(id);

        auto& cost_data = data.costs.try_emplace(std::move(id)).first->second;
        cost_data.updated = QDateTime::fromSecsSinceEpoch(cost_o["updated"].toInteger());
        cost_data.popular = CurrencyCost::fromJson(cost_o["popular"].toObject(), cache);

        cache.prepareCurrency(cost_data.popular.currency);
    }

    const QJsonArray core_currencies_a{cost_data_o["cores"].toArray()};
    for (auto& core : core_currencies_a)
        data.core_currencies.push_back(CoreCurrency::fromJson(core.toObject(), cache));

    return data;
}

} // namespace planner
