#ifndef STEP_H
#define STEP_H

#include "CustomExpression.h"
#include "StepItem.h"
#include <boost/container/container_fwd.hpp>
#include <vector>
#include <QJsonObject>
#include <QString>
#include <QUuid>

namespace planner {
enum class ResourceCalcMethod {
    Sum,
    Min,
    Custom,
};
enum class ResultCalcMethod {
    Sum,
    Max,
    Custom,
};

class CustomCalcData
{
public:
    CustomCalcData() = default;
    CustomCalcData(const QJsonObject& custom_o)
        : text{custom_o["text"].toString()}
    {}
    CustomCalcData(const CustomCalcData&) = default;
    CustomCalcData(CustomCalcData&&) noexcept = default;
    CustomCalcData& operator=(const CustomCalcData&) = default;
    CustomCalcData& operator=(CustomCalcData&&) noexcept = default;

    QJsonObject toJson() const
    {
        QJsonObject custom_o;
        custom_o["text"] = text;
        return custom_o;
    }

    QString text;
    std::optional<custom_tree::Expression> tree;
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
    Step(Step&&) noexcept = default;
    Step& operator=(Step o) noexcept;

    QJsonObject saveJson() const;
    QJsonObject exportJson(const ExchangeRequestCache& cache, TradeRequestCache& trade_cache) const;
    void gatherDependencies(std::vector<QUuid>& dependencies) const;
    void gatherDependencies(const PlanModel& model, std::vector<QUuid>& dependencies) const;

    QUuid id;

    QString name;
    QString description;

    ItemCost resources_cost;
    ItemCost results_cost;
    ItemCost failed_cost;

    ResourceCalcMethod resource_calc{ResourceCalcMethod::Sum};
    CustomCalcData custom_resource_data;
    std::vector<StepItem> resources;

    ResultCalcMethod result_calc{ResultCalcMethod::Sum};
    CustomCalcData custom_result_data;
    std::vector<StepItem> results;

    ItemCost cost() const { return resources_cost - failed_cost; }
    CurrencyCost costCurrency() const
    {
        return resources_cost.cost_in_primary - failed_cost.cost_in_primary;
    }
    double costGold() const { return resources_cost.gold - failed_cost.gold; }
    ItemTime costTime() const { return resources_cost.time - failed_cost.time; }

    ItemCost profit() const { return results_cost - cost(); }

    void updateIds(const boost::container::flat_map<QUuid, QUuid>& changed_ids);
    void updatePlanIds(const boost::container::flat_map<QUuid, QUuid>& changed_ids);

private:
    void commonJson(QJsonObject& step_o) const;
};

} // namespace planner
#endif // STEP_H
