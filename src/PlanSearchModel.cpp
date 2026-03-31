#include "PlanSearchModel.h"
#include "PlanModel.h"
#include <QCompleter>
#include <QSortFilterProxyModel>

namespace planner {

PlanSearchModel::PlanSearchModel(PlanModel& model)
    : QAbstractTableModel{&model}
    , completer{new QCompleter{{}, this}}
    , proxy_model{new QSortFilterProxyModel{this}}
{
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionColumn(static_cast<int>(PlanSearchColumn::Name));
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCompletionRole(Qt::DisplayRole);
    completer->setFilterMode(Qt::MatchContains);
    completer->setModel(this);

    proxy_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy_model->setFilterKeyColumn(static_cast<int>(PlanSearchColumn::Name));
    proxy_model->setDynamicSortFilter(false);
    proxy_model->setSourceModel(this);
}

void PlanSearchModel::reset()
{
    beginResetModel();
    plans.clear();
    for (auto& [id, plan] : model()->plans)
        plans.try_emplace(id, plan.item()->shortPath());
    endResetModel();
}

void PlanSearchModel::insertPlan(const Plan& plan)
{
    auto it = plans.lower_bound(plan.id());
    if (it == plans.end() || it->first != plan.id()) {
        auto pos = plans.index_of(it);
        beginInsertRows({}, pos, pos);
        plans.try_emplace(it, plan.id(), plan.item()->shortPath());
        endInsertRows();
    }
}

void PlanSearchModel::removePlan(const QUuid& plan_id)
{
    auto it = plans.find(plan_id);
    if (it == plans.end())
        return;

    auto pos = plans.index_of(it);
    beginRemoveRows({}, pos, pos);
    plans.erase(it);
    endRemoveRows();
}

void PlanSearchModel::updatePath(const Plan& plan)
{
    auto it = plans.find(plan.id());
    if (it == plans.end())
        return;

    it->second = plan.item()->shortPath();
    auto idx = index(plans.index_of(it), static_cast<int>(PlanSearchColumn::Name));
    emit dataChanged(idx, idx, {Qt::DisplayRole});
}

QVariant PlanSearchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation == Qt::Vertical)
        return {};

    auto col = static_cast<PlanSearchColumn>(section);
    switch (col) {
    case PlanSearchColumn::Name:
        return tr("Name");
    }
    return {};
}

int PlanSearchModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return plans.size();
}

int PlanSearchModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(PlanSearchColumn::last) + 1;
}

QVariant PlanSearchModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    auto col = static_cast<PlanSearchColumn>(index.column());
    switch (col) {
    case PlanSearchColumn::Name:
        switch (role) {
        case Qt::DisplayRole:
            return plans.nth(index.row())->second;
        case Qt::ToolTipRole: {
            if (auto plan_it = model()->plans.find(plans.nth(index.row())->first);
                plan_it != model()->plans.end())
                return plan_it->second.item()->path();
            return {};
        }
        }
        return {};
    }
    return {};
}

PlanModel* PlanSearchModel::model() const
{
    return static_cast<PlanModel*>(parent());
}

} // namespace planner
