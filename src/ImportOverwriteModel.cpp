#include "ImportOverwriteModel.h"

namespace planner {

ImportOverwriteModel::ImportOverwriteModel(const PlanModel::Plans& import_plans,
                                           const PlanModel::Plans& plans,
                                           QObject* parent)
    : QAbstractListModel{parent}
{
    for (auto& p : import_plans) {
        if (auto it = plans.find(p.first); it != plans.end())
            plans_for_overwrite.try_emplace(p.first, &it->second, true);
    }
}

QVariant ImportOverwriteModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() > rowCount())
        return {};
    switch (role) {
    case Qt::DisplayRole: {
        auto plan = plans_for_overwrite.nth(index.row())->second.first;
        auto plan_item = plan->item();
        auto parent = plan_item->parent();
        if (parent && !parent->name().isEmpty())
            return {parent->name() % '/' % plan->name};
        return plan->name;
    }
    case Qt::CheckStateRole:
        if (plans_for_overwrite.nth(index.row())->second.second)
            return Qt::Checked;
        return Qt::Unchecked;
    }
    return {};
}

bool ImportOverwriteModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() > rowCount())
        return false;
    if (role == Qt::CheckStateRole) {
        plans_for_overwrite.nth(index.row())->second.second = value.value<Qt::CheckState>()
                                                              == Qt::Checked;
        emit dataChanged(index, index, {role});
        return true;
    }
    return false;
}

} // namespace planner
