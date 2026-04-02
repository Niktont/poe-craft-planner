#include "CustomResult.h"

namespace planner {

CustomResult& CustomResult::operator+=(CustomResult&& o)
{
    if (!o.cost)
        return *this;

    if (!cost)
        cost = std::move(o.cost);
    else
        *cost += *o.cost;
    used_items.merge(o.used_items);

    return *this;
}

CustomResult& CustomResult::operator-=(CustomResult&& o)
{
    if (!o.cost)
        return *this;

    if (!cost)
        cost = std::move(o.cost);
    else
        *cost -= *o.cost;
    used_items.merge(o.used_items);

    return *this;
}

void CustomResult::min(CustomResult& min, CustomResult&& o, std::set<unsigned>& not_used_items)
{
    if (min.cost && o.cost) {
        if (*o.cost < *min.cost) {
            min.cost = o.cost;
            not_used_items.merge(min.used_items);
            min.used_items = std::move(o.used_items);
        }
    } else if (o.cost)
        min = std::move(o);
}

void CustomResult::max(CustomResult& max, CustomResult&& o, std::set<unsigned>& not_used_items)
{
    if (max.cost && o.cost) {
        if (*max.cost < *o.cost) {
            max.cost = o.cost;
            not_used_items.merge(max.used_items);
            max.used_items = std::move(o.used_items);
        }
    } else if (o.cost)
        max = std::move(o);
}

} // namespace planner
