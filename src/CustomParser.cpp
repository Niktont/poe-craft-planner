#include "CustomParser.h"
#include "CustomVisitor.h"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/phoenix.hpp>

// clang-format off
BOOST_FUSION_ADAPT_STRUCT(planner::custom_tree::Item, (unsigned, row))

BOOST_FUSION_ADAPT_STRUCT(planner::custom_tree::NaryFunction,
                          (planner::custom_tree::NaryFun, fun)
                          (std::list<planner::custom_tree::Operand>, operands))

BOOST_FUSION_ADAPT_STRUCT(planner::custom_tree::Operation,
                          (planner::custom_tree::BinaryOp, op)
                          (planner::custom_tree::Operand, r))

BOOST_FUSION_ADAPT_STRUCT(planner::custom_tree::Expression,
                          (planner::custom_tree::Operand, l)
                          (std::list<planner::custom_tree::Operation>, r))
// clang-format on

namespace planner {

template<typename Iterator>
CustomGrammar<Iterator>::CustomGrammar()
    : CustomGrammar::base_type(expression)
{
    using qi::uint_;

    // clang-format off
    
    additive_op.add
        ("+", &CustomVisitor::add)
        ("-", &CustomVisitor::sub);

    min_max_op.add
        ("|", &CustomVisitor::min)
        ("&", &CustomVisitor::max);
    
    nary_fun.add
        ("min", &CustomVisitor::minN)
        ("max", &CustomVisitor::maxN);
    
    expression = min_max >> *(additive_op > min_max);
    
    min_max = primary >> *(min_max_op > min_max);
    
    nary = nary_fun > '(' > expression >> *(',' > expression) > ')';
    
    primary = item | ('(' > expression > ')') | nary;
    
    item = uint_;

    // clang-format on
}
template class CustomGrammar<std::string::const_iterator>;

} // namespace planner
