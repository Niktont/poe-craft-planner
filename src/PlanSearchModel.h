#ifndef PLANSEARCHMODEL_H
#define PLANSEARCHMODEL_H

#include <boost/container/flat_map.hpp>
#include <QAbstractTableModel>
#include <QUuid>

class QCompleter;

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

    void reset();
    void insertPlan(const Plan& plan);
    void removePlan(const QUuid& plan_id);

    void updatePath(const Plan& plan);

    QUuid planId(const QModelIndex& idx) const { return plans.nth(idx.row())->first; }

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;

    QVariant data(const QModelIndex& index, int role) const override;

private:
    boost::container::flat_map<QUuid, QString> plans;
    PlanModel* model() const;

    friend class PlanModel;
};

} // namespace planner

#endif // PLANSEARCHMODEL_H
