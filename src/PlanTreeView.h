#ifndef PLANTREEVIEW_H
#define PLANTREEVIEW_H

#include <QTreeView>

class QAction;

namespace planner {
class PlanModel;

class PlanTreeView : public QTreeView
{
    Q_OBJECT
public:
    PlanTreeView(PlanModel& model, QWidget* parent = nullptr);

    PlanModel* planModel();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void deleteItem();

private:
    QAction* add_plan_action;
    QAction* add_folder_action;
    QAction* duplicate_action;
    QAction* save_action;
    QAction* restore_action;
    QAction* delete_action;
    QAction* export_clipboard_action;
    QAction* export_file_action;

    std::optional<QModelIndex> contextIndex;
};

} // namespace planner

#endif // PLANTREEVIEW_H
