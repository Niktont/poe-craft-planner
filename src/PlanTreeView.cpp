#include "PlanTreeView.h"
#include "Plan.h"
#include "PlanItem.h"
#include "PlanModel.h"
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>

namespace planner {

PlanTreeView::PlanTreeView(QWidget* parent)
    : QTreeView{parent}
{
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

    update_action = addAction(tr("Update Costs"), this, [this] {
        auto current = context_index ? *context_index : selectionModel()->currentIndex();
        bool send_requests = !QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
        planModel()->updateCosts(current, send_requests);
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

    export_clipboard_action = addAction(tr("Export (Clipboard)"), this, &PlanTreeView::exportItem);

    export_file_action = addAction(tr("Export (File)"), this, &PlanTreeView::exportItem);

    connect(this, &PlanTreeView::clicked, this, &PlanTreeView::selectPlanOnClick);
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

void PlanTreeView::setModel(QAbstractItemModel* model)
{
    QTreeView::setModel(model);

    connect(selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this,
            &PlanTreeView::selectPlanOnCurrentChange);
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

        menu->addSeparator();
        menu->addAction(update_action);

        auto item = planModel()->internalPtr(*context_index);
        if (!item->isFolder() && item->plan()->is_changed) {
            menu->addSeparator();
            menu->addAction(save_action);
            if (planModel()->canRestorePlan(*context_index))
                menu->addAction(restore_action);
        }
        menu->addSeparator();
        menu->addAction(delete_action);
    } else {
        if (planModel()->haveCopy())
            menu->addAction(paste_action);

        menu->addSeparator();
        menu->addAction(update_action);
    }

    menu->addSeparator();
    menu->addAction(export_clipboard_action);
    menu->addAction(export_file_action);

    menu->popup(event->globalPos());
}

void PlanTreeView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        auto idx = indexAt(event->pos());
        auto item = planModel()->internalPtr(idx);
        if (item->plan()) {
            emit planWindowRequested(item->plan()->id(), planModel()->game);
            event->accept();
            return;
        }
    }
    QTreeView::mousePressEvent(event);
}

void PlanTreeView::selectPlanOnClick(const QModelIndex& idx)
{
    if (QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier))
        return;

    auto item = planModel()->internalPtr(idx);
    if (item->isFolder())
        return;

    emit planSelected(*planModel(), *item->plan());
}

void PlanTreeView::selectPlanOnCurrentChange(const QModelIndex& idx)
{
    if (QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier)
        && QGuiApplication::mouseButtons().testFlag(Qt::LeftButton))
        return;

    auto item = planModel()->internalPtr(idx);
    if (item->isFolder())
        return;

    emit planSelected(*planModel(), *item->plan());
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
        QMessageBox msg{this};
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
        QMessageBox msg{this};
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

void PlanTreeView::exportItem()
{
    auto current = selectionModel()->currentIndex();

    auto modifiers = QGuiApplication::keyboardModifiers();
    bool without_deps = modifiers.testFlag(Qt::ShiftModifier);
    bool without_requests = modifiers.testFlag(Qt::ControlModifier);

    bool is_clipboard = sender() == export_clipboard_action;
    if (is_clipboard) {
        auto json = planModel()->exportItem(current, without_deps, without_requests);
        qApp->clipboard()->setText(json.toJson(QJsonDocument::Compact));
        return;
    }

    auto name = planModel()->exportFileName(current);
    auto file_name = QFileDialog::getSaveFileName(this,
                                                  tr("Export"),
                                                  name + ".json",
                                                  tr("JSON file (*.json)"));
    if (file_name.isEmpty())
        return;

    auto json = planModel()->exportItem(current, without_deps, without_requests);
    if (QFile file{file_name};
        !file.open(QFile::WriteOnly) || file.write(json.toJson(QJsonDocument::Compact)) == -1) {
        auto msg = new QMessageBox{this};
        msg->setAttribute(Qt::WA_DeleteOnClose);
        msg->setWindowTitle(tr("Export Failed"));
        msg->setText(tr("Failed to write file \"%1\".").arg(file_name));
        msg->open();
    }
}

} // namespace planner
