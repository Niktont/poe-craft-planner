#include "ArithmeticParser.h"
#include "ArithmeticVisitor.h"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/phoenix.hpp>

// clang-format off
BOOST_FUSION_ADAPT_STRUCT(planner::arithmetic_tree::Operation,
                          (planner::arithmetic_tree::BinaryOp, op)
                          (planner::arithmetic_tree::Operand, r))

BOOST_FUSION_ADAPT_STRUCT(planner::arithmetic_tree::Expression,
                          (planner::arithmetic_tree::Operand, l)
                          (std::list<planner::arithmetic_tree::Operation>, r))
// clang-format on

namespace planner {

template<typename Iterator>
ArithmeticGrammar<Iterator>::ArithmeticGrammar()
    : ArithmeticGrammar::base_type{expression}
{
    using qi::double_;

    // clang-format off
    
    additive_op.add
        ("+", &ArithmeticVisitor::add)
        ("-", &ArithmeticVisitor::sub);
    
    multiplicative_op.add
        ("*", &ArithmeticVisitor::multiply)
        ("/", &ArithmeticVisitor::divide);

    pow_op.add
        ("^", &ArithmeticVisitor::pow);
    
    expression = multiplicative >> *(additive_op > multiplicative);

    multiplicative = pow >> *(multiplicative_op > pow);

    pow = primary >> *(pow_op > pow);

    primary = double_ | ('(' > expression > ')');

    // clang-format on
}

template class ArithmeticGrammar<std::string::const_iterator>;

} // namespace planner
