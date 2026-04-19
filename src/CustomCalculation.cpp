#include "CustomCalculation.h"
#include "CustomVisitor.h"
#include "Step.h"

namespace planner {

std::pair<bool, std::string::const_iterator> CustomCalculation::parseString(
    const std::string& text, custom_tree::Expression& tree) const
{
    bool success = false;
    auto first = text.cbegin();
    try {
        success = qi::phrase_parse(first, text.cend(), parser, qi::space, tree);
        if (first != text.cend())
            success = false;
    } catch (ParseException& e) {
        first = e.first;
    }
    return {success, first};
}

std::pair<bool, std::string::const_iterator> CustomCalculation::parseString(
    const std::string& text, CustomCalcData& custom) const
{
    if (text.empty()) {
        custom.tree.reset();
        return {false, text.cend()};
    }

    custom.tree.emplace();

    auto [success, consumed] = parseString(text, *custom.tree);
    if (!success)
        custom.tree.reset();

    return {success, consumed};
}

std::optional<CustomResult> CustomCalculation::calculate(CustomCalcData& custom)
{
    if (!custom.tree) {
        auto std_string = custom.text.toStdString();
        if (!parseString(std_string, custom).first)
            return {};
    }

    return visitor(*custom.tree);
}

} // namespace planner
