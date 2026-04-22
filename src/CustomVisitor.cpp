#include "CustomVisitor.h"
#include "Plan.h"
#include "PlanModel.h"
#include "Step.h"
#include "StepItem.h"
#include "TradeRequestCache.h"

namespace planner {

ItemCostVisitor::result_type ItemCostVisitor::operator()(const ExchangeItemData& data) const
{
    ItemCost cost;
    cost.cost_in_primary = exchange_cache->convertToPrimary(data.currency);
    if (cost.cost_in_primary.value == 0.0)
        return {};

    if (is_resource) {
        auto data_it = exchange_cache->currencyData(data.currency);
        if (data_it != exchange_cache->cache.end())
            cost.gold = data_it->second.gold_fee;

        cost.time = exchange_cache->time(data);
    }
    return cost;
}

ItemCostVisitor::result_type ItemCostVisitor::operator()(const TradeItemData& data) const
{
    auto cost_data = trade_cache->costData(data.request_key);
    if (!cost_data)
        return {};

    ItemCost cost;
    cost.cost_in_primary = exchange_cache->convertToPrimary(cost_data->cost.currency);
    cost.cost_in_primary.value *= cost_data->cost.value;
    if (is_resource) {
        cost.gold = cost_data->gold_fee;
        cost.time = trade_cache->time(data);
    }
    return cost;
}

ItemCostVisitor::result_type ItemCostVisitor::operator()(const CustomItemData& data) const
{
    ItemCost cost;
    cost.cost_in_primary = exchange_cache->convertToPrimary(data.cost.currency);
    cost.cost_in_primary.value *= data.cost.value;
    if (is_resource) {
        cost.gold = data.gold;
        cost.time = data.time;
    }
    return cost;
}

ItemCostVisitor::result_type ItemCostVisitor::operator()(const StepItemData& data) const
{
    if (auto plan_step = plan->findStep(data.step_id))
        return plan_step->cost();
    return {};
}

ItemCostVisitor::result_type ItemCostVisitor::operator()(const PlanItemData& data) const
{
    if (auto it = model->plans.find(data.plan_id); it != model->plans.end()) {
        if (auto cost_step = it->second.costStep())
            return cost_step->cost();
    }
    return {};
}

ItemCostVisitor::result_type ItemCostVisitor::calculateCost(const StepItem& item) const
{
    if (item.amount <= 0.0)
        return {};

    auto cost = std::visit(*this, item.data);
    if (!cost || (!cost->isValid() && cost->gold == 0.0 && cost->time.count() == 0.0))
        return {};

    cost->cost_in_primary.value *= item.amount;
    cost->gold *= item.amount;
    cost->time *= item.amount;
    return cost;
}

using result_type = CustomVisitor::result_type;
result_type CustomVisitor::operator()(const custom_tree::Item& item)
{
    result_type result;
    auto i = item.row - 1;
    if (i >= items->size())
        return result;

    result.cost = cost_visitor.calculateCost((*items)[i]);
    if (result.cost)
        result.used_items.insert(i);

    return result;
}

void CustomVisitor::operator()(result_type& l, const custom_tree::Operation& operation)
{
    return (this->*operation.op)(l, boost::apply_visitor(*this, operation.r));
}

result_type CustomVisitor::operator()(const custom_tree::Expression& expression)
{
    auto l = boost::apply_visitor(*this, expression.l);
    for (auto& op : expression.r)
        (*this)(l, op);
    return l;
}

result_type CustomVisitor::operator()(const custom_tree::NaryFunction& function)
{
    return (this->*function.fun)(function.operands);
}

void CustomVisitor::add(result_type& l, result_type&& r)
{
    l += std::move(r);
}

void CustomVisitor::sub(result_type& l, result_type&& r)
{
    l -= std::move(r);
}

void CustomVisitor::min(result_type& l, result_type&& r)
{
    CustomResult::min(l, std::move(r));
}

void CustomVisitor::max(result_type& l, result_type&& r)
{
    CustomResult::max(l, std::move(r));
}

result_type CustomVisitor::minN(const std::list<custom_tree::Operand>& operands)
{
    auto it = operands.begin();
    auto result = boost::apply_visitor(*this, *it);
    while (++it != operands.end())
        min(result, boost::apply_visitor(*this, *it));
    return result;
}

result_type CustomVisitor::maxN(const std::list<custom_tree::Operand>& operands)
{
    auto it = operands.begin();
    auto result = boost::apply_visitor(*this, *it);
    while (++it != operands.end())
        max(result, boost::apply_visitor(*this, *it));
    return result;
}

} // namespace planner
