#include "StepItem.h"
#include "ExchangeRequestCache.h"
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
