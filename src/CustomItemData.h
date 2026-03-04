#ifndef CUSTOMITEMDATA_H
#define CUSTOMITEMDATA_H
#include <QString>

#include "CurrencyCost.h"
#include "ItemTime.h"

namespace planner {

class CustomItemData
{
public:
    CustomItemData() = default;
    CustomItemData(const QJsonObject& item_o, const ExchangeRequestCache& cache)
        : name{item_o["name"].toString()}
        , link{item_o["custom_link"].toString()}
        , cost{CurrencyCost::fromJson(item_o["custom_cost"].toObject(), cache)}
        , gold{item_o["custom_gold"].toDouble()}
        , time{item_o["time"].toDouble()}
    {}

    void toJson(QJsonObject& item_o) const
    {
        item_o["name"] = name;
        item_o["custom_link"] = link;
        item_o["custom_cost"] = cost.toJson();
        item_o["custom_gold"] = gold;
        item_o["time"] = time.count();
    }
    QString name;
    QString link;

    CurrencyCost cost;
    double gold{};

    ItemTime time{};
};

} // namespace planner
#endif // CUSTOMITEMDATA_H
