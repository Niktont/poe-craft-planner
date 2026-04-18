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
    void openPlanLink(const QUuid& plan_id, planner::Game game, bool need_window);
    void openPlan(const QUuid& plan_id, planner::Game game);
    void openPlanWindow(const QUuid& plan_id, planner::Game game);

    void addStep();
    void savePlan();
    void updateCosts();
    void openShoppingDialog();

    void hideDescriptions(bool hide);
    void hideEmptyResources(bool hide);
    void hideEmptyResults(bool hide);
    void hideNotUsedItems(bool hide);
    void hideTitleCurrencyName(bool hide);

    void updateCost(planner::Game game, const std::vector<std::pair<Plan*, bool>>& updated_plans);

    void checkDeletingPlans(const QModelIndex& parent, int first, int last);

    void goBack();
    void goForward();

private slots:
    void selectPlan(planner::PlanModel& model, planner::Plan& selected_plan);

    void reselectCurrent(planner::Game game);

    void setPlan(const planner::PlanModel& model,
                 planner::Plan& new_plan,
                 bool update_history = true);
    void eraseWindow(planner::PlanWidget& window);

private:
    PlanWidget* plan_widget;
    std::map<QUuid, PlanWidget*> plan_windows;
    bool isCurrentPlan(const QUuid& id) const;
    bool isPlanActive(const QUuid& id) const;

    static void raisePlanWindow(PlanWidget& window);

    using History = std::vector<std::pair<QUuid, Game>>;
    History navigation_history;
    History::iterator history_it{navigation_history.end()};

    void updateBack();
    void updateForward();
};
} // namespace planner

#endif // MAINWIDGET_H
