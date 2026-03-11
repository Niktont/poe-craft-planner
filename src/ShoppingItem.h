#ifndef SHOPPINGITEM_H
#define SHOPPINGITEM_H

#include "CurrencyCost.h"
#include "TradeRequestKey.h"
#include <variant>

namespace planner {

class ShoppingItem
{
public:
    double amount{};

    struct Trade
    {
        QString name;
        TradeRequestKey request;
    };
    std::variant<Currency, Trade> data;

    Currency* exchange() { return std::get_if<Currency>(&data); }
    const Currency* exchange() const { return std::get_if<Currency>(&data); }

    Trade* trade() { return std::get_if<Trade>(&data); }
    const Trade* trade() const { return std::get_if<Trade>(&data); }
};

} // namespace planner

#endif // SHOPPINGITEM_H
