#ifndef PLANMODEL_H
#define PLANMODEL_H

#include "Game.h"
#include "HashFunctions.h"
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <boost/unordered/unordered_node_map.hpp>
#include <memory>
#include <QAbstractItemModel>
#include <QUuid>

class QSqlQuery;

namespace planner {
class ImportOverwriteModel;
class PlanSearchModel;
class Plan;
class PlanItem;

enum class PlanItemColumn {
    Name,
    Cost,

    last = Cost,
};

class PlanModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    PlanModel(Game game, QObject* parent = nullptr);
    ~PlanModel() noexcept;

    using Plans = boost::unordered::unordered_node_map<QUuid, Plan>;
    Plans plans;

    const Game game;
    PlanSearchModel* search_model;

    QModelIndex insertPlan(const QModelIndex& dest = {});
    QModelIndex insertFolder(const QModelIndex& dest = {});

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& /*parent*/ = {}) const override
    {
        return static_cast<int>(PlanItemColumn::last) + 1;
    }

    bool moveRows(const QModelIndex& source_idx,
                  int source_row,
                  int count,
                  const QModelIndex& dest_idx,
                  int dest_row) override;
    bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

    static const QString move_mime_poe1;
    static const QString move_mime_poe2;
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

    static std::vector<PlanItem*> decodePlanItemsMime(Game game, const QMimeData& data);
    static std::vector<Plan*> decodeMimeToPlans(Game game, const QMimeData& data);

    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    PlanItem* internalPtr(const QModelIndex& index = {}) const;
    const PlanItem* constInternalPtr(const QModelIndex& index = {}) const
    {
        return static_cast<const PlanItem*>(internalPtr(index));
    }

    bool readDatabase();

    bool importItem(const QJsonObject& export_o, QWidget* dialog_parent = nullptr);
    QString exportFileName(const QModelIndex& index) const;
    QJsonDocument exportItem(const QModelIndex& index) const;

    void savePlan(const PlanItem& item);
    void savePlan(const QModelIndex& index);
    void saveAllPlans();

    bool isNewPlan(const QModelIndex& index) const;
    bool canRestorePlan(const QModelIndex& index) const;
    void restorePlan(const QModelIndex& index);

    QModelIndex duplicateItem(const QModelIndex& index);

    void copyItem(const QModelIndex& idx) const;
    bool haveCopy() const { return item_copy_state != nullptr; }
    QModelIndex pasteItem(const QModelIndex& idx);

    bool haveUnsavedPlans() const { return !changed_plans.empty(); }

    void updateCost(const QModelIndex& index);

signals:
    void planRenamed(const planner::Plan& plan);
    void planUpdated(planner::Plan& updated_plan);

    void descriptionsNeeded(planner::Game game, const planner::Plan* target_plan) const;
    void currentNeedsReselecting(planner::Game game) const;

private:
    std::unique_ptr<PlanItem> root;

    QString base_plan_name;
    QString base_folder_name;

    boost::unordered::unordered_flat_set<const PlanItem*> changed_folders;
    boost::unordered::unordered_flat_map<const PlanItem*, bool> changed_plans;

    mutable PlanItem* item_copy_state{};

    QModelIndex insertCopy(const QModelIndex& parent, int row, const PlanItem& item);

    void setPlanChanged(const PlanItem& item);

    void saveName(const PlanItem& item) const;

    void saveFoldersTransaction();
    void saveFolders(QSqlQuery& save_query);
    void savePlanItem(const PlanItem& item, QSqlQuery& save_query);

    bool gatherDependencies(QJsonObject& export_o,
                            QJsonObject& item_o,
                            std::vector<QUuid>& dependencies) const;

    bool handleOverwrite(QWidget* dialog_parent);
    Plans import_plans;

    friend class PlanItem;
};

} // namespace planner
#endif // PLANMODEL_H
