#ifndef EXCHANGECOSTDATA_H
#define EXCHANGECOSTDATA_H

#include "CurrencyCost.h"
#include "HashFunctions.h"
#include <boost/container/small_vector.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <QJsonObject>

namespace planner {

class ExchangeCostData
{
public:
    QString league_url;

    struct Data
    {
        QDateTime updated;
        CurrencyCost popular;
    };
    using Costs = boost::unordered::unordered_flat_map<QString, Data>;
    Costs costs;

    Costs::iterator costData(QString id) { return costs.find(id); }
    Costs::const_iterator costData(QString id) const { return costs.find(id); }

    boost::container::small_vector<CoreCurrency, 5> core_currencies;

    Currency primaryCurrency() const
    {
        return !core_currencies.empty() ? core_currencies.back().currency : Currency{};
    }
    std::optional<double> primaryValue(QString id) const;

    mutable bool is_changed{true};

    QJsonObject toJson() const;
    static ExchangeCostData fromJson(const QJsonObject& cost_data_o,
                                     const ExchangeRequestCache& cache);
};

} // namespace planner

#endif // EXCHANGECOSTDATA_H
