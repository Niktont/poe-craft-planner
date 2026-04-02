#ifndef STEPITEMSWIDGET_H
#define STEPITEMSWIDGET_H

#include <QWidget>

class QComboBox;
class QPushButton;

namespace planner {
class StepItemView;
class StepItemDelegate;
class Plan;
class StepItemModel;
class CustomEditDialog;

class StepItemsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StepItemsWidget(CustomEditDialog& custom_edit_dialog,
                             StepItemModel& model,
                             QWidget* parent = nullptr);

    const bool is_resources_widget;

    void updateCustomText();

    void setOtherView(StepItemsWidget& other);
    StepItemView* view;

public slots:
    void setStep(planner::Plan* plan, size_t step_pos);
    void updatePos(size_t new_pos) { step_pos = new_pos; }

private slots:
    void setMethod(int index);

private:
    Plan* plan{};
    size_t step_pos{};
    QComboBox* method_combo;
    QPushButton* custom_button;

    StepItemDelegate* delegate;

    CustomEditDialog& custom_edit_dialog;
};

} // namespace planner

#endif // STEPITEMSWIDGET_H
