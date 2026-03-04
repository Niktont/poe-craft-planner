#include "StepItemsWidget.h"
#include "Plan.h"
#include "StepItemDelegate.h"
#include "StepItemModel.h"
#include "StepItemView.h"
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>

namespace planner {

StepItemsWidget::StepItemsWidget(StepItemModel& model, QWidget* parent)
    : QWidget{parent}
    , is_resources_widget{model.is_resource_model}
{
    auto main_layout = new QVBoxLayout{};
    main_layout->setContentsMargins(0, 0, 0, 5);
    setLayout(main_layout);

    auto title_layout = new QHBoxLayout{};
    title_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->addLayout(title_layout);

    title_layout->addWidget(new QLabel{is_resources_widget ? tr("Resources:") : tr("Results:")});

    method_combo = new QComboBox{};
    method_combo->addItems(is_resources_widget ? resourceMethods() : resultMethods());
    title_layout->addWidget(method_combo);
    title_layout->addStretch(1);
    view = new StepItemView{model};
    delegate = new StepItemDelegate{this};
    view->setItemDelegate(delegate);
    main_layout->addWidget(view);
    connect(method_combo, &QComboBox::activated, this, &StepItemsWidget::setMethod);
}

void StepItemsWidget::setOtherView(StepItemsWidget& other)
{
    view->setOtherView(other.view);
    other.view->setOtherView(view);
}

void StepItemsWidget::setStep(Plan* plan, size_t step_pos)
{
    this->plan = plan;
    this->step_pos = step_pos;
    if (!plan)
        return;
    if (is_resources_widget)
        method_combo->setCurrentIndex(static_cast<int>(plan->steps[step_pos].resource_calc));
    else
        method_combo->setCurrentIndex(static_cast<int>(plan->steps[step_pos].result_calc));
}

void StepItemsWidget::setMethod(int index)
{
    if (!plan)
        return;

    if (is_resources_widget) {
        auto method = static_cast<ResourceCalcMethod>(index);
        if (method != plan->steps[step_pos].resource_calc) {
            plan->steps[step_pos].resource_calc = method;
            plan->setChanged();
        }
    } else {
        auto method = static_cast<ResultCalcMethod>(index);
        if (method != plan->steps[step_pos].result_calc) {
            plan->steps[step_pos].result_calc = method;
            plan->setChanged();
        }
    }
}

} // namespace planner
