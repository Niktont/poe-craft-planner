#ifndef PLANWIDGET_H
#define PLANWIDGET_H

#include "Plan.h"
#include "StepItem.h"
#include <vector>
#include <QWidget>

class QLabel;
class QScrollArea;
class QAction;
class QCheckBox;

namespace planner {
class PlanModel;
class Plan;
class StepWidget;
class MainWindow;
class CostWidget;
class TradeRequestCache;

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

    void openPlan(const QUuid& plan_id, Game game);
    void setPlanFromSearch(const QUuid& plan_id, Game game);

    void moveStep(size_t step_pos, bool up);
    void deleteStep(size_t step_pos);
    void duplicateStep(size_t step_pos);
    void setFinalStep(size_t step_pos, bool checked);
    void scrollToStep(const QUuid& step_id);
    void copyStep(size_t step_pos);
    void pasteStep(size_t step_pos);

    void copyItem(Game game, const StepItem& item) { step_item_copy_state = {game, item}; }
    const std::pair<Game, StepItem>& itemForPaste() const { return step_item_copy_state; }

    bool haveCopyStep() const { return !step_copy_state.first.isNull(); }
    bool haveCopyItem(Game game) const
    {
        return step_item_copy_state.first == game && game != Game::Unknown;
    }

signals:
    void gameChanged(Game game);

public slots:
    void addStep();
    void updateCost(planner::Game game, const std::vector<std::pair<Plan*, bool>>& updated_plans);

    void hideDescriptions(bool hide);
    void hideEmptyResources(bool hide);
    void hideEmptyResults(bool hide);
    void hideNotUsedItems(bool hide);
    void hideTitleCurrencyName(bool hide);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void setPlanOnClick(const QModelIndex& index);
    void setPlanOnUpdate(planner::Plan* new_plan, const planner::Plan* old_plan);
    void setPlanOnCurrentChange(const QModelIndex& new_current);
    void checkDeletingPlans(const QModelIndex& parent, int first, int last);
    void updatePlanName(const planner::Plan& renamed_plan);

    void updateTradeRequests(const QModelIndex& top_left, const QModelIndex& bottom_right);
    void updateTradeName(int row, const planner::TradeRequestCache& cache);
    void updateTradeTime(int row, const planner::TradeRequestCache& cache);
    void checkDeletingTradeRequests(const QModelIndex& parent, int first, int last);

    void updateCurrencyTime(const planner::Currency& currency);
    void updateTradeDefaultTime();
    void updateExchangeDefaultTime();

private:
    QLabel* name_label;
    QLabel* league_label;
    CostWidget* cost_widget;
    QCheckBox* locked_cb;
    QCheckBox* is_auto_final_cb;

    QScrollArea* steps_scroll;
    QWidget* steps_widget;

    QAction* paste_step_action;

    const PlanModel* current_model{};
    Plan* plan_{};

    void setPlan(const planner::PlanModel* model, planner::Plan* plan);
    void clear();

    std::vector<StepWidget*> step_widgets;
    void emplaceStepWidget(size_t i);

    void displayFinalStep();
    void updateDisplayedCost();
    void displayCost();
    void updateCosts(bool current_updated);

    std::pair<QUuid, QUuid> step_copy_state;
    std::pair<Game, StepItem> step_item_copy_state{Game::Unknown, {}};
};
} // namespace planner
#endif // PLANWIDGET_H
