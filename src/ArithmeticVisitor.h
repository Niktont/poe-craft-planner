#ifndef ARITHMETICVISITOR_H
#define ARITHMETICVISITOR_H

#include "ArithmeticExpression.h"
#include <cmath>

namespace planner {

class ArithmeticVisitor : public boost::static_visitor<double>
{
public:
    result_type operator()(double value) const { return value; };
    result_type operator()(const arithmetic_tree::Expression& expression) const;
    result_type operator()(double l, const arithmetic_tree::Operation& operation) const;

    double add(double l, double r) const { return l + r; };
    double sub(double l, double r) const { return l - r; };

    double multiply(double l, double r) const { return l * r; };
    double divide(double l, double r) const { return l / r; };

    double pow(double l, double r) const { return std::pow(l, r); };
};

} // namespace planner

#endif // ARITHMETICVISITOR_H
