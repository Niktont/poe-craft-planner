#include "Step.h"
#include "StepItemsWidget.h"
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
    , result_calc{static_cast<ResultCalcMethod>(step_o["result_calc"].toInt())}
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
    , resources{o.resources}
    , result_calc{o.result_calc}
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
    swap(resources, o.resources);
    swap(result_calc, o.result_calc);
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

void Step::updateIds(const boost::container::flat_map<QUuid, QUuid>& steps_id)
{
    for (auto& item : resources) {
        if (auto step = item.step(); step && !step->step_id.isNull())
            if (auto it = steps_id.find(step->step_id); it != steps_id.end())
                step->step_id = it->second;
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
    step_o["result_calc"] = static_cast<std::underlying_type_t<ResourceCalcMethod>>(result_calc);
}

const QStringList& resourceMethods()
{
    static const QStringList list{
        StepItemsWidget::tr("Sum"),
        StepItemsWidget::tr("Min"),
    };
    return list;
}

const QStringList& resultMethods()
{
    static const QStringList list{
        StepItemsWidget::tr("Sum"),
        StepItemsWidget::tr("Max"),
    };
    return list;
}

} // namespace planner
