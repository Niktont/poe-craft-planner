#include "StepItem.h"
#include "ExchangeRequestCache.h"
#include "Plan.h"
#include "PlanModel.h"
#include "StepItemModel.h"
#include "TradeRequestCache.h"

using namespace Qt::StringLiterals;

namespace planner {

const QStringList& StepItem::typeList()
{
    static const QStringList list{
        StepItemModel::tr("Exchange"),
        StepItemModel::tr("Trade"),
        StepItemModel::tr("Custom"),
        StepItemModel::tr("Step"),
        StepItemModel::tr("Plan"),
    };
    return list;
}

std::optional<ItemCost> StepItem::calculateCost(const Plan& plan,
                                                const ExchangeRequestCache& exchange_cache,
                                                const TradeRequestCache& trade_cache,
                                                const PlanModel& plan_model) const
{
    if (amount <= 0.0)
        return {};

    ItemCost result;
    if (auto exchange = this->exchange()) {
        result.cost_in_primary = exchange_cache.convertToPrimary(exchange->currency);
        if (result.cost_in_primary.value == 0.0)
            return {};

        auto data_it = exchange_cache.currencyData(exchange->currency);
        if (data_it != exchange_cache.cache.end())
            result.gold = data_it->second.gold_fee;

        result.time = exchange_cache.time(*exchange);
    } else if (auto trade = this->trade()) {
        if (auto costData = trade_cache.costData(trade->request_key)) {
            result.cost_in_primary = exchange_cache.convertToPrimary(costData->cost.currency);
            result.cost_in_primary.value *= costData->cost.value;
            result.gold = costData->gold_fee;
            result.time = trade_cache.time(*trade);
        } else
            return {};
    } else if (auto custom = this->custom()) {
        result.cost_in_primary = exchange_cache.convertToPrimary(custom->cost.currency);
        result.cost_in_primary.value *= custom->cost.value;
        result.gold = custom->gold;
        result.time = custom->time;
    } else if (auto step = this->step()) {
        if (auto plan_step = plan.findStep(step->step_id); plan_step)
            result = plan_step->cost();
        else
            return {};
    } else if (auto plan = this->plan()) {
        if (auto it = plan_model.plans.find(plan->plan_id); it != plan_model.plans.end()) {
            if (auto cost_step = it->second.costStep())
                result = cost_step->cost();
            else
                return {};
        } else
            return {};
    }

    if (!result.isValid() && result.gold == 0.0 && result.time.count() == 0.0)
        return {};

    result.cost_in_primary.value *= amount;
    result.gold *= amount;
    result.time *= amount;
    return result;
}

StepItem::StepItem(const QJsonObject& item_o, const ExchangeRequestCache& cache)
    : not_used{item_o["not_used"].toBool()}
    , amount{item_o["amount"].toDouble()}
    , is_success_result{item_o["success"].toBool()}
{
    auto type = static_cast<StepItemType>(item_o["type"].toInt());
    switch (type) {
    case StepItemType::Exchange:
        data.emplace<ExchangeItemData>(item_o, cache);
        break;
    case StepItemType::Trade:
        data.emplace<TradeItemData>(item_o);
        break;
    case StepItemType::Custom:
        data.emplace<CustomItemData>(item_o, cache);
        break;
    case StepItemType::Step:
        data.emplace<StepItemData>(item_o);
        break;
    case StepItemType::Plan:
        data.emplace<PlanItemData>(item_o);
        break;
    }
}

QJsonObject StepItem::saveJson() const
{
    QJsonObject item_o;
    item_o["not_used"] = not_used;
    item_o["amount"] = amount;
    item_o["type"] = static_cast<int>(data.index());

    if (auto exchange = this->exchange()) {
        exchange->toJson(item_o);
    } else if (auto trade = this->trade()) {
        trade->toJson(item_o);
    } else if (auto custom = this->custom()) {
        custom->toJson(item_o);
    } else if (auto step = this->step()) {
        step->toJson(item_o);
    } else if (auto plan = this->plan()) {
        plan->toJson(item_o);
    }

    item_o["success"] = is_success_result;
    return item_o;
}

QJsonObject StepItem::exportJson(const ExchangeRequestCache& cache,
                                 TradeRequestCache& trade_cache) const
{
    QJsonObject item_o;
    item_o["not_used"] = not_used;
    item_o["amount"] = amount;
    item_o["type"] = static_cast<int>(data.index());

    if (auto exchange = this->exchange()) {
        exchange->exportJson(item_o, cache);
    } else if (auto trade = this->trade()) {
        trade->exportJson(item_o, trade_cache);
    } else if (auto custom = this->custom()) {
        custom->toJson(item_o);
    } else if (auto step = this->step()) {
        step->toJson(item_o);
    } else if (auto plan = this->plan()) {
        plan->toJson(item_o);
    }

    item_o["success"] = is_success_result;
    return item_o;
}

void StepItem::setType(StepItemType type)
{
    switch (type) {
    case StepItemType::Exchange:
        data.emplace<ExchangeItemData>();
        return;
    case StepItemType::Trade:
        data.emplace<TradeItemData>();
        return;
    case StepItemType::Custom:
        data.emplace<CustomItemData>();
        return;
    case StepItemType::Step:
        data.emplace<StepItemData>();
        is_success_result = false;
        return;
    case StepItemType::Plan:
        data.emplace<PlanItemData>();
        is_success_result = false;
        return;
    }
}

} // namespace planner
