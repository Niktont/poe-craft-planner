#include "StepItemView.h"
#include "PlanWidget.h"
#include "Settings.h"
#include "StepItemModel.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QStyledItemDelegate>

using namespace Qt::StringLiterals;

static constexpr int min_name_width = 120;

namespace planner {

int StepItemView::widthForItemText(QStyleOptionViewItem& option, const QString& text) const
{
    option.text = text;
    return style()->sizeFromContents(QStyle::CT_ItemViewItem, &option, {}).width() + showGrid();
}

void StepItemView::setupColumns()
{
    if (!stepModel()->is_resource_model) {
        hideColumn(static_cast<int>(StepItemColumn::Gold));
        hideColumn(static_cast<int>(StepItemColumn::Time));
    } else
        hideColumn(static_cast<int>(StepItemColumn::Success));

    QStyleOptionViewItem option;
    initViewItemOption(&option);
    option.features = QStyleOptionViewItem::HasDisplay;

    auto header = horizontalHeader();
    header->setMinimumSectionSize(20);
    header->setSectionResizeMode(QHeaderView::Fixed);
    header->resizeSection(static_cast<int>(StepItemColumn::Row), 20);
    header->resizeSection(static_cast<int>(StepItemColumn::Type), 20);

    auto num_6_width = widthForItemText(option, u"10000.0"_s);
    auto num_5_width = widthForItemText(option, u"1000.0"_s);

    auto amount_width = std::max(header->sectionSizeHint(static_cast<int>(StepItemColumn::Amount)),
                                 num_6_width);
    header->resizeSection(static_cast<int>(StepItemColumn::Amount), amount_width);

    header->resizeSection(static_cast<int>(StepItemColumn::Name), min_name_width);

    auto link_width = header->sectionSizeHint(static_cast<int>(StepItemColumn::Link));
    header->resizeSection(static_cast<int>(StepItemColumn::Link), link_width);

    auto cost_num_width = std::max(widthForItemText(option, u"1/1000.0"_s), num_6_width);
    auto cost_width = std::max(header->sectionSizeHint(static_cast<int>(StepItemColumn::Cost)),
                               cost_num_width);
    header->resizeSection(static_cast<int>(StepItemColumn::Cost), cost_width);

    header->resizeSection(static_cast<int>(StepItemColumn::CostCurrency), min_name_width);

    auto gold_width = std::max(header->sectionSizeHint(static_cast<int>(StepItemColumn::Gold)),
                               num_6_width);
    header->resizeSection(static_cast<int>(StepItemColumn::Gold), gold_width);

    auto time_width = std::max(header->sectionSizeHint(static_cast<int>(StepItemColumn::Time)),
                               num_5_width);
    header->resizeSection(static_cast<int>(StepItemColumn::Time), time_width);
    auto success_width = gold_width + time_width;
    header->resizeSection(static_cast<int>(StepItemColumn::Success), success_width);

    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setDefaultSectionSize(fontMetrics().height());
    verticalHeader()->hide();

    connect(stepModel(), &StepItemModel::rowsInserted, this, &StepItemView::syncColumns);
    connect(stepModel(), &StepItemModel::rowsRemoved, this, &StepItemView::syncColumns);
    connect(stepModel(), &StepItemModel::dataChanged, this, &StepItemView::resizeColumns);
}

StepItemView::StepItemView(StepItemModel& model, QWidget* parent)
    : QTableView{parent}
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setWordWrap(false);

    setDragDropMode(QAbstractItemView::DragDrop);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropOverwriteMode(false);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setSelectionMode(SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    setModel(&model);

    setupColumns();

    connect(this, &QTableView::clicked, this, &StepItemView::indexClicked);

    add_exchange_action = addAction(tr("Add Exchange Item"), this, [this] {
        auto current = context_index ? *context_index : selectionModel()->currentIndex();
        stepModel()->insertItem(current, StepItemType::Exchange);
        context_index.reset();
    });
    add_trade_action = addAction(tr("Add Trade Item"), this, [this] {
        auto current = context_index ? *context_index : selectionModel()->currentIndex();
        stepModel()->insertItem(current, StepItemType::Trade);
        context_index.reset();
    });
    add_custom_action = addAction(tr("Add Custom Item"), this, [this] {
        auto current = context_index ? *context_index : selectionModel()->currentIndex();
        stepModel()->insertItem(current, StepItemType::Custom);
        context_index.reset();
    });
    add_step_action = addAction(tr("Add Step Item"), this, [this] {
        auto current = context_index ? *context_index : selectionModel()->currentIndex();
        stepModel()->insertItem(current, StepItemType::Step);
        context_index.reset();
    });
    add_plan_action = addAction(tr("Add Plan Item"), this, [this] {
        auto current = context_index ? *context_index : selectionModel()->currentIndex();
        stepModel()->insertItem(current, StepItemType::Plan);
        context_index.reset();
    });

    duplicate_action = addAction(tr("Duplicate"), {Qt::ControlModifier | Qt::Key_D}, this, [this] {
        stepModel()->duplicateItem(selectionModel()->currentIndex());
    });
    duplicate_action->setShortcutContext(Qt::WidgetShortcut);

    copy_action = addAction(tr("Copy"), this, [this] {
        stepModel()->copyItem(selectionModel()->currentIndex());
    });
    paste_action = addAction(tr("Paste"), this, [this] {
        auto current = context_index ? *context_index : selectionModel()->currentIndex();
        stepModel()->pasteItem(current);
        context_index.reset();
    });

    copy_regex_action = addAction(tr("Copy Regex"), {Qt::AltModifier | Qt::Key_C}, this, [this] {
        stepModel()->copyRegex(selectionModel()->currentIndex());
    });
    copy_regex_action->setShortcutContext(Qt::WidgetShortcut);

    edit_search_action = addAction(tr("Edit Search"), this, [this] {
        stepModel()->openSearch(selectionModel()->currentIndex());
    });

    delete_search_action = addAction(tr("Delete Search"), this, &StepItemView::deleteSearch);

    default_time_action = addAction(tr("Set As Default"), this, [this] {
        stepModel()->setDefaultTime(selectionModel()->currentIndex());
    });

    delete_action = addAction(tr("Delete"), QKeySequence{Qt::ShiftModifier | Qt::Key_Delete});
    delete_action->setShortcutContext(Qt::WidgetShortcut);
    connect(delete_action, &QAction::triggered, this, [this] {
        auto selection = selectionModel()->selection();
        if (selection.count() > 0) {
            for (const auto& sel_range : selection)
                stepModel()->removeRows(sel_range.top(), sel_range.bottom() - sel_range.top() + 1);
        }
    });
}

void StepItemView::contextMenuEvent(QContextMenuEvent* event)
{
    auto menu = new QMenu{this};
    menu->setAttribute(Qt::WA_DeleteOnClose);

    bool have_item_copy = stepModel()->planWidget()->haveCopyItem(stepModel()->game());
    context_index = indexAt(event->pos());
    if (context_index->isValid()) {
        menu->addAction(duplicate_action);
        menu->addAction(copy_action);
        if (have_item_copy)
            menu->addAction(paste_action);

        menu->addSeparator();
        auto item = stepModel()->stepItem(*context_index);
        if (item) {
            switch (item->type()) {
            case StepItemType::Trade:
                if (stepModel()->haveRegex(*item))
                    menu->addAction(copy_regex_action);
                menu->addAction(edit_search_action);
                menu->addAction(delete_search_action);
                if (context_index->column() == static_cast<int>(StepItemColumn::Time))
                    menu->addAction(default_time_action);
                menu->addSeparator();
                break;
            case StepItemType::Exchange:
                if (context_index->column() == static_cast<int>(StepItemColumn::Time)) {
                    menu->addAction(default_time_action);
                    menu->addSeparator();
                }
                break;
            default:
                break;
            }
        }
        menu->addAction(delete_action);
    } else {
        menu->addAction(add_trade_action);
        menu->addAction(add_exchange_action);
        menu->addAction(add_custom_action);
        if (stepModel()->stepPos() > 0)
            menu->addAction(add_step_action);
        menu->addAction(add_plan_action);

        if (have_item_copy)
            menu->addAction(paste_action);
    }

    context_menu_shown = true;
    menu->popup(event->globalPos());
}

void StepItemView::focusOutEvent(QFocusEvent* event)
{
    QTableView::focusOutEvent(event);
    if (!context_menu_shown)
        selectionModel()->clearSelection();
    context_menu_shown = false;
}

QSize StepItemView::sizeHint() const
{
    return {horizontalHeader()->length() /* + verticalHeader()->sizeHint().width()*/ + lineWidth() * 2,
            horizontalHeader()->height() + verticalHeader()->length() + 15 + lineWidth() * 2};
}

void StepItemView::hideNotUsedItems()
{
    auto step = stepModel()->step();
    auto& items = stepModel()->is_resource_model ? step->resources : step->results;
    if (items.empty())
        return;

    bool rows_changed = false;
    if (Settings::get<Settings::windows_main_hide_not_used_items>()) {
        for (size_t i = 0; i < items.size(); ++i) {
            if (isRowHidden(i) != items[i].not_used) {
                setRowHidden(i, items[i].not_used);
                rows_changed = true;
            }
        }
    } else {
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i].not_used) {
                setRowHidden(i, false);
                rows_changed = true;
            }
        }
    }

    if (rows_changed)
        setFixedSize(sizeHint());
}

void StepItemView::resizeColumns(const QModelIndex& top_left,
                                 const QModelIndex& bottom_right,
                                 const QList<int>& /*roles*/)
{
    auto col_left = static_cast<StepItemColumn>(top_left.column());
    auto col_right = static_cast<StepItemColumn>(bottom_right.column());
    bool resized = false;
    if (col_left <= StepItemColumn::Name && StepItemColumn::Name <= col_right) {
        auto col = static_cast<int>(StepItemColumn::Name);
        if (other_view)
            syncColumn(col, min_name_width);
        else
            resizeColumnToContents(col);
        resized = true;
    }
    if (col_left <= StepItemColumn::CostCurrency && StepItemColumn::CostCurrency <= col_right) {
        auto col = static_cast<int>(StepItemColumn::CostCurrency);
        if (other_view)
            syncColumn(col, min_name_width);
        else
            resizeColumnToContents(col);
        resized = true;
    }
    if (resized) {
        setFixedSize(sizeHint());
        if (other_view)
            other_view->setFixedSize(other_view->sizeHint());
    }
}

void StepItemView::indexClicked(const QModelIndex& idx)
{
    if (idx.column() != static_cast<int>(StepItemColumn::Link))
        return;

    if (QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier))
        stepModel()->openLink(idx);
}

void StepItemView::deleteSearch()
{
    auto current = selectionModel()->currentIndex();
    auto item = stepModel()->stepItem(current);
    if (!item)
        return;
    if (auto trade = item->trade(); !trade || !trade->request_key.isValid())
        return;

    auto modifiers = QGuiApplication::keyboardModifiers();

    bool delete_search = modifiers.testFlag(Qt::ShiftModifier);
    if (!delete_search) {
        QMessageBox msg;

        msg.setWindowTitle(tr("Delete Search"));
        msg.setText(tr("Delete this search?"));
        msg.setInformativeText(tr("This will affect all Trade items which uses this search."));
        msg.addButton(QMessageBox::Ok);
        msg.addButton(QMessageBox::Cancel);
        delete_search = msg.exec() == QMessageBox::Ok;
    }
    if (delete_search)
        stepModel()->deleteSearch(current);
}

void StepItemView::syncColumns()
{
    syncColumn(static_cast<int>(StepItemColumn::Row), 20);
    syncColumn(static_cast<int>(StepItemColumn::Name), min_name_width);
    syncColumn(static_cast<int>(StepItemColumn::CostCurrency), min_name_width);
    setFixedSize(sizeHint());
    if (other_view)
        other_view->setFixedSize(other_view->sizeHint());
}

void StepItemView::syncColumn(int col, int min_width)
{
    auto size_hint = sizeHintForColumn(col);
    auto other_size_hint = other_view ? other_view->sizeHintForColumn(col) : 0;
    auto width = std::max({size_hint, other_size_hint, min_width});
    setColumnWidth(col, width);
    if (other_view)
        other_view->setColumnWidth(col, width);
}

StepItemModel* StepItemView::stepModel() const
{
    return static_cast<StepItemModel*>(model());
}

} // namespace planner
