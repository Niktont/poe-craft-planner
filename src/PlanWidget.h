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
class Plan;
class PlanItem;
class StepWidget;
class CostWidget;
class TradeRequestCache;
class Currency;
class PlanTitleWidget;
class CustomEditDialog;
class ShoppingSetupDialog;
class RequestEditDialog;

class PlanWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlanWidget(bool is_main, QWidget* parent = nullptr, Qt::WindowFlags flags = {});

    void connectSignals();

    Game game() const;
    Plan* plan() const { return plan_; }

    void updateStepNames(size_t renamed_step);

    void moveStep(size_t step_pos, bool up);
    void deleteStep(size_t step_pos);
    void duplicateStep(size_t step_pos);
    void setFinalStep(size_t step_pos, bool checked);
    void pasteStep(size_t step_pos);

    CustomEditDialog& customEdit();
    ShoppingSetupDialog& shoppingSetup();
    RequestEditDialog& requestEdit();

signals:
    void gameChanged(planner::Game game);
    void aboutToClose(planner::PlanWidget& window);

public slots:
    void scrollToStep(const QUuid& step_id);

    void addStep();
    void updateCost(planner::Game game, const std::vector<std::pair<Plan*, bool>>& updated_plans);

    void setDescriptions(planner::Game game, const planner::Plan* target_plan);

    void hideDescriptions();
    void hideEmptyResources();
    void hideEmptyResults();
    void hideNotUsedItems();
    void hideTitleCurrencyName();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void setPlanOnUpdate(planner::Plan& updated_plan);

    void checkDeletingItems(const QModelIndex& parent, int first, int last);

    void updatePlanName(const planner::Plan& renamed_plan);
    void resetDescriptionChange(planner::Game game, const planner::Plan* plan);

    void updateTradeRequests(const QModelIndex& top_left, const QModelIndex& bottom_right);
    void updateTradeName(int row, const planner::TradeRequestCache& cache);
    void updateTradeTime(int row, const planner::TradeRequestCache& cache);
    void checkDeletingTradeRequests(const QModelIndex& parent, int first, int last);

    void updateCurrencyTime(const planner::Currency& currency);
    void updateTradeDefaultTime();
    void updateExchangeDefaultTime();

    void updateOnSnapshotChange(planner::Game game);

    void openShoppingDialog();

private:
    PlanTitleWidget* title_widget;

    QScrollArea* steps_scroll;
    QWidget* steps_widget;

    QAction* add_step_action;
    QAction* paste_step_action;

    QAction* update_costs_action;
    QAction* shopping_mode_action;

    std::vector<StepWidget*> step_widgets;

    CustomEditDialog* custom_edit_dialog{};
    ShoppingSetupDialog* shopping_setup{};
    RequestEditDialog* request_edit{};

    Plan* plan_{};

    const bool is_main;
    friend class MainWidget;

    void setStepDescriptions();

    void setPlan(planner::Plan* plan, bool is_update = false);
    void clear();

    bool checkDeletingPlans(const PlanItem& parent_item, int first, int last);

    void emplaceStepWidget(size_t i);
    void reuseStepWidget(size_t i);

    void displayFinalStep();
    void updateDisplayedCost();
    void displayCost();
    void updateCosts(bool current_updated);

    struct ScrollState
    {
        QWidget* step_widget{};
        QWidget* focus_widget{};
        int y_diff;
    };
    ScrollState scrollState();
    void restoreScrollState(const ScrollState& state);

    template<auto fun, typename... Args>
    void applyResizeFunction(Args&&... args);
};
} // namespace planner
#endif // PLANWIDGET_H
