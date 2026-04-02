#ifndef CUSTOMPARSER_H
#define CUSTOMPARSER_H

#include "CustomExpression.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;

namespace planner {

template<typename Iterator>
class CustomGrammar : public qi::grammar<Iterator, custom_tree::Expression(), qi::space_type>
{
public:
    CustomGrammar();

    qi::symbols<typename std::iterator_traits<Iterator>::value_type, custom_tree::BinaryOp>
        additive_op;
    qi::symbols<typename std::iterator_traits<Iterator>::value_type, custom_tree::BinaryOp>
        min_max_op;
    qi::symbols<typename std::iterator_traits<Iterator>::value_type, custom_tree::NaryFun> nary_fun;

    qi::rule<Iterator, custom_tree::Expression(), qi::space_type> expression;
    qi::rule<Iterator, custom_tree::Expression(), qi::space_type> min_max;
    qi::rule<Iterator, custom_tree::NaryFunction(), qi::space_type> nary;
    qi::rule<Iterator, custom_tree::Operand(), qi::space_type> primary;
    qi::rule<Iterator, custom_tree::Item(), qi::space_type> item;
};
using CustomParser = CustomGrammar<std::string::const_iterator>;
using ParseException = qi::expectation_failure<std::string::const_iterator>;

} // namespace planner

#endif // CUSTOMPARSER_H
