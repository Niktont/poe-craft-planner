#include "StepItemsWidget.h"
#include "CustomEditDialog.h"
#include "Plan.h"
#include "StepItemDelegate.h"
#include "StepItemModel.h"
#include "StepItemView.h"
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace planner {

StepItemsWidget::StepItemsWidget(CustomEditDialog& custom_edit_dialog,
                                 StepItemModel& model,
                                 QWidget* parent)
    : QWidget{parent}
    , is_resources_widget{model.is_resource_model}
    , custom_edit_dialog{custom_edit_dialog}
{
    auto main_layout = new QVBoxLayout{};
    main_layout->setVerticalSizeConstraint(QLayout::SetFixedSize);
    main_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(main_layout);

    auto title_layout = new QHBoxLayout{};
    title_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->addLayout(title_layout);

    title_layout->addWidget(new QLabel{is_resources_widget ? tr("Resources:") : tr("Results:")});

    method_combo = new QComboBox{};
    method_combo->addItems(is_resources_widget ? resourceMethods() : resultMethods());
    connect(method_combo, &QComboBox::activated, this, &StepItemsWidget::setMethod);
    title_layout->addWidget(method_combo);

    custom_button = new QPushButton{tr("Edit")};
    auto sp = custom_button->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    custom_button->setSizePolicy(sp);
    custom_button->hide();
    connect(custom_button, &QPushButton::clicked, this, [this] {
        this->custom_edit_dialog.openCustomEdit(plan, step_pos, this);
    });
    title_layout->addWidget(custom_button);

    title_layout->addStretch(1);

    view = new StepItemView{model};
    delegate = new StepItemDelegate{this};
    view->setItemDelegate(delegate);
    main_layout->addWidget(view);
}

void StepItemsWidget::updateCustomText()
{
    auto& text = is_resources_widget ? plan->steps[step_pos].custom_resource_data.text
                                     : plan->steps[step_pos].custom_result_data.text;
    custom_button->setToolTip(text);
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

    auto& step = plan->steps[step_pos];
    if (is_resources_widget) {
        method_combo->setCurrentIndex(static_cast<int>(step.resource_calc));
        custom_button->setVisible(step.resource_calc == ResourceCalcMethod::Custom);
    } else {
        method_combo->setCurrentIndex(static_cast<int>(step.result_calc));
        custom_button->setVisible(step.result_calc == ResultCalcMethod::Custom);
    }
    updateCustomText();
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
            custom_button->setVisible(method == ResourceCalcMethod::Custom);
        }
    } else {
        auto method = static_cast<ResultCalcMethod>(index);
        if (method != plan->steps[step_pos].result_calc) {
            plan->steps[step_pos].result_calc = method;
            plan->setChanged();
            custom_button->setVisible(method == ResultCalcMethod::Custom);
        }
    }
}

} // namespace planner
