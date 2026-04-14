#include "TradeRequestView.h"
#include "AppState.h"
#include "RequestEditDialog.h"
#include "Settings.h"
#include "TradeRequestCache.h"
#include <QAction>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QSortFilterProxyModel>

namespace planner {

TradeRequestView::TradeRequestView(QWidget* parent)
    : QTableView{parent}
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setWordWrap(false);
    setSelectionMode(ContiguousSelection);
    setSelectionBehavior(SelectRows);

    setModel(AppState::state.trade_cache_poe1->proxy_model);

    auto header = horizontalHeader();

    header->setMinimumSectionSize(20);
    header->setSectionResizeMode(QHeaderView::Interactive);

    auto settings = Settings::get();
    if (auto header_state = settings.value(Settings::windows_searches_view_columns);
        header_state.isValid())
        header->restoreState(header_state.toByteArray());

    auto link_width = header->sectionSizeHint(static_cast<int>(TradeRequestColumn::Link));
    header->setSectionResizeMode(static_cast<int>(TradeRequestColumn::Link), QHeaderView::Fixed);
    header->resizeSection(static_cast<int>(TradeRequestColumn::Link), link_width);

    auto query_width = header->sectionSizeHint(static_cast<int>(TradeRequestColumn::Query));
    header->setSectionResizeMode(static_cast<int>(TradeRequestColumn::Query), QHeaderView::Fixed);
    header->resizeSection(static_cast<int>(TradeRequestColumn::Query), query_width);

    setFixedWidth(header->length() + verticalScrollBar()->sizeHint().width() + lineWidth() * 2);
    connect(header, &QHeaderView::sectionResized, this, [this] {
        setFixedWidth(horizontalHeader()->length() + verticalScrollBar()->sizeHint().width()
                      + lineWidth() * 2);
    });

    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setDefaultSectionSize(fontMetrics().height());
    verticalHeader()->hide();

    edit_action = addAction(tr("Edit"), this, [this] {
        auto current = selectionModel()->currentIndex();
        current = cache->proxy_model->mapToSource(current);

        auto& request = cache->cache.nth(current.row())->first;
        AppState::state.request_edit_dialog->openRequest(cache->game, request);
    });

    add_action = addAction(tr("Add"), this, [this] {
        AppState::state.request_edit_dialog->openGame(cache->game, true);
    });

    delete_action = addAction(tr("Delete"), this, &TradeRequestView::deleteSearch);
    delete_action->setShortcuts({Qt::Key_Delete, Qt::ShiftModifier | Qt::Key_Delete});
    delete_action->setShortcutContext(Qt::WidgetShortcut);

    connect(this, &QTableView::clicked, this, &TradeRequestView::indexClicked);
}

void TradeRequestView::setCache(TradeRequestCache& cache_)
{
    if (cache == &cache_)
        return;

    cache = &cache_;
    setModel(cache->proxy_model);
}

void TradeRequestView::filterName(const QString& filter_str)
{
    if (0 < filter_str.size() && filter_str.size() < 3)
        return;

    cache->proxy_model->setFilterFixedString(filter_str);
}

void TradeRequestView::contextMenuEvent(QContextMenuEvent* event)
{
    auto menu = new QMenu{this};
    menu->setAttribute(Qt::WA_DeleteOnClose);

    auto index = indexAt(event->pos());
    if (index.isValid()) {
        menu->addAction(edit_action);
        menu->addAction(add_action);
        menu->addSeparator();
        menu->addAction(delete_action);
    } else
        menu->addAction(add_action);

    menu->popup(event->globalPos());
}

void TradeRequestView::indexClicked(const QModelIndex& idx)
{
    if (idx.column() != static_cast<int>(TradeRequestColumn::Link))
        return;

    if (QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier)) {
        auto it = cache->cache.nth(cache->proxy_model->mapToSource(idx).row());
        auto url = QUrl::fromUserInput(it->first.toUrl(cache->game));
        if (url.isValid())
            QDesktopServices::openUrl(url);
    }
}

void TradeRequestView::deleteSearch()
{
    if (!selectionModel()->hasSelection())
        return;

    auto selection = selectionModel()->selection();

    auto modifiers = QGuiApplication::keyboardModifiers();
    bool delete_search = modifiers.testFlag(Qt::ShiftModifier);
    if (!delete_search) {
        QMessageBox msg{this};
        if (selection[0].top() == selection[0].bottom()) {
            msg.setWindowTitle(tr("Delete Search"));
            msg.setText(tr("Delete this search?"));
            msg.setInformativeText(tr("This will affect all Trade items which uses this search."));
        } else {
            msg.setWindowTitle(tr("Delete Searches"));
            msg.setText(tr("Delete selected searches?"));
            msg.setInformativeText(
                tr("This will affect all Trade items which uses this searches."));
        }
        msg.addButton(QMessageBox::Ok);
        msg.addButton(QMessageBox::Cancel);
        delete_search = msg.exec() == QMessageBox::Ok;
    }
    if (!delete_search)
        return;

    if (cache->proxy_model->filterRegularExpression().pattern().isEmpty())
        cache->deleteRequests(selection[0].top(), selection[0].bottom());
    else {
        std::vector<TradeRequestKey> deleting_requests;
        auto rows = selectionModel()->selectedRows();
        for (auto& idx : rows)
            deleting_requests.push_back(
                cache->cache.nth(cache->proxy_model->mapToSource(idx).row())->first);

        for (auto& request : deleting_requests)
            cache->deleteRequest(request);
    }
}

} // namespace planner
