#ifndef IMPORTEXCEPTION_H
#define IMPORTEXCEPTION_H

#include <exception>

namespace planner {
enum class ImportError {
    InvalidPlanId,
    InvalidTradeRequest,
};

struct ImportException : public std::exception
{
    ImportException(ImportError type)
        : type{type}
    {}
    ImportError type;
};
} // namespace planner

#endif // IMPORTEXCEPTION_H
