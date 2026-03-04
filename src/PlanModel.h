#ifndef PLANMODEL_H
#define PLANMODEL_H

#include "Plan.h"
#include "PlanItem.h"
#include "HashFunctions.h"
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <boost/unordered/unordered_node_map.hpp>
#include <memory>
#include <QAbstractItemModel>
#include <QUuid>

namespace planner {

enum class PlanItemColumn {
    Name,
};
class MainWindow;
class ImportOverwriteModel;

class PlanModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    PlanModel(Game game, MainWindow& mw);
    ~PlanModel()
    {
        saveFoldersTransaction();
    }
    using Plans = boost::unordered::unordered_node_map<QUuid, Plan>;
    Plans plans;
    const Game game;

    QModelIndex insertPlan(const QModelIndex& dest = {});
    QModelIndex insertFolder(const QModelIndex& dest = {});

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;

    bool moveRows(const QModelIndex& source_idx,
                  int source_row,
                  int count,
                  const QModelIndex& dest_idx,
                  int dest_row) override;
    bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

    Qt::DropActions supportedDropActions() const override { return Qt::CopyAction; }
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool canDropMimeData(const QMimeData* data,
                         Qt::DropAction action,
                         int row,
                         int column,
                         const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex& parent) override;

    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    PlanItem* internalPtr(const QModelIndex& index = {}) const;
    const PlanItem* constInternalPtr(const QModelIndex& index = {}) const;

    bool readDatabase();

    void importItem(const QJsonObject& export_o);
    void exportItem(const QModelIndex& index, bool to_clipboard) const;

    void savePlan(PlanItem* item);
    void savePlan(const QModelIndex& index);
    void saveAllPlans();

    void restorePlan(const QModelIndex& index);
    void duplicateItem(const QModelIndex& index);
    bool isNewPlan(const QModelIndex& index) const;
    bool haveUnsavedPlans() const { return !changed_plans.empty(); }

    MainWindow* mw() const;

signals:
    void planRenamed(planner::Plan* plan);
    void planUpdated(planner::Plan* new_plan, const planner::Plan* old_plan);

private:
    std::unique_ptr<PlanItem> root;

    QString base_plan_name;
    QString base_folder_name;

    boost::unordered::unordered_flat_set<PlanItem*> changed_folders;
    boost::unordered::unordered_flat_map<PlanItem*, bool> changed_plans;

    void setPlanChanged(PlanItem* item);

    void saveName(PlanItem& item);

    void saveFoldersTransaction();
    void saveFolders(QSqlQuery& save_query);
    void savePlanItem(PlanItem* item, QSqlQuery& save_query);

    Plans import_plans;

    friend class PlanItem;
};

} // namespace planner
#endif // PLANMODEL_H
