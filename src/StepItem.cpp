#include "StepItem.h"
#include "ExchangeRequestCache.h"
#include "Plan.h"
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
    };
    return list;
}

ItemCost StepItem::calculateCost(const Plan& plan,
                                 const ExchangeRequestCache& exchange_cache,
                                 const TradeRequestCache& trade_cache) const
{
    ItemCost result;
    if (amount <= 0.0)
        return result;

    if (auto exchange = this->exchange()) {
        auto primary = exchange_cache.convertToPrimary(exchange->currency);
        result.cost_in_primary.value = amount * primary.value;
        result.cost_in_primary.currency = primary.currency;

        auto data_it = exchange_cache.currencyData(exchange->currency);
        if (data_it != exchange_cache.cache.end())
            result.gold = amount * data_it->second.gold_fee;

        result.time = amount * exchange_cache.time(*exchange);
    } else if (auto trade = this->trade()) {
        auto costData = trade_cache.costData(trade->request_key);
        if (costData) {
            auto primary = exchange_cache.convertToPrimary(costData->cost.currency);
            result.cost_in_primary.value = amount * costData->cost.value * primary.value;
            result.cost_in_primary.currency = primary.currency;
            result.gold = amount * costData->gold_fee;
            result.time = amount * trade_cache.time(*trade);
        }
    } else if (auto custom = this->custom()) {
        auto primary = exchange_cache.convertToPrimary(custom->cost.currency);
        result.cost_in_primary.value = amount * custom->cost.value * primary.value;
        result.cost_in_primary.currency = primary.currency;
        result.gold = amount * custom->gold;
        result.time = amount * custom->time;
    } else if (auto step = this->step()) {
        if (auto plan_step = plan.findStep(step->step_id); plan_step) {
            result = plan_step->resources_cost - plan_step->failed_cost;
        }
    }

    return result;
}

StepItem::StepItem(const QJsonObject& item_o, const ExchangeRequestCache& cache)
    : amount{item_o["amount"].toDouble()}
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
    }
}

QJsonObject StepItem::saveJson() const
{
    QJsonObject item_o;
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
    }

    item_o["success"] = is_success_result;
    return item_o;
}

QJsonObject StepItem::exportJson(const ExchangeRequestCache& cache,
                                 TradeRequestCache& trade_cache) const
{
    QJsonObject item_o;
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
    }

    item_o["success"] = is_success_result;
    return item_o;
}

bool StepItem::setType(StepItemType type)
{
    switch (type) {
    case StepItemType::Exchange:
        data.emplace<ExchangeItemData>();
        return true;
    case StepItemType::Trade:
        data.emplace<TradeItemData>();
        return true;
    case StepItemType::Custom:
        data.emplace<CustomItemData>();
        return true;
    case StepItemType::Step:
        data.emplace<StepItemData>();
        is_success_result = false;
        return true;
    }

    return false;
}

} // namespace planner
