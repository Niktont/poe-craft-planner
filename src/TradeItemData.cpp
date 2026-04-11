#include "TradeItemData.h"
#include "TradeRequestCache.h"

namespace planner {

void TradeItemData::exportJson(QJsonObject& item_o, TradeRequestCache& cache) const
{
    toJson(item_o);
    cache.export_requests.emplace(request_key);
    if (name.isEmpty()) {
        if (auto it = cache.requestData(request_key); it != cache.cache.end())
            item_o["name"] = it->second.name();
    }
    if (!time)
        item_o["time"] = cache.time(*this).count();
}

} // namespace planner
