#include "PlanTreeView.h"
#include "PlanModel.h"
#include <QContextMenuEvent>
#include <QHeaderView>
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

    header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    add_plan_action = addAction(tr("New Plan"), this, [this] {
        auto current = context_index ? *context_index : selectionModel()->currentIndex();
        setCurrentIndex(planModel()->insertPlan(current));
        context_index.reset();
    });
    add_folder_action = addAction(tr("New Folder"), this, [this] {
        auto current = context_index ? *context_index : selectionModel()->currentIndex();
        setCurrentIndex(planModel()->insertFolder(current));
        context_index.reset();
    });
    duplicate_action = addAction(tr("Duplicate"), {Qt::ControlModifier | Qt::Key_D}, this, [this] {
        auto copy_index = planModel()->duplicateItem(selectionModel()->currentIndex());
        if (copy_index.isValid())
            setCurrentIndex(copy_index);
    });
    duplicate_action->setShortcutContext(Qt::WidgetShortcut);

    copy_action = addAction(tr("Copy Reference"), this, [this] {
        planModel()->copyItem(selectionModel()->currentIndex());
    });

    paste_action = addAction(tr("Paste"), this, [this] {
        auto current = context_index ? *context_index : selectionModel()->currentIndex();
        auto copy_index = planModel()->pasteItem(current);
        if (copy_index.isValid())
            setCurrentIndex(copy_index);
        context_index.reset();
    });

    save_action = addAction(tr("Save"), this, [this] {
        planModel()->savePlan(selectionModel()->currentIndex());
    });
    restore_action = addAction(tr("Restore"), this, &PlanTreeView::restoreItem);

    delete_action = addAction(tr("Delete"),
                              this,
                              &PlanTreeView::deleteItem);
    delete_action->setShortcuts({{Qt::Key_Delete}, {Qt::ShiftModifier | Qt::Key_Delete}});
    delete_action->setShortcutContext(Qt::WidgetShortcut);

    export_clipboard_action = addAction(tr("Export (Clipboard)"), this, [this] {
        planModel()->exportItem(selectionModel()->currentIndex(), true);
    });

    export_file_action = addAction(tr("Export (File)"), this, [this] {
        planModel()->exportItem(selectionModel()->currentIndex(), false);
    });
}

PlanModel* PlanTreeView::planModel()
{
    return static_cast<PlanModel*>(model());
}

void PlanTreeView::selectPlan(const QUuid& plan_id)
{
    if (auto it = planModel()->plans.find(plan_id); it != planModel()->plans.end())
        selectPlan(it->second);
}

void PlanTreeView::selectPlan(Plan& plan)
{
    auto idx = plan.item()->index();
    if (idx.internalId() != currentIndex().internalId())
        setCurrentIndex(idx);
    else
        emit planSelected(*planModel(), plan);
}

void PlanTreeView::contextMenuEvent(QContextMenuEvent* event)
{
    auto menu = new QMenu{this};
    menu->setAttribute(Qt::WA_DeleteOnClose);

    menu->addAction(add_plan_action);
    menu->addAction(add_folder_action);

    context_index = indexAt(event->pos());
    if (context_index->isValid()) {
        menu->addAction(duplicate_action);
        menu->addAction(copy_action);
        if (planModel()->haveCopy())
            menu->addAction(paste_action);

        auto item = planModel()->internalPtr(*context_index);
        if (!item->isFolder() && item->plan()->is_changed) {
            menu->addSeparator();
            menu->addAction(save_action);
            if (planModel()->canRestorePlan(*context_index))
                menu->addAction(restore_action);
        }
        menu->addSeparator();
        menu->addAction(delete_action);
    } else if (planModel()->haveCopy())
        menu->addAction(paste_action);

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

void PlanTreeView::restoreItem()
{
    auto current = selectionModel()->currentIndex();
    if (!current.isValid())
        return;
    auto item = planModel()->internalPtr(current);
    if (!planModel()->canRestorePlan(current))
        return;

    auto modifiers = QGuiApplication::keyboardModifiers();
    bool restore_item = modifiers.testFlag(Qt::ShiftModifier);
    if (!restore_item) {
        QMessageBox msg;
        msg.setWindowTitle(tr("Restore Plan"));
        msg.setText(tr("Discard changes to \"%1\"?").arg(item->name()));
        msg.addButton(QMessageBox::Ok);
        msg.addButton(QMessageBox::Cancel);
        restore_item = msg.exec() == QMessageBox::Ok;
    }
    if (restore_item)
        planModel()->restorePlan(current);
}

void PlanTreeView::deleteItem()
{
    auto current = selectionModel()->currentIndex();
    if (!current.isValid())
        return;
    auto item = planModel()->internalPtr(current);
    auto modifiers = QGuiApplication::keyboardModifiers();

    bool delete_item = modifiers.testFlag(Qt::ShiftModifier)
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
}

} // namespace planner
