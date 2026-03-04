#ifndef STEPITEMDATA_H
#define STEPITEMDATA_H

#include <QJsonObject>
#include <QString>
#include <QUuid>

namespace planner {
class StepItemData
{
public:
    StepItemData() = default;
    StepItemData(const QJsonObject& item_o)
        : step_id{QUuid::fromString(item_o["step_id"].toStringView())}
    {}
    void toJson(QJsonObject& item_o) const { item_o["step_id"] = step_id.toString(); }

    QUuid step_id;
};
} // namespace planner
#endif // STEPITEMDATA_H
