#include "PlanSearchView.h"
#include "AppState.h"
#include "PlanModel.h"
#include "PlanSearchModel.h"
#include <QHeaderView>
#include <QMouseEvent>
#include <QScrollBar>
#include <QSortFilterProxyModel>

namespace planner {

PlanSearchView::PlanSearchView(QWidget* parent)
    : QTableView{parent}
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setWordWrap(false);
    setSelectionMode(SingleSelection);
    setSelectionBehavior(SelectRows);

    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    horizontalHeader()->hide();

    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setDefaultSectionSize(fontMetrics().height());
    verticalHeader()->hide();

    connect(this, &QTableView::clicked, this, &PlanSearchView::indexClicked);
}

void PlanSearchView::setGame(Game game_)
{
    if (game != game_) {
        game = game_;
        search_model = AppState::planModel(game)->search_model;
        setModel(search_model->proxy_model);
    }
}

void PlanSearchView::filterName(const QString& filter_str)
{
    if (0 < filter_str.size() && filter_str.size() < 2)
        return;

    search_model->proxy_model->setFilterFixedString(filter_str);
}

void PlanSearchView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        if (auto idx = indexAt(event->pos()); idx.isValid()) {
            emit planClicked(search_model->planId(search_model->proxy_model->mapToSource(idx)),
                             game,
                             true);
            return;
        }
    }
    QTableView::mousePressEvent(event);
}

void PlanSearchView::indexClicked(const QModelIndex& idx)
{
    auto plan_id = search_model->planId(search_model->proxy_model->mapToSource(idx));
    emit planClicked(plan_id, game, false);
}

} // namespace planner
