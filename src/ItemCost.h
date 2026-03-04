#ifndef ITEMCOST_H
#define ITEMCOST_H

#include "CurrencyCost.h"
#include "ItemTime.h"
#include <QJsonObject>

namespace planner {
class ExchangeRequestCache;

class ItemCost
{
public:
    CurrencyCost cost_in_primary;
    double gold{};
    ItemTime time{};

    bool isValid() const
    {
        return cost_in_primary.value != 0.0 && cost_in_primary.currency.isValid();
    }
    QJsonObject toJson() const;
    static ItemCost fromJson(const QJsonObject& cost_o, const ExchangeRequestCache& cache);
    ItemCost& operator+=(const ItemCost& r)
    {
        cost_in_primary += r.cost_in_primary;
        gold += r.gold;
        time += r.time;
        return *this;
    }
    ItemCost& operator-=(const ItemCost& r)
    {
        cost_in_primary -= r.cost_in_primary;
        gold -= r.gold;
        time -= r.time;
        return *this;
    }
    friend ItemCost operator+(ItemCost l, const ItemCost& r)
    {
        l += r;
        return l;
    }
    friend ItemCost operator-(ItemCost l, const ItemCost& r)
    {
        l -= r;
        return l;
    }
    friend bool operator<(const ItemCost& l, const ItemCost& r)
    {
        return l.cost_in_primary < r.cost_in_primary;
    }
};

} // namespace planner

#endif // ITEMCOST_H
