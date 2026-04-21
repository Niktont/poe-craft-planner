#ifndef STEPITEMSWIDGET_H
#define STEPITEMSWIDGET_H

#include <QLineEdit>
#include <QWidget>

class QComboBox;
class QAction;

namespace planner {
class StepItemView;
class StepItemDelegate;
class Plan;
class Step;
class PlanWidget;
class StepItemModel;

class ExpressionEdit : public QLineEdit
{
    Q_OBJECT
public:
    ExpressionEdit(std::optional<int> min_width = {},
                   std::optional<int> max_width = {},
                   QWidget* parent = nullptr);

signals:
    void editRequested();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void adjustSize(const QString& new_text);

private:
    QAction* edit_action;

    std::optional<int> min_width;
    std::optional<int> max_width;
};

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
    ExpressionEdit* display_edit;

    StepItemDelegate* delegate;

    PlanWidget& plan_widget;

    Step& step();
    const Step& step() const { return const_cast<StepItemsWidget*>(this)->step(); }
};

} // namespace planner

#endif // STEPITEMSWIDGET_H
