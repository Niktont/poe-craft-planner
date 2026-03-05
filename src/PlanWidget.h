#ifndef PLANWIDGET_H
#define PLANWIDGET_H

#include "Plan.h"
#include <vector>
#include <QWidget>

class QLabel;
class QNetworkReply;

namespace planner {
class PlanModel;
class Plan;
class StepWidget;
class MainWindow;
class CostWidget;

class PlanWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlanWidget(MainWindow& mw);

    void connectSignals();

    MainWindow* mw() const;
    Game game() const;
    Plan* plan() const { return plan_; }

    void setPlanChanged();
    void updateStepNames(size_t renamed_step);
    void setDescriptions(planner::Plan* plan);

    void moveStep(size_t step_pos, bool up);
    void deleteStep(size_t step_pos);
    void duplicateStep(size_t step_pos);

public slots:
    void addStep();
    void updateCost(planner::Plan* plan);
    void hideDescriptions(bool hide);

private slots:
    void setPlanOnClick(const QModelIndex& index);
    void setPlanOnUpdate(planner::Plan* new_plan, const planner::Plan* old_plan);
    void setPlanOnCurrentChange(const QModelIndex& new_current);
    void checkDeletingPlans(const QModelIndex& parent, int first, int last);
    void updatePlanName(planner::Plan* renamed_plan);

    void updateTradeName(int row, planner::Game game);
    void checkDeletingTradeRequest(int row, planner::Game game);

    void updateTradeTime(const planner::TradeRequestKey& request);
    void updateCurrencyTime(const planner::Currency& currency);
    void updateTradeDefaultTime();
    void updateExchangeDefaultTime();

private:
    QLabel* name_label;
    QLabel* league_label;
    CostWidget* cost_widget;

    QWidget* steps_widget;

    const PlanModel* current_model{};
    Plan* plan_{};

    void setPlan(const planner::PlanModel* model, planner::Plan* plan);
    void clear();

    bool is_descriptions_hidden{false};
    std::vector<StepWidget*> step_widgets;
    void addStepWidget(size_t i);
    void displayCost();
};
} // namespace planner
#endif // PLANWIDGET_H
