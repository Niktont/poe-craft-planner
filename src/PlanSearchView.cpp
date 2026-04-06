#include "PlanSearchView.h"
#include "MainWindow.h"
#include "PlanModel.h"
#include "PlanSearchModel.h"
#include "PlanTreeView.h"
#include "PlanWidget.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QSortFilterProxyModel>

namespace planner {

PlanSearchView::PlanSearchView(MainWindow& mw, QWidget* parent)
    : QTableView{parent}
    , mw{mw}
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
        search_model = mw.planModel(game)->search_model;
        setModel(search_model->proxy_model);
    }
}

void PlanSearchView::filterName(const QString& filter_str)
{
    if (0 < filter_str.size() && filter_str.size() < 3)
        return;

    search_model->proxy_model->setFilterFixedString(filter_str);
}

void PlanSearchView::indexClicked(const QModelIndex& idx)
{
    auto plan_id = search_model->planId(search_model->proxy_model->mapToSource(idx));
    mw.planWidget()->openPlan(plan_id, game);
}

} // namespace planner
