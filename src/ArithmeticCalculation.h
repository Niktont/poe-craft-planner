#ifndef ARITHMETICCALCULATION_H
#define ARITHMETICCALCULATION_H

#include "ArithmeticParser.h"
#include "ArithmeticVisitor.h"
#include <optional>

class QString;

namespace planner {

class ArithmeticCalculation
{
public:
    ArithmeticParser parser;
    ArithmeticVisitor visitor;

    std::optional<double> calculate(const QString& expression_text) const;
};

} // namespace planner

#endif // ARITHMETICCALCULATION_H
