#include "ArithmeticCalculation.h"
#include <QString>

namespace planner {

std::optional<double> ArithmeticCalculation::calculate(const QString& expression_text) const
{
    if (expression_text.isEmpty())
        return {};

    auto std_string = expression_text.toStdString();
    arithmetic_tree::Expression tree;

    bool success = false;
    try {
        auto first = std_string.cbegin();
        success = qi::phrase_parse(first, std_string.cend(), parser, qi::space, tree);
        if (first != std_string.cend())
            success = false;
    } catch (ParseException&) {
    }

    return success ? visitor(tree) : std::optional<double>{};
}

} // namespace planner
