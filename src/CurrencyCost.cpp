#include "CurrencyCost.h"
#include "ExchangeRequestCache.h"

namespace planner {

Currency Currency::fromJson(const QJsonValue& id, const ExchangeRequestCache& cache)
{
    Currency currency{id.toString()};
    if (!currency.id.isEmpty())
        cache.prepareCurrency(currency);
    return currency;
}

CoreCurrency CoreCurrency::fromJson(const QJsonObject& core, const ExchangeRequestCache& cache)
{
    CoreCurrency core_currency;
    core_currency.ratio_to_primary = core["ratio"].toDouble();
    core_currency.currency = Currency::fromJson(core["currency"], cache);
    return core_currency;
}

CurrencyCost CurrencyCost::fromJson(const QJsonObject& cost, const ExchangeRequestCache& cache)
{
    CurrencyCost currency_cost;
    currency_cost.value = cost["value"].toDouble();
    currency_cost.currency = Currency::fromJson(cost["currency"], cache);

    return currency_cost;
}

} // namespace planner
