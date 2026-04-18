#include "CustomCalculation.h"
#include "CustomVisitor.h"
#include "Step.h"

namespace planner {

std::pair<bool, std::string::const_iterator> CustomCalculation::parseString(
    const std::string& text, custom_tree::Expression& tree)
{
    auto first = text.cbegin();
    bool result = qi::phrase_parse(first, text.cend(), parser, qi::space, tree);
    return {result, first};
}

std::pair<bool, std::string::const_iterator> CustomCalculation::parseString(const std::string& text,
                                                                            CustomCalcData& custom)
{
    if (text.empty()) {
        custom.tree.reset();
        return {false, text.cend()};
    }

    custom.tree.emplace();

    auto [result, first] = parseString(text, *custom.tree);
    if (!result || first != text.cend())
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

    return visitor(*custom.tree);
}

} // namespace planner
