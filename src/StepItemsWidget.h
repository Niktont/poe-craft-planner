#ifndef STEPITEMSWIDGET_H
#define STEPITEMSWIDGET_H

#include <QWidget>

class QComboBox;
class QPushButton;

namespace planner {
class StepItemView;
class StepItemDelegate;
class Plan;
class PlanWidget;
class StepItemModel;

class StepItemsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StepItemsWidget(StepItemModel& model,
                             PlanWidget& plan_widget,
                             QWidget* parent = nullptr);

    const bool is_resources_widget;

    void setOtherView(StepItemsWidget& other);
    StepItemView* view;

public slots:
    void setStep(planner::Plan* plan, size_t step_pos);
    void updatePos(size_t new_pos) { step_pos = new_pos; }

private slots:
    void setMethod(int index);
    void openCustomEdit();
    void updateCustomText();

private:
    Plan* plan{};
    size_t step_pos{};
    QComboBox* method_combo;
    QPushButton* custom_button;

    StepItemDelegate* delegate;

    PlanWidget& plan_widget;
};

} // namespace planner

#endif // STEPITEMSWIDGET_H
