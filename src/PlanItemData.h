#ifndef PLANITEMDATA_H
#define PLANITEMDATA_H

#include <QJsonObject>
#include <QUuid>

namespace planner {
class PlanItemData
{
public:
    PlanItemData() = default;
    PlanItemData(const QJsonObject& item_o)
        : name{item_o["name"].toString()}
        , plan_id{QUuid::fromString(item_o["plan_id"].toStringView())}
    {}
    void toJson(QJsonObject& item_o) const
    {
        item_o["name"] = name;
        item_o["plan_id"] = plan_id.toString();
    }

    QString name;
    QUuid plan_id;
};
} // namespace planner

#endif // PLANITEMDATA_H
