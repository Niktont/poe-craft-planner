#ifndef PLANSEARCHMODEL_H
#define PLANSEARCHMODEL_H

#include <boost/container/flat_map.hpp>
#include <QAbstractTableModel>
#include <QUuid>

class QCompleter;
class QSortFilterProxyModel;

namespace planner {
class PlanModel;
class Plan;

enum class PlanSearchColumn {
    Name,

    last = Name,
};

class PlanSearchModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit PlanSearchModel(PlanModel& model);

    QCompleter* completer;
    QSortFilterProxyModel* proxy_model;

    void reset();
    void insertPlan(const Plan& plan);
    void removePlan(const QUuid& plan_id);

    void updatePath(const Plan& plan);

    const QUuid& planId(const QModelIndex& idx) const { return plans.nth(idx.row())->first; }

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;

    QVariant data(const QModelIndex& index, int role) const override;

    PlanModel* model() const;

private:
    boost::container::flat_map<QUuid, QString> plans;

    friend class PlanModel;
};

} // namespace planner

#endif // PLANSEARCHMODEL_H
