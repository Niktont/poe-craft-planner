#ifndef PLANWIDGET_H
#define PLANWIDGET_H

#include "Game.h"
#include <vector>
#include <QWidget>

class QLabel;
class QScrollArea;
class QAction;
class QCheckBox;

namespace planner {
class PlanModel;
class Plan;
class PlanItem;
class StepWidget;
class MainWindow;
class CostWidget;
class TradeRequestCache;
class Currency;
class PlanTitleWidget;

class PlanWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlanWidget(MainWindow& mw);

    void connectSignals();

    Game game() const;
    Plan* plan() const { return plan_; }

    void setPlanChanged();
    void updateStepNames(size_t renamed_step);

    void moveStep(size_t step_pos, bool up);
    void deleteStep(size_t step_pos);
    void duplicateStep(size_t step_pos);
    void setFinalStep(size_t step_pos, bool checked);
    void pasteStep(size_t step_pos);

signals:
    void gameChanged(planner::Game game);

public slots:
    void openPlan(const QUuid& plan_id, planner::Game game);
    void scrollToStep(const QUuid& step_id);

    void addStep();
    void updateCost(planner::Game game, const std::vector<std::pair<Plan*, bool>>& updated_plans);

    void setDescriptions(planner::Game game, const planner::Plan* target_plan);

    void hideDescriptions(bool hide);
    void hideEmptyResources(bool hide);
    void hideEmptyResults(bool hide);
    void hideNotUsedItems(bool hide);
    void hideTitleCurrencyName(bool hide);

    void goBack();
    void goForward();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void setPlanOnClick(const QModelIndex& index);
    void setPlanOnUpdate(planner::Plan& updated_plan);
    void setPlanOnCurrentChange(const QModelIndex& new_current);
    void selectPlan(planner::PlanModel& model, planner::Plan& plan);

    void reselectCurrent(planner::Game game);

    void checkDeletingPlans(const QModelIndex& parent, int first, int last);

    void updatePlanName(const planner::Plan& renamed_plan);

    void updateTradeRequests(const QModelIndex& top_left, const QModelIndex& bottom_right);
    void updateTradeName(int row, const planner::TradeRequestCache& cache);
    void updateTradeTime(int row, const planner::TradeRequestCache& cache);
    void checkDeletingTradeRequests(const QModelIndex& parent, int first, int last);

    void updateCurrencyTime(const planner::Currency& currency);
    void updateTradeDefaultTime();
    void updateExchangeDefaultTime();

    void updateOnSnapshotChange(planner::Game game);

private:
    PlanTitleWidget* title_widget;

    QScrollArea* steps_scroll;
    QWidget* steps_widget;

    QAction* paste_step_action;

    std::vector<StepWidget*> step_widgets;

    const PlanModel* current_model{};
    Plan* plan_{};

    MainWindow* mw() const;

    void setPlan(const planner::PlanModel* model, planner::Plan* plan, bool is_update = false);
    void setStepDescriptions();

    void clear();

    void emplaceStepWidget(size_t i);

    void displayFinalStep();
    void updateDisplayedCost();
    void displayCost();
    void updateCosts(bool current_updated);

    using History = std::vector<std::pair<QUuid, Game>>;
    History navigation_history;
    History::const_iterator history_it{navigation_history.end()};

    void updateBack();
    void updateForward();
};
} // namespace planner
#endif // PLANWIDGET_H
