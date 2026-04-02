#include "CustomVisitor.h"
#include "StepItem.h"

namespace planner {

using result_type = CustomVisitor::result_type;
result_type CustomVisitor::operator()(const custom_tree::Item& item)
{
    result_type result;
    auto i = item.row - 1;
    if (i >= items->size())
        return result;

    result.cost = (*items)[i].calculateCost(*plan, *exchange_cache, *trade_cache, *model);
    if (result.cost)
        result.used_items.insert(i);
    else
        not_used_items.insert(i);

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
    CustomResult::min(l, std::move(r), not_used_items);
}

void CustomVisitor::max(result_type& l, result_type&& r)
{
    CustomResult::max(l, std::move(r), not_used_items);
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
