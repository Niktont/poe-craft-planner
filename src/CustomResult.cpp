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

void CustomResult::min(CustomResult& min, CustomResult&& o)
{
    if (min.cost && o.cost) {
        if (*o.cost < *min.cost) {
            min.cost = o.cost;
            min.used_items = std::move(o.used_items);
        }
    } else if (o.cost)
        min = std::move(o);
}

void CustomResult::max(CustomResult& max, CustomResult&& o)
{
    if (max.cost && o.cost) {
        if (*max.cost < *o.cost) {
            max.cost = o.cost;
            max.used_items = std::move(o.used_items);
        }
    } else if (o.cost)
        max = std::move(o);
}

} // namespace planner
