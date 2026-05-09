#ifndef ARITHMETICPARSER_H
#define ARITHMETICPARSER_H

#include "ArithmeticExpression.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;

namespace planner {

template<typename Iterator>
class ArithmeticGrammar
    : public qi::grammar<Iterator, arithmetic_tree::Expression(), qi::space_type>
{
public:
    ArithmeticGrammar();

    qi::symbols<typename std::iterator_traits<Iterator>::value_type, arithmetic_tree::BinaryOp>
        additive_op;
    qi::symbols<typename std::iterator_traits<Iterator>::value_type, arithmetic_tree::BinaryOp>
        multiplicative_op;
    qi::symbols<typename std::iterator_traits<Iterator>::value_type, arithmetic_tree::BinaryOp>
        pow_op;

    qi::rule<Iterator, arithmetic_tree::Expression(), qi::space_type> expression;
    qi::rule<Iterator, arithmetic_tree::Expression(), qi::space_type> multiplicative;
    qi::rule<Iterator, arithmetic_tree::Expression(), qi::space_type> pow;
    qi::rule<Iterator, arithmetic_tree::Operand(), qi::space_type> primary;
};

using ArithmeticParser = ArithmeticGrammar<std::string::const_iterator>;
using ParseException = qi::expectation_failure<std::string::const_iterator>;

} // namespace planner

#endif // ARITHMETICPARSER_H
