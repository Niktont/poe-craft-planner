#ifndef CUSTOMEXPRESSION_H
#define CUSTOMEXPRESSION_H

#include "CustomResult.h"
#include <boost/variant.hpp>

namespace planner {
class CustomVisitor;

namespace custom_tree {

class Item
{
public:
    unsigned row{};
    Item() = default;
    Item(unsigned row)
        : row{row}
    {}
};

class Expression;
class NaryFunction;

using Operand = boost::
    variant<Item, boost::recursive_wrapper<Expression>, boost::recursive_wrapper<NaryFunction>>;

using NaryFun = CustomResult (CustomVisitor::*)(const std::list<Operand>&);
class NaryFunction
{
public:
    NaryFun fun;
    std::list<Operand> operands;
};

using BinaryOp = void (CustomVisitor::*)(CustomResult&, CustomResult&&);
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

} // namespace custom_tree
} // namespace planner
#endif // CUSTOMEXPRESSION_H
