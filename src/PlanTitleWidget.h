#ifndef PLANTITLEWIDGET_H
#define PLANTITLEWIDGET_H

#include <QWidget>

class QLabel;
class QCheckBox;

namespace planner {
class CostWidget;
class Plan;

class PlanTitleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlanTitleWidget(QWidget* parent = nullptr);

    void setPlan(const Plan& plan);

    QLabel* name_label;
    QCheckBox* is_auto_final_cb;
    QCheckBox* locked_cb;
    QLabel* league_label;
    CostWidget* cost_widget;
};

} // namespace planner

#endif // PLANTITLEWIDGET_H
