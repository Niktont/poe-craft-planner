#include "SnapshotView.h"
#include "SnapshotModel.h"
#include "SnapshotsDialog.h"
#include <QContextMenuEvent>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>

namespace planner {

SnapshotView::SnapshotView(SnapshotsDialog& snapshots_dialog, SnapshotModel& model)
    : QTableView{&snapshots_dialog}
    , snapshots_dialog{snapshots_dialog}
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setWordWrap(false);
    setSelectionMode(ContiguousSelection);
    setSelectionBehavior(SelectRows);

    setModel(&model);

    auto header = horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);

    setFixedWidth(header->length() + verticalScrollBar()->sizeHint().width() + lineWidth() * 2);
    connect(header, &QHeaderView::sectionResized, this, [this] {
        setFixedWidth(horizontalHeader()->length() + verticalScrollBar()->sizeHint().width()
                      + lineWidth() * 2);
    });

    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setDefaultSectionSize(fontMetrics().height());
    verticalHeader()->hide();

    add_action = addAction(tr("Create"), this, [this] {
        this->snapshots_dialog.create_snapshot_dialog->open();
    });

    delete_action = addAction(tr("Delete"), this, &SnapshotView::deleteSnapshot);
    delete_action->setShortcuts({Qt::Key_Delete, Qt::ShiftModifier | Qt::Key_Delete});
    delete_action->setShortcutContext(Qt::WidgetShortcut);
}

void SnapshotView::contextMenuEvent(QContextMenuEvent* event)
{
    auto menu = new QMenu{this};
    menu->setAttribute(Qt::WA_DeleteOnClose);

    auto index = indexAt(event->pos());
    if (index.isValid()) {
        menu->addAction(add_action);
        menu->addSeparator();
        menu->addAction(delete_action);
    } else
        menu->addAction(add_action);

    menu->popup(event->globalPos());
}

void SnapshotView::deleteSnapshot()
{
    if (!selectionModel()->hasSelection())
        return;

    auto selection = selectionModel()->selection();

    auto modifiers = QGuiApplication::keyboardModifiers();
    bool delete_search = modifiers.testFlag(Qt::ShiftModifier);
    if (!delete_search) {
        QMessageBox msg{this};
        if (selection[0].top() == selection[0].bottom()) {
            msg.setWindowTitle(tr("Delete Snapshot"));
            msg.setText(tr("Delete this snapshot?"));
        } else {
            msg.setWindowTitle(tr("Delete Snapshots"));
            msg.setText(tr("Delete selected Snapshots?"));
        }
        msg.addButton(QMessageBox::Ok);
        msg.addButton(QMessageBox::Cancel);
        delete_search = msg.exec() == QMessageBox::Ok;
    }
    if (!delete_search)
        return;

    snapshotModel()->deleteSnapshots(selection[0].top(), selection[0].bottom());
}

SnapshotModel* SnapshotView::snapshotModel() const
{
    return static_cast<SnapshotModel*>(this->model());
}

} // namespace planner
