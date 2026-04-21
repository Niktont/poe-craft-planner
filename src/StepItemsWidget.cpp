#include "StepItemsWidget.h"
#include "CustomEditDialog.h"
#include "Plan.h"
#include "PlanWidget.h"
#include "StepItemDelegate.h"
#include "StepItemModel.h"
#include "StepItemView.h"
#include <QComboBox>
#include <QContextMenuEvent>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

namespace planner {

ExpressionEdit::ExpressionEdit(std::optional<int> min_width,
                               std::optional<int> max_width,
                               QWidget* parent)
    : QLineEdit{parent}
    , min_width{min_width}
    , max_width{max_width}
{
    if (min_width)
        setFixedWidth(*min_width);
    else if (max_width && maximumWidth() > *max_width)
        setMaximumWidth(*max_width);

    edit_action = addAction(tr("Edit"), {Qt::Key_F2}, this, &ExpressionEdit::editRequested);
    edit_action->setShortcutContext(Qt::WidgetShortcut);

    connect(this, &QLineEdit::textChanged, this, &ExpressionEdit::adjustSize);
}

void ExpressionEdit::contextMenuEvent(QContextMenuEvent* event)
{
    auto menu = createStandardContextMenu();
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->addSeparator();
    menu->addAction(edit_action);
    menu->popup(event->globalPos());
}

void ExpressionEdit::adjustSize(const QString& new_text)
{
    auto width = fontMetrics().horizontalAdvance(new_text) + 15;
    if (min_width)
        width = std::max(*min_width, width);
    if (max_width) {
        if (width > max_width) {
            width = *max_width;
            setToolTip(new_text);
        } else
            setToolTip({});
    }
    setFixedWidth(width);
}

Step& StepItemsWidget::step()
{
    assert(plan);
    return plan->steps[step_pos];
}

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

    display_edit = new ExpressionEdit{100, 400, this};
    display_edit->hide();

    connect(display_edit, &ExpressionEdit::editRequested, this, &StepItemsWidget::openCustomEdit);
    connect(display_edit, &QLineEdit::editingFinished, this, [this] {
        if (!plan) {
            display_edit->clear();
            return;
        }
        auto& custom = is_resources_widget ? step().custom_resource_data
                                           : step().custom_result_data;
        if (auto text = display_edit->text().trimmed(); custom.text != text) {
            custom.text = text;
            custom.tree.reset();
            plan->setChanged();
        }
    });

    title_layout->addWidget(display_edit);

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

    auto& dialog = plan_widget.customEdit();
    auto& custom_text = is_resources_widget ? step().custom_resource_data.text
                                            : step().custom_result_data.text;
    dialog.openCustomEdit(custom_text);
    connect(
        &dialog,
        &QDialog::finished,
        this,
        [this](int result) {
            if (!plan || result != QDialog::Accepted)
                return;

            auto& custom = is_resources_widget ? step().custom_resource_data
                                               : step().custom_result_data;
            auto& dialog = plan_widget.customEdit();
            if (custom.text != dialog.custom_text) {
                custom.text = dialog.custom_text;
                if (!custom.text.isEmpty())
                    custom.tree = dialog.custom_tree;
                else
                    custom.tree.reset();
                plan->setChanged();

                updateCustomText();
            }
        },
        Qt::SingleShotConnection);
}

void StepItemsWidget::updateCustomText()
{
    auto& text = is_resources_widget ? step().custom_resource_data.text
                                     : step().custom_result_data.text;
    display_edit->setText(text);
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

    if (is_resources_widget) {
        method_combo->setCurrentIndex(static_cast<int>(step().resource_calc));
        display_edit->setVisible(step().resource_calc == ResourceCalcMethod::Custom);
    } else {
        method_combo->setCurrentIndex(static_cast<int>(step().result_calc));
        display_edit->setVisible(step().result_calc == ResultCalcMethod::Custom);
    }
    updateCustomText();
}

void StepItemsWidget::setMethod(int index)
{
    if (!plan)
        return;

    if (is_resources_widget) {
        auto method = static_cast<ResourceCalcMethod>(index);
        if (method != step().resource_calc) {
            step().resource_calc = method;
            plan->setChanged();
            display_edit->setVisible(method == ResourceCalcMethod::Custom);
        }
    } else {
        auto method = static_cast<ResultCalcMethod>(index);
        if (method != step().result_calc) {
            step().result_calc = method;
            plan->setChanged();
            display_edit->setVisible(method == ResultCalcMethod::Custom);
        }
    }
}

} // namespace planner
