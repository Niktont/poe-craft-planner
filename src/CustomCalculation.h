#ifndef CUSTOMCALCULATION_H
#define CUSTOMCALCULATION_H

#include "CustomParser.h"
#include "CustomVisitor.h"
#include <QString>

namespace planner {
class CustomCalcData;

class CustomCalculation
{
public:
    CustomParser parser;
    CustomVisitor visitor;

    std::pair<bool, std::string::const_iterator> parseString(const std::string& text,
                                                             custom_tree::Expression& tree);
    std::pair<bool, std::string::const_iterator> parseString(const std::string& text,
                                                             CustomCalcData& custom);
    std::optional<CustomResult> calculate(CustomCalcData& custom);
};

} // namespace planner

#endif // CUSTOMCALCULATION_H
