#include "StepItemsWidget.h"
#include "CustomEditDialog.h"
#include "Plan.h"
#include "PlanWidget.h"
#include "StepItemDelegate.h"
#include "StepItemModel.h"
#include "StepItemView.h"
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace planner {

StepItemsWidget::StepItemsWidget(StepItemModel& model, PlanWidget& plan_widget, QWidget* parent)
    : QWidget{parent}
    , is_resources_widget{model.is_resource_model}
    , plan_widget{plan_widget}
{
    auto main_layout = new QVBoxLayout{};
    main_layout->setVerticalSizeConstraint(QLayout::SetFixedSize);
    main_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(main_layout);

    auto title_layout = new QHBoxLayout{};
    title_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->addLayout(title_layout);

    title_layout->addWidget(
        new QLabel{is_resources_widget ? tr("Resources:") : tr("Results:"), this});

    method_combo = new QComboBox{this};
    method_combo->addItems(is_resources_widget ? resourceMethods() : resultMethods());
    connect(method_combo, &QComboBox::activated, this, &StepItemsWidget::setMethod);
    title_layout->addWidget(method_combo);

    custom_button = new QPushButton{tr("Edit"), this};
    auto sp = custom_button->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    custom_button->setSizePolicy(sp);
    custom_button->hide();
    connect(custom_button, &QPushButton::clicked, this, &StepItemsWidget::openCustomEdit);
    title_layout->addWidget(custom_button);

    title_layout->addStretch(1);

    view = new StepItemView{model, plan_widget, this};
    delegate = new StepItemDelegate{this};
    view->setItemDelegate(delegate);
    main_layout->addWidget(view);
}

void StepItemsWidget::openCustomEdit()
{
    if (!plan)
        return;

    auto& edit = plan_widget.customEdit();
    auto& custom_text = is_resources_widget ? plan->steps[step_pos].custom_resource_data.text
                                            : plan->steps[step_pos].custom_result_data.text;
    edit.openCustomEdit(custom_text);
    connect(
        &edit,
        &QDialog::finished,
        this,
        [this](int result) {
            if (!plan || result != QDialog::Accepted)
                return;

            auto& custom = is_resources_widget ? plan->steps[step_pos].custom_resource_data
                                               : plan->steps[step_pos].custom_result_data;
            auto& edit = plan_widget.customEdit();
            if (custom.text == edit.custom_text)
                return;

            custom.text = edit.custom_text;
            if (!custom.text.isEmpty())
                custom.tree = edit.custom_tree;
            else
                custom.tree.reset();
            plan->setChanged();

            updateCustomText();
        },
        Qt::SingleShotConnection);
}

void StepItemsWidget::updateCustomText()
{
    auto& text = is_resources_widget ? plan->steps[step_pos].custom_resource_data.text
                                     : plan->steps[step_pos].custom_result_data.text;
    custom_button->setToolTip(text);
}

void StepItemsWidget::setOtherView(StepItemsWidget& other)
{
    view->setOtherView(*other.view);
    other.view->setOtherView(*view);
}

void StepItemsWidget::setStep(Plan* plan_, size_t step_pos_)
{
    plan = plan_;
    step_pos = step_pos_;
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
