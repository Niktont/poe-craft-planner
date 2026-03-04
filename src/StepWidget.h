#ifndef STEPWIDGET_H
#define STEPWIDGET_H

#include "StepItemModel.h"
#include <QFrame>

class QLineEdit;
class QTextEdit;
class QTableView;
class QAction;

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

    Step* currentStep();

    PlanWidget* planWidget() { return plan_widget; }
    void displayCost();

    size_t stepPos() const { return step_pos; }
    void updatePos(size_t new_pos);

    void updateCost(planner::Plan* plan);
    void updateStepNames(const QUuid& changed_step, bool deleted);
    void updateTradeName(const planner::TradeRequestKey& request);
    void updateTradeTime(const planner::TradeRequestKey& request);
    void updateCurrencyTime(const planner::Currency& currency);

    void setStep(planner::Plan* plan, size_t step_pos);
    void setName(QString name);

    void setDescription();
    void hideDescription(bool hide);

public slots:
    void deleteStep();
    void setNameFromEdit();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QLineEdit* name_edit;
    CostWidget* cost_widget;

    QWidget* edit_widget;
    DescriptionEdit* description;

    Plan* plan{};
    size_t step_pos{};

    StepItemModel* resources_model;
    StepItemModel* results_model;

    StepItemsWidget* resources_widget;
    StepItemsWidget* results_widget;

    PlanWidget* plan_widget;

    QAction* duplicate_action;
    QAction* move_up_action;
    QAction* move_down_action;
    QAction* delete_action;
    bool is_text_reset{false};
};
} // namespace planner
#endif // STEPWIDGET_H
