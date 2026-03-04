#ifndef IMPORTOVERWRITEMODEL_H
#define IMPORTOVERWRITEMODEL_H

#include "PlanModel.h"
#include <boost/container/flat_map.hpp>
#include <QAbstractListModel>

namespace planner {

class ImportOverwriteModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ImportOverwriteModel(const PlanModel::Plans& import_plans,
                                  const PlanModel::Plans& plans,
                                  QObject* parent = nullptr);
    int rowCount(const QModelIndex& /*parent*/ = {}) const override
    {
        return plans_for_overwrite.size();
    }
    QVariant data(const QModelIndex& index, int role) const override;

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        if (!index.isValid())
            return Qt::NoItemFlags;
        return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
    }

    boost::container::flat_map<QUuid, std::pair<const Plan*, bool>> plans_for_overwrite;
};

} // namespace planner

#endif // IMPORTOVERWRITEMODEL_H
