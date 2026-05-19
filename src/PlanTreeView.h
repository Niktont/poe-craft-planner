#ifndef PLANTREEVIEW_H
#define PLANTREEVIEW_H

#include "Game.h"
#include <QTreeView>

class QAction;

namespace planner {
class PlanModel;
class Plan;

class PlanTreeView : public QTreeView
{
    Q_OBJECT
public:
    PlanTreeView(QWidget* parent = nullptr);

    PlanModel* planModel();

    void selectPlan(const QUuid& plan_id);
    void selectPlan(Plan& plan);

    void setModel(QAbstractItemModel* model) override;

signals:
    void planSelected(planner::Plan& plan);
    void planWindowRequested(const QUuid& plan_id, planner::Game game);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void selectPlanOnClick(const QModelIndex& idx);
    void selectPlanOnCurrentChange(const QModelIndex& idx);

    void restoreItem();
    void deleteItem();

    void exportItem();

private:
    QAction* add_plan_action;
    QAction* add_folder_action;
    QAction* duplicate_action;
    QAction* copy_action;
    QAction* paste_action;

    QAction* update_action;

    QAction* save_action;
    QAction* restore_action;

    QAction* delete_action;

    QAction* export_clipboard_action;
    QAction* export_file_action;

    bool is_context_idx_valid{};
};

} // namespace planner

#endif // PLANTREEVIEW_H
