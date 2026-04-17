#ifndef PLANTREEVIEW_H
#define PLANTREEVIEW_H

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

signals:
    void planSelected(planner::PlanModel& model, planner::Plan& plan);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void restoreItem();
    void deleteItem();
    void exportToFile();

private:
    QAction* add_plan_action;
    QAction* add_folder_action;
    QAction* duplicate_action;
    QAction* copy_action;
    QAction* paste_action;

    QAction* save_action;
    QAction* restore_action;

    QAction* delete_action;
    QAction* export_clipboard_action;
    QAction* export_file_action;

    std::optional<QModelIndex> context_index;
};

} // namespace planner

#endif // PLANTREEVIEW_H
