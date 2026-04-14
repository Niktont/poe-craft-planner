#include "Step.h"
#include "Plan.h"
#include "PlanItem.h"
#include "PlanModel.h"
#include "StepItemsWidget.h"
#include <boost/container/flat_map.hpp>
#include <QJsonArray>

namespace planner {

Step::Step(const QJsonObject& step_o, const ExchangeRequestCache& cache)
    : id{QUuid{step_o["id"].toStringView()}}
    , name{step_o["name"].toString()}
    , description{step_o["description"].toString()}
    , resources_cost{ItemCost::fromJson(step_o["resources_cost"].toObject(), cache)}
    , results_cost{ItemCost::fromJson(step_o["results_cost"].toObject(), cache)}
    , failed_cost{ItemCost::fromJson(step_o["failed_cost"].toObject(), cache)}
    , resource_calc{static_cast<ResourceCalcMethod>(step_o["resource_calc"].toInt())}
    , custom_resource_data{step_o["custom_resource_calc"].toObject()}
    , result_calc{static_cast<ResultCalcMethod>(step_o["result_calc"].toInt())}
    , custom_result_data{step_o["custom_result_calc"].toObject()}
{
    const auto resources_a = step_o["resources"].toArray();
    resources.reserve(resources_a.size());
    for (auto resource_v : resources_a)
        resources.emplace_back(resource_v.toObject(), cache);

    const auto results_a = step_o["results"].toArray();
    results.reserve(results_a.size());
    for (auto result_v : results_a)
        results.emplace_back(result_v.toObject(), cache);
}

Step::Step(const Step& o)
    : id{QUuid::createUuidV7()}
    , name{o.name}
    , description{o.description}
    , resources_cost{o.resources_cost}
    , results_cost{o.results_cost}
    , failed_cost{o.failed_cost}
    , resource_calc{o.resource_calc}
    , custom_resource_data{o.custom_resource_data}
    , resources{o.resources}
    , result_calc{o.result_calc}
    , custom_result_data{o.custom_result_data}
    , results{o.results}
{}

Step& Step::operator=(Step o)
{
    using std::swap;
    swap(id, o.id);
    swap(name, o.name);
    swap(description, o.description);
    swap(resources_cost, o.resources_cost);
    swap(results_cost, o.results_cost);
    swap(failed_cost, o.failed_cost);
    swap(resource_calc, o.resource_calc);
    swap(custom_resource_data, o.custom_resource_data);
    swap(resources, o.resources);
    swap(result_calc, o.result_calc);
    swap(custom_result_data, o.custom_result_data);
    swap(results, o.results);
    return *this;
}

QJsonObject Step::saveJson() const
{
    QJsonObject step_o;
    commonJson(step_o);

    QJsonArray resources_a;
    for (auto& resource : resources)
        resources_a.push_back(resource.saveJson());
    step_o["resources"] = resources_a;

    QJsonArray results_a;
    for (auto& result : results)
        results_a.push_back(result.saveJson());
    step_o["results"] = results_a;

    return step_o;
}

QJsonObject Step::exportJson(const ExchangeRequestCache& cache, TradeRequestCache& trade_cache) const
{
    QJsonObject step_o;
    commonJson(step_o);

    QJsonArray resources_a;
    for (auto& resource : resources)
        resources_a.push_back(resource.exportJson(cache, trade_cache));
    step_o["resources"] = resources_a;

    QJsonArray results_a;
    for (auto& result : results)
        results_a.push_back(result.exportJson(cache, trade_cache));
    step_o["results"] = results_a;

    return step_o;
}

void Step::gatherDependencies(std::vector<QUuid>& dependencies) const
{
    for (auto& item : resources) {
        if (auto plan = item.plan();
            plan && !plan->plan_id.isNull()
            && std::ranges::find(dependencies, plan->plan_id) == dependencies.end())
            dependencies.push_back(plan->plan_id);
    }
    for (auto& item : results) {
        if (auto plan = item.plan();
            plan && !plan->plan_id.isNull()
            && std::ranges::find(dependencies, plan->plan_id) == dependencies.end())
            dependencies.push_back(plan->plan_id);
    }
}

void Step::gatherDependencies(const PlanModel& model, std::vector<QUuid>& dependencies) const
{
    for (auto& item : resources) {
        if (auto plan = item.plan();
            plan && !plan->plan_id.isNull()
            && std::ranges::find(dependencies, plan->plan_id) == dependencies.end()) {
            if (auto it = model.plans.find(plan->plan_id); it != model.plans.end()) {
                dependencies.push_back(plan->plan_id);
                it->second.gatherDependencies(model, dependencies);
            }
        }
    }
    for (auto& item : results) {
        if (auto plan = item.plan();
            plan && !plan->plan_id.isNull()
            && std::ranges::find(dependencies, plan->plan_id) == dependencies.end()) {
            if (auto it = model.plans.find(plan->plan_id); it != model.plans.end()) {
                dependencies.push_back(plan->plan_id);
                it->second.gatherDependencies(model, dependencies);
            }
        }
    }
}

void Step::updateIds(const boost::container::flat_map<QUuid, QUuid>& changed_ids)
{
    for (auto& item : resources) {
        if (auto step = item.step(); step && !step->step_id.isNull())
            if (auto it = changed_ids.find(step->step_id); it != changed_ids.end())
                step->step_id = it->second;
    }
    for (auto& item : results) {
        if (auto step = item.step(); step && !step->step_id.isNull())
            if (auto it = changed_ids.find(step->step_id); it != changed_ids.end())
                step->step_id = it->second;
    }
}

void Step::updatePlanIds(const boost::container::flat_map<QUuid, QUuid>& changed_ids)
{
    for (auto& [old_id, new_id] : changed_ids) {
        description.replace(old_id.toString(QUuid::WithoutBraces),
                            new_id.toString(QUuid::WithoutBraces));
    }
    for (auto& item : resources) {
        if (auto plan = item.plan(); plan && !plan->plan_id.isNull())
            if (auto it = changed_ids.find(plan->plan_id); it != changed_ids.end())
                plan->plan_id = it->second;
    }
    for (auto& item : results) {
        if (auto plan = item.plan(); plan && !plan->plan_id.isNull())
            if (auto it = changed_ids.find(plan->plan_id); it != changed_ids.end())
                plan->plan_id = it->second;
    }
}

void Step::commonJson(QJsonObject& step_o) const
{
    step_o["id"] = id.toString();
    step_o["name"] = name;
    step_o["description"] = description;

    step_o["resources_cost"] = resources_cost.toJson();
    step_o["results_cost"] = results_cost.toJson();
    step_o["failed_cost"] = failed_cost.toJson();

    step_o["resource_calc"] = static_cast<std::underlying_type_t<ResourceCalcMethod>>(resource_calc);
    step_o["custom_resource_calc"] = custom_resource_data.toJson();

    step_o["result_calc"] = static_cast<std::underlying_type_t<ResourceCalcMethod>>(result_calc);
    step_o["custom_result_calc"] = custom_result_data.toJson();
}

const QStringList& resourceMethods()
{
    static const QStringList list{
        StepItemsWidget::tr("Sum"),
        StepItemsWidget::tr("Min"),
        StepItemsWidget::tr("Custom"),
    };
    return list;
}

const QStringList& resultMethods()
{
    static const QStringList list{
        StepItemsWidget::tr("Sum"),
        StepItemsWidget::tr("Max"),
        StepItemsWidget::tr("Custom"),
    };
    return list;
}

} // namespace planner
