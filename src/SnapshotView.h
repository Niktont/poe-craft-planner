#ifndef SNAPSHOTVIEW_H
#define SNAPSHOTVIEW_H

#include <QTableView>

namespace planner {
class SnapshotsDialog;
class SnapshotModel;

class SnapshotView : public QTableView
{
    Q_OBJECT
public:
    explicit SnapshotView(SnapshotsDialog& snapshots_dialog, SnapshotModel& model);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void deleteSnapshot();

private:
    QAction* add_action;
    QAction* delete_action;

    SnapshotsDialog& snapshots_dialog;
    SnapshotModel* snapshotModel() const;
};

} // namespace planner

#endif // SNAPSHOTVIEW_H
