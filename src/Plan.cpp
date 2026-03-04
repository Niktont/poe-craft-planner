#include "Plan.h"

#include "ExchangeRequestCache.h"
#include "PlanItem.h"
#include <QJsonArray>

namespace planner {

Plan::Plan(QUuid id, QString name)
    : name{name}
    , id_{id}
{}

Plan::Plan(QUuid id, const QJsonObject& plan_o, const ExchangeRequestCache& cache)
    : is_changed{false}
    , id_{id}
{
    game = cache.game;
    league = plan_o["league"].toString();
    final_step = QUuid{plan_o["final_step"].toString()};

    const auto steps_a = plan_o["steps"].toArray();
    steps.reserve(steps_a.size());
    for (auto step_v : steps_a)
        steps.emplace_back(step_v.toObject(), cache);
}

Plan::Plan(QUuid id, const Plan& o)
    : name{o.name}
    , game{o.game}
    , league{o.league}
    , steps{o.steps}
    , id_{id}
    , final_step{o.final_step}
{
    boost::container::flat_map<QUuid, QUuid> steps_id;
    steps_id.reserve(o.steps.size());
    for (size_t i = 0; i < o.steps.size(); ++i)
        steps_id.emplace(o.steps[i].id, steps[i].id);

    for (auto& step : steps)
        step.updateIds(steps_id);

    if (!final_step.isNull())
        final_step = steps_id[final_step];
}

QJsonObject Plan::saveJson() const
{
    QJsonObject plan_o;
    plan_o["league"] = league;
    plan_o["final_step"] = final_step.toString();

    QJsonArray steps_a;
    for (auto& step : steps)
        steps_a.push_back(step.saveJson());
    plan_o["steps"] = steps_a;

    return plan_o;
}

QJsonObject Plan::exportJson(const ExchangeRequestCache& cache, TradeRequestCache& trade_cache) const
{
    QJsonObject plan_o;
    plan_o["is_folder"] = false;
    plan_o["id"] = id_.toString();
    plan_o["name"] = name;

    plan_o["league"] = league;
    plan_o["final_step"] = final_step.toString();

    QJsonArray steps_a;
    for (auto& step : steps)
        steps_a.push_back(step.exportJson(cache, trade_cache));
    plan_o["steps"] = steps_a;

    return plan_o;
}

QUuid Plan::changeId()
{
    id_ = QUuid::createUuidV7();
    if (item_)
        item_->id = id_;
    return id_;
}

void Plan::setChanged()
{
    if (is_changed)
        return;
    is_changed = true;
    item_->setPlanChanged();
}

Plan::Steps::const_iterator Plan::findStepIt(const QUuid& step_id) const
{
    return std::ranges::find_if(steps, [&](const Step& step) { return step.id == step_id; });
}

const Step* Plan::findStep(const QUuid& step_id) const
{
    auto it = findStepIt(step_id);
    return it != steps.end() ? &(*it) : nullptr;
}

const Step* Plan::costStep() const
{
    if (steps.empty())
        return nullptr;

    if (final_step.isNull())
        return &steps.back();

    return findStep(final_step);
}

QStringList Plan::stepsName(size_t last) const
{
    QStringList result;
    last = std::min(last, steps.size());
    for (size_t i = 0; i < last; ++i)
        result.push_back(steps[i].name);
    return result;
}

} // namespace planner
