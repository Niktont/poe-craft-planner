#ifndef CUSTOMVISITOR_H
#define CUSTOMVISITOR_H

#include "CustomExpression.h"

namespace planner {
class Plan;
class StepItem;
class ExchangeRequestCache;
class TradeRequestCache;
class PlanModel;
class ExchangeItemData;
class TradeItemData;
class CustomItemData;
class StepItemData;
class PlanItemData;

class ItemCostVisitor : public boost::static_visitor<std::optional<ItemCost>>
{
public:
    result_type operator()(const ExchangeItemData& data) const;
    result_type operator()(const TradeItemData& data) const;
    result_type operator()(const CustomItemData& data) const;
    result_type operator()(const StepItemData& data) const;
    result_type operator()(const PlanItemData& data) const;

    result_type calculateCost(const StepItem& item) const;

    void setPlan(const Plan& plan_) { plan = &plan_; }

    void setIsResource(bool is_resource_) { is_resource = is_resource_; }

    void setModels(const ExchangeRequestCache& exchange_cache_,
                   const TradeRequestCache& trade_cache_,
                   const PlanModel& model_)
    {
        exchange_cache = &exchange_cache_;
        trade_cache = &trade_cache_;
        model = &model_;
    }

private:
    bool is_resource{};

    const Plan* plan{};
    const ExchangeRequestCache* exchange_cache{};
    const TradeRequestCache* trade_cache{};
    const PlanModel* model{};
};

class CustomVisitor : public boost::static_visitor<CustomResult>
{
public:
    result_type operator()(const custom_tree::Item& item);
    void operator()(result_type& l, const custom_tree::Operation& operation);

    result_type operator()(const custom_tree::Expression& expression);
    result_type operator()(const custom_tree::NaryFunction& function);

    void add(result_type& l, result_type&& r);
    void sub(result_type& l, result_type&& r);

    void min(result_type& l, result_type&& r);
    void max(result_type& l, result_type&& r);

    result_type minN(const std::list<custom_tree::Operand>& operands);
    result_type maxN(const std::list<custom_tree::Operand>& operands);

    void setItems(const std::vector<StepItem>& items_, bool is_resources)
    {
        items = &items_;
        cost_visitor.setIsResource(is_resources);
    }

    ItemCostVisitor cost_visitor;

private:
    const std::vector<StepItem>* items{};
};

} // namespace planner

#endif // CUSTOMVISITOR_H
