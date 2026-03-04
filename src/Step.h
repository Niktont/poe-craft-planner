#ifndef STEP_H
#define STEP_H

#include "StepItem.h"
#include <boost/container/flat_map.hpp>
#include <vector>
#include <QJsonObject>
#include <QString>
#include <QUuid>

namespace planner {
enum class ResourceCalcMethod {
    Sum,
    Min,
};
enum class ResultCalcMethod {
    Sum,
    Max,
};

const QStringList& resourceMethods();
const QStringList& resultMethods();

class Step
{
public:
    Step()
        : id{QUuid::createUuidV7()}
    {}
    Step(const QJsonObject& step_o, const ExchangeRequestCache& cache);

    Step(const Step& o);
    Step(Step&&) = default;
    Step& operator=(Step o);

    QJsonObject saveJson() const;
    QJsonObject exportJson(const ExchangeRequestCache& cache, TradeRequestCache& trade_cache) const;

    QUuid id;

    QString name;
    QString description;

    ItemCost resources_cost;
    ItemCost results_cost;
    ItemCost failed_cost;

    ResourceCalcMethod resource_calc{ResourceCalcMethod::Sum};
    std::vector<StepItem> resources;
    ResultCalcMethod result_calc{ResultCalcMethod::Sum};
    std::vector<StepItem> results;

    ItemCost cost() const { return resources_cost - failed_cost; }
    CurrencyCost costCurrency() const
    {
        return resources_cost.cost_in_primary - failed_cost.cost_in_primary;
    }
    double costGold() const { return resources_cost.gold - failed_cost.gold; }
    ItemTime costTime() const { return resources_cost.time - failed_cost.time; }

    void updateIds(const boost::container::flat_map<QUuid, QUuid>& steps_id);

private:
    void commonJson(QJsonObject& step_o) const;
};

} // namespace planner
#endif // STEP_H
