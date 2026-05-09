#include "ArithmeticVisitor.h"

namespace planner {

double ArithmeticVisitor::operator()(double l, const arithmetic_tree::Operation& operation) const
{
    return (this->*operation.op)(l, boost::apply_visitor(*this, operation.r));
}

double ArithmeticVisitor::operator()(const arithmetic_tree::Expression& expression) const
{
    auto l = boost::apply_visitor(*this, expression.l);
    for (auto& op : expression.r)
        l = (*this)(l, op);
    return l;
}

} // namespace planner
