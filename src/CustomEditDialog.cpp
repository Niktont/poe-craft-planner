#include "CustomEditDialog.h"
#include "Plan.h"
#include "Step.h"
#include "StepItemsWidget.h"
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace planner {

CustomEditDialog::CustomEditDialog(QWidget* parent)
    : QDialog{parent}
{
    setWindowTitle(tr("Custom Expression"));

    auto layout = new QVBoxLayout{};
    layout->setVerticalSizeConstraint(QLayout::SetFixedSize);
    setLayout(layout);

    custom_edit = new QLineEdit{};
    custom_edit->setPlaceholderText("(1 + 2) | (3 + 4)");
    custom_edit->setMinimumWidth(200);
    connect(custom_edit, &QLineEdit::textChanged, this, [this](const QString& text) {
        auto fm = custom_edit->fontMetrics();
        auto width = fm.horizontalAdvance(text) + 15;
        custom_edit->resize(width, custom_edit->height());
    });

    layout->addWidget(custom_edit);

    auto buttons = new QDialogButtonBox{};

    ok_button = buttons->addButton(QDialogButtonBox::Ok);
    connect(ok_button, &QPushButton::clicked, this, &CustomEditDialog::saveCustomString);
    layout->addWidget(buttons);
}

void CustomEditDialog::openCustomEdit(Plan* plan_, size_t step_pos_, StepItemsWidget* items_widget_)
{
    plan = plan_;
    step_pos = step_pos_;
    items_widget = items_widget_;
    if (!plan)
        return;

    this->is_resource_calc = items_widget;

    auto& string = items_widget ? plan->steps[step_pos].custom_resource_data.text
                                : plan->steps[step_pos].custom_result_data.text;
    custom_edit->setText(string);
    open();
}

void CustomEditDialog::saveCustomString()
{
    if (!plan) {
        reject();
        return;
    }

    auto& custom = is_resource_calc ? plan->steps[step_pos].custom_resource_data
                                    : plan->steps[step_pos].custom_result_data;

    auto text = custom_edit->text();
    auto trimmed_text = text.trimmed();
    if (trimmed_text != custom.text) {
        auto std_text = trimmed_text.toStdString();
        auto first = std_text.cend();
        auto last = std_text.cend();
        try {
            first = calc.parseString(std_text, custom).second;
        } catch (ParseException& e) {
            first = e.first;
            last = e.last;
            custom.tree.reset();
        }
        if (first != last) {
            if (text != trimmed_text)
                custom_edit->setText(trimmed_text);
            custom_edit->setFocus();
            custom_edit->setCursorPosition(std::distance(std_text.cbegin(), first));

            auto msg = new QMessageBox{this};
            msg->setAttribute(Qt::WA_DeleteOnClose);
            msg->setWindowTitle(tr("Parse Failed"));
            msg->setText(tr("Failed to parse expression."));
            msg->open();
            return;
        }

        custom.text = trimmed_text;
        plan->setChanged();
        items_widget->updateCustomText();
    }
    accept();
}

} // namespace planner
