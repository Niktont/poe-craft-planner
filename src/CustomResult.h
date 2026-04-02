#ifndef CUSTOMRESULT_H
#define CUSTOMRESULT_H

#include "ItemCost.h"
#include <optional>
#include <set>

namespace planner {

class CustomResult
{
public:
    std::optional<ItemCost> cost;
    std::set<unsigned> used_items;

    CustomResult& operator+=(CustomResult&& o);
    CustomResult& operator-=(CustomResult&& o);

    static void min(CustomResult& min, CustomResult&& o, std::set<unsigned>& not_used_items);
    static void max(CustomResult& max, CustomResult&& o, std::set<unsigned>& not_used_items);
};

} // namespace planner

#endif // CUSTOMRESULT_H
