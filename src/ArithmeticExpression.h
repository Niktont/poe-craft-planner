#ifndef ARITHMETICEXPRESSION_H
#define ARITHMETICEXPRESSION_H

#include <boost/variant.hpp>

namespace planner {
class ArithmeticVisitor;

namespace arithmetic_tree {
class Expression;

using Operand = boost::variant<double, boost::recursive_wrapper<Expression>>;

using BinaryOp = double (ArithmeticVisitor::*)(double, double) const;
class Operation
{
public:
    BinaryOp op;
    Operand r;
};

class Expression
{
public:
    Operand l;
    std::list<Operation> r;
};

} // namespace arithmetic_tree
} // namespace planner
#endif // ARITHMETICEXPRESSION_H
