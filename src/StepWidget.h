#ifndef STEPWIDGET_H
#define STEPWIDGET_H

#include "StepItemModel.h"
#include <QFrame>

class QLineEdit;
class QTextEdit;
class QTableView;
class QAction;
class QCheckBox;

namespace planner {

class Plan;
class Step;
class PlanWidget;
class StepItemsWidget;
class CostWidget;
class DescriptionEdit;

class StepWidget : public QFrame
{
    Q_OBJECT
public:
    StepWidget(PlanWidget* plan_widget, QWidget* parent = nullptr);

    size_t stepPos() const { return step_pos; }
    void updatePos(size_t new_pos);

    void updateCost(bool current_updated);
    void updateStepName(const QUuid& changed_step, bool deleted);
    void updatePlanName(const QUuid& changed_plan);
    void updateTradeName(const planner::TradeRequestKey& request);
    void updateTradeTime(const planner::TradeRequestKey& request);
    void updateCurrencyTime(const planner::Currency& currency);
    void clearTradeRequest(const planner::TradeRequestKey& request);

    void setStep(planner::Plan* plan, size_t step_pos);
    void setFinal(bool checked);

    void setDescription();
    void hideDescription();
    void hideEmptyResources();
    void hideEmptyResults();
    void hideNotUsedItems();
    void hideTitleCurrencyName();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void deleteStep();
    void setNameFromEdit();

private:
    QLineEdit* name_edit;
    CostWidget* cost_widget;

    QWidget* edit_widget;

    DescriptionEdit* description;
    bool is_text_reset{false};

    Plan* plan{};
    size_t step_pos{};

    StepItemModel* resources_model;
    StepItemModel* results_model;

    StepItemsWidget* resources_widget;
    StepItemsWidget* results_widget;

    PlanWidget* plan_widget;

    QAction* duplicate_action;
    QAction* copy_action;
    QAction* paste_action;
    QAction* move_up_action;
    QAction* move_down_action;
    QAction* delete_action;

    QCheckBox* final_step_cb;

    void setName(QString name);
    void displayCost();

    Step* currentStep();
};
} // namespace planner
#endif // STEPWIDGET_H
