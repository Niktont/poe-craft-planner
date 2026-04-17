#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "Game.h"
#include <vector>
#include <QUuid>
#include <QWidget>

class QAction;

namespace planner {
class Plan;
class PlanModel;
class PlanWidget;
class TradeRequestCache;
class Currency;

class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(QWidget* parent = nullptr);

    void connectSignals();

    Game game() const;
    Plan* plan() const;

signals:
    void gameChanged(planner::Game game);

public slots:
    void openPlan(const QUuid& plan_id, planner::Game game);

    void addStep();
    void updateCost(planner::Game game, const std::vector<std::pair<Plan*, bool>>& updated_plans);

    void hideDescriptions(bool hide);
    void hideEmptyResources(bool hide);
    void hideEmptyResults(bool hide);
    void hideNotUsedItems(bool hide);
    void hideTitleCurrencyName(bool hide);

    void checkDeletingPlans(const QModelIndex& parent, int first, int last);

    void goBack();
    void goForward();

private slots:
    void setPlanOnClick(const QModelIndex& index);
    void setPlanOnCurrentChange(const QModelIndex& new_current);
    void selectPlan(planner::PlanModel& model, planner::Plan& selected_plan);

    void reselectCurrent(planner::Game game);

    void setPlan(const planner::PlanModel* model,
                 planner::Plan* new_plan,
                 bool update_history = true);

private:
    PlanWidget* plan_widget;

    using History = std::vector<std::pair<QUuid, Game>>;
    History navigation_history;
    History::iterator history_it{navigation_history.end()};

    void updateBack();
    void updateForward();
};
} // namespace planner

#endif // MAINWIDGET_H
