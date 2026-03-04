#include "PlanTreeView.h"
#include "PlanModel.h"
#include <QContextMenuEvent>
#include <QMenu>
#include <QMessageBox>

namespace planner {

PlanTreeView::PlanTreeView(PlanModel& model, QWidget* parent)
    : QTreeView{parent}
{
    setModel(&model);

    setDragDropMode(QAbstractItemView::DragDrop);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setHeaderHidden(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    add_plan_action = addAction(tr("New Plan"), this, [this] {
        auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
        planModel()->insertPlan(current);
        contextIndex.reset();
    });
    add_folder_action = addAction(tr("New Folder"), this, [this] {
        auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
        planModel()->insertFolder(current);
        contextIndex.reset();
    });
    duplicate_action = addAction(tr("Duplicate"), {Qt::CTRL | Qt::Key_D}, this, [this] {
        auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
        planModel()->duplicateItem(current);
        contextIndex.reset();
    });
    duplicate_action->setShortcutContext(Qt::WidgetShortcut);

    save_action = addAction(tr("Save"), this, [this] {
        auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
        planModel()->savePlan(current);
        contextIndex.reset();
    });
    restore_action = addAction(tr("Restore"), this, [this] {
        auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
        planModel()->restorePlan(current);
        contextIndex.reset();
    });

    delete_action = addAction(tr("Delete"),
                              this,
                              &PlanTreeView::deleteItem);
    delete_action->setShortcuts({{Qt::Key_Delete}, {Qt::SHIFT | Qt::Key_Delete}});
    delete_action->setShortcutContext(Qt::WidgetShortcut);

    export_clipboard_action = addAction(tr("Export (Clipboard)"), this, [this] {
        auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
        planModel()->exportItem(current, true);
        contextIndex.reset();
    });

    export_file_action = addAction(tr("Export (File)"), this, [this] {
        auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
        planModel()->exportItem(current, false);
        contextIndex.reset();
    });
}

PlanModel* PlanTreeView::planModel()
{
    return static_cast<PlanModel*>(model());
}

void PlanTreeView::contextMenuEvent(QContextMenuEvent* event)
{
    auto menu = new QMenu{this};
    menu->setAttribute(Qt::WA_DeleteOnClose);

    menu->addAction(add_plan_action);
    menu->addAction(add_folder_action);

    contextIndex = indexAt(event->pos());
    if (contextIndex->isValid()) {
        menu->addAction(duplicate_action);

        auto item = planModel()->internalPtr(*contextIndex);
        if (!item->isFolder() && item->plan()->is_changed) {
            menu->addSeparator();
            menu->addAction(save_action);
            if (!planModel()->isNewPlan(*contextIndex))
                menu->addAction(restore_action);
        }
        menu->addSeparator();
        menu->addAction(delete_action);
    }
    menu->addSeparator();
    menu->addAction(export_clipboard_action);
    menu->addAction(export_file_action);

    menu->popup(event->globalPos());
}

void PlanTreeView::keyPressEvent(QKeyEvent* event)
{
    if (delete_action->shortcut().matches(event->keyCombination())) {
        delete_action->trigger();
    } else
        QTreeView::keyPressEvent(event);
}

void PlanTreeView::deleteItem()
{
    auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
    if (!current.isValid())
        return;
    auto item = planModel()->internalPtr(current);
    auto modifiers = QGuiApplication::keyboardModifiers();

    bool delete_item = (modifiers & Qt::ShiftModifier)
                       || (item->isFolder() && item->childCount() == 0);
    if (!delete_item) {
        QMessageBox msg;
        if (item->isFolder())
            msg.setWindowTitle(tr("Delete Folder"));
        else
            msg.setWindowTitle(tr("Delete Plan"));
        msg.setText(tr("Delete \"%1\"?").arg(item->name()));
        msg.addButton(QMessageBox::Ok);
        msg.addButton(QMessageBox::Cancel);
        delete_item = msg.exec() == QMessageBox::Ok;
    }
    if (delete_item)
        planModel()->removeRows(current.row(), 1, current.parent());

    contextIndex.reset();
}

} // namespace planner
