#ifndef TRADECOSTDATA_H
#define TRADECOSTDATA_H

#include "CurrencyCost.h"
#include "TradeRequestKey.h"
#include <boost/unordered/unordered_flat_map.hpp>
#include <QDateTime>
#include <QJsonObject>

namespace planner {

class TradeCostData
{
public:
    struct Data
    {
        QDateTime updated;
        CurrencyCost cost;
        double gold_fee{};
        int total_available{};
    };
    boost::unordered::unordered_flat_map<TradeRequestKey, Data> costs;
    mutable bool is_changed{};

    QJsonObject toJson() const;
    static TradeCostData fromJson(const QJsonObject& cost_data_o, const ExchangeRequestCache& cache);
};

} // namespace planner

#endif // TRADECOSTDATA_H
