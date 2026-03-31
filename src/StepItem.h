#ifndef STEPITEM_H
#define STEPITEM_H

#include "CustomItemData.h"
#include "ExchangeItemData.h"
#include "ItemCost.h"
#include "PlanItemData.h"
#include "StepItemData.h"
#include "TradeItemData.h"
#include <variant>

namespace planner {
class Plan;
class ExchangeRequestCache;
class TradeRequestCache;
class PlanModel;

enum class StepItemType {
    Exchange,
    Trade,
    Custom,
    Step,
    Plan,
};

class StepItem
{
public:
    StepItem() = default;
    StepItem(const QJsonObject& item_o, const ExchangeRequestCache& cache);

    QJsonObject saveJson() const;
    QJsonObject exportJson(const ExchangeRequestCache& cache, TradeRequestCache& trade_cache) const;

    bool not_used{false};
    double amount{1.0};

    std::variant<ExchangeItemData, TradeItemData, CustomItemData, StepItemData, PlanItemData> data;

    bool is_success_result{true};

    StepItemType type() const { return static_cast<StepItemType>(data.index()); }
    void setType(StepItemType type);

    static const QStringList& typeList();
    QString typeStr() const { return typeList()[data.index()]; }

    ExchangeItemData* exchange() { return std::get_if<ExchangeItemData>(&data); }
    const ExchangeItemData* exchange() const { return std::get_if<ExchangeItemData>(&data); }

    TradeItemData* trade() { return std::get_if<TradeItemData>(&data); }
    const TradeItemData* trade() const { return std::get_if<TradeItemData>(&data); }

    CustomItemData* custom() { return std::get_if<CustomItemData>(&data); }
    const CustomItemData* custom() const { return std::get_if<CustomItemData>(&data); }

    StepItemData* step() { return std::get_if<StepItemData>(&data); }
    const StepItemData* step() const { return std::get_if<StepItemData>(&data); }

    PlanItemData* plan() { return std::get_if<PlanItemData>(&data); }
    const PlanItemData* plan() const { return std::get_if<PlanItemData>(&data); }

    std::optional<ItemCost> calculateCost(const Plan& plan,
                                          const ExchangeRequestCache& exchange_cache,
                                          const TradeRequestCache& trade_cache,
                                          const PlanModel& plan_model) const;
};

} // namespace planner

#endif // STEPITEM_H
