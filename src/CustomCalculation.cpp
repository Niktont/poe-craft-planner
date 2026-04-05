#include "CustomCalculation.h"
#include "CustomVisitor.h"
#include "Step.h"

namespace planner {

std::pair<bool, std::string::const_iterator> CustomCalculation::parseString(const std::string& text,
                                                                            CustomCalcData& custom)
{
    if (text.empty()) {
        custom.tree.reset();
        return {false, text.cend()};
    }

    custom.tree.emplace();

    auto first = text.cbegin();
    auto last = text.cend();
    bool result = qi::phrase_parse(first, last, parser, qi::space, *custom.tree);
    if (!result || first != last)
        custom.tree.reset();

    return {result, first};
}

std::optional<CustomResult> CustomCalculation::calculate(CustomCalcData& custom)
{
    if (!custom.tree) {
        auto std_string = custom.text.toStdString();
        auto result = parseString(std_string, custom);
        if (!result.first || result.second != std_string.cend())
            return {};
    }

    visitor.not_used_items.clear();
    return visitor(*custom.tree);
}

} // namespace planner
