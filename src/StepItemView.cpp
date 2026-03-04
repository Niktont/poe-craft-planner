#include "StepItemView.h"
#include "StepItemModel.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QStyledItemDelegate>
using namespace Qt::StringLiterals;

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
    header->resizeSection(static_cast<int>(StepItemColumn::Type), 20);

    auto num_6_width = widthForItemText(option, u"10000.0"_s);
    auto num_5_width = widthForItemText(option, u"1000.0"_s);

    auto amount_width = std::max(header->sectionSizeHint(static_cast<int>(StepItemColumn::Amount)),
                                 num_6_width);
    header->resizeSection(static_cast<int>(StepItemColumn::Amount), amount_width);

    header->resizeSection(static_cast<int>(StepItemColumn::Name), 120);

    auto link_width = header->sectionSizeHint(static_cast<int>(StepItemColumn::Link));
    header->resizeSection(static_cast<int>(StepItemColumn::Link), link_width);

    auto cost_num_width = std::max(widthForItemText(option, u"1/1000.0"_s), num_6_width);
    auto cost_width = std::max(header->sectionSizeHint(static_cast<int>(StepItemColumn::Cost)),
                               cost_num_width);
    header->resizeSection(static_cast<int>(StepItemColumn::Cost), cost_width);

    header->resizeSection(static_cast<int>(StepItemColumn::CostCurrency), 120);

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

    connect(stepModel(), &StepItemModel::rowsInserted, this, &StepItemView::syncColumns);
    connect(stepModel(), &StepItemModel::rowsRemoved, this, &StepItemView::syncColumns);
    connect(stepModel(), &StepItemModel::dataChanged, this, &StepItemView::resizeColumns);
}

QSize StepItemView::totalSize() const
{
    return {horizontalHeader()->length() + verticalHeader()->width() + lineWidth() * 2,
            static_cast<int>(horizontalHeader()->height() * 1.4) + verticalHeader()->length()
                + lineWidth() * 2};
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
        auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
        stepModel()->insertItem(current, StepItemType::Exchange);
        contextIndex.reset();
    });
    add_trade_action = addAction(tr("Add Trade Item"), this, [this] {
        auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
        stepModel()->insertItem(current, StepItemType::Trade);
        contextIndex.reset();
    });
    add_custom_action = addAction(tr("Add Custom Item"), this, [this] {
        auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
        stepModel()->insertItem(current, StepItemType::Custom);
        contextIndex.reset();
    });
    add_step_action = addAction(tr("Add Step Item"), this, [this] {
        auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
        stepModel()->insertItem(current, StepItemType::Step);
        contextIndex.reset();
    });
    duplicate_action = addAction(tr("Duplicate"), {Qt::CTRL | Qt::Key_D}, this, [this] {
        auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
        stepModel()->duplicateItem(current);
        contextIndex.reset();
    });
    duplicate_action->setShortcutContext(Qt::WidgetShortcut);

    manage_searches_action = addAction(tr("Manage Searches"), this, [this] {
        auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
        stepModel()->openSearch(current);
        contextIndex.reset();
    });

    default_time_action = addAction(tr("Set As Default"), this, [this] {
        auto current = contextIndex ? *contextIndex : selectionModel()->currentIndex();
        stepModel()->setDefaultTime(current);
        contextIndex.reset();
    });

    delete_action = addAction(tr("Delete"), QKeySequence{Qt::SHIFT | Qt::Key_Delete});
    delete_action->setShortcutContext(Qt::WidgetShortcut);
    connect(delete_action, &QAction::triggered, this, [this] {
        auto selection = selectionModel()->selection();
        if (selection.count() > 0) {
            for (const auto& sel_range : selection)
                stepModel()->removeRows(sel_range.top(), sel_range.bottom() - sel_range.top() + 1);
        }
        contextIndex.reset();
    });
}

void StepItemView::contextMenuEvent(QContextMenuEvent* event)
{
    auto menu = new QMenu{this};
    menu->setAttribute(Qt::WA_DeleteOnClose);

    menu->addAction(add_trade_action);
    menu->addAction(add_exchange_action);
    menu->addAction(add_custom_action);
    contextIndex = indexAt(event->pos());
    if (stepModel()->stepPos() > 0)
        menu->addAction(add_step_action);

    if (contextIndex->isValid()) {
        menu->addSeparator();
        menu->addAction(duplicate_action);
        auto item = stepModel()->stepItem(*contextIndex);
        if (item) {
            switch (item->type()) {
            case StepItemType::Trade:
                menu->addAction(manage_searches_action);
                [[fallthrough]];
            case StepItemType::Exchange:
                if (contextIndex->column() == static_cast<int>(StepItemColumn::Time))
                    menu->addAction(default_time_action);
                break;
            default:
                break;
            }
        }
        menu->addSeparator();
        menu->addAction(delete_action);
    }

    menu->popup(event->globalPos());
}

QSize StepItemView::sizeHint() const
{
    return {horizontalHeader()->length() + verticalHeader()->width() + lineWidth() * 2,
            static_cast<int>(horizontalHeader()->height() * 1.4) + verticalHeader()->length()
                + lineWidth() * 2};
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
            syncColumn(col);
        else
            resizeColumnToContents(col);
        resized = true;
    }
    if (col_left <= StepItemColumn::CostCurrency && StepItemColumn::CostCurrency <= col_right) {
        auto col = static_cast<int>(StepItemColumn::CostCurrency);
        if (other_view)
            syncColumn(col);
        else
            resizeColumnToContents(col);
        resized = true;
    }
    if (resized) {
        updateGeometry();
        if (other_view)
            other_view->updateGeometry();
    }
}

void StepItemView::indexClicked(const QModelIndex& idx)
{
    if (idx.column() != static_cast<int>(StepItemColumn::Link))
        return;

    auto modifiers = QGuiApplication::keyboardModifiers();
    if (modifiers & Qt::ControlModifier) {
        auto url = QUrl::fromUserInput(model()->data(idx, Qt::ToolTipRole).toString());
        if (url.isValid())
            QDesktopServices::openUrl(url);
    }
}

void StepItemView::syncColumns()
{
    syncColumn(static_cast<int>(StepItemColumn::Name));
    syncColumn(static_cast<int>(StepItemColumn::CostCurrency));
    updateGeometry();
    if (other_view)
        other_view->updateGeometry();
}

void StepItemView::syncColumn(int col)
{
    auto size_hint = sizeHintForColumn(col);
    auto other_size_hint = other_view ? other_view->sizeHintForColumn(col) : 0;
    auto width = std::max({size_hint, other_size_hint, 120});
    setColumnWidth(col, width);
    if (other_view)
        other_view->setColumnWidth(col, width);
}

StepItemModel* StepItemView::stepModel()
{
    return static_cast<StepItemModel*>(model());
}

} // namespace planner
