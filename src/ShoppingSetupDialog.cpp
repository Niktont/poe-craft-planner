#include "ShoppingSetupDialog.h"
#include "AppState.h"
#include "Plan.h"
#include "ShoppingDialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace planner {

ShoppingSetupDialog::ShoppingSetupDialog(QWidget* parent)
    : QDialog{parent}
{
    setWindowTitle(tr("Shopping Setup"));

    auto main_layout = new QVBoxLayout{};
    main_layout->setVerticalSizeConstraint(QLayout::SetFixedSize);
    setLayout(main_layout);
    auto form = new QFormLayout{};
    main_layout->addLayout(form);

    step_combo = new QComboBox{};
    step_combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    form->addRow(tr("Step:"), step_combo);

    amount_edit = new QDoubleSpinBox{};
    amount_edit->setMaximum(9999.999999);
    amount_edit->setValue(1.0);
    amount_edit->setDecimals(6);
    form->addRow(tr("Amount:"), amount_edit);

    dependencies_cb = new QCheckBox{tr("Include dependencies")};
    dependencies_cb->setChecked(true);
    main_layout->addWidget(dependencies_cb);

    auto button = new QPushButton{tr("Continue")};
    connect(button, &QPushButton::clicked, this, &ShoppingSetupDialog::openShoppingList);
    main_layout->addWidget(button, 0, Qt::AlignRight | Qt::AlignVCenter);
}

void ShoppingSetupDialog::openPlan(Plan& plan_)
{
    auto index = step_combo->currentIndex();
    step_combo->clear();

    auto names = plan_.stepsName(plan_.steps.size());
    names.prepend(tr("Final Step"));
    step_combo->addItems(names);
    QFont f;
    f.setItalic(true);
    step_combo->setItemData(0, f, Qt::FontRole);

    if (plan != &plan_) {
        plan = &plan_;
        step_combo->setCurrentIndex(0);
    } else
        step_combo->setCurrentIndex(index);

    open();

    auto title_pos = geometry().topLeft() - pos();
    auto new_pos = QCursor::pos() - title_pos - step_combo->pos();
    move(new_pos);
}

void ShoppingSetupDialog::openShoppingList()
{
    auto pos = step_combo->currentIndex();
    if (pos == 0) {
        auto it = plan->costStepIt();
        if (it == plan->steps.end()) {
            reject();
            return;
        }

        pos = std::distance(plan->steps.cbegin(), it);
    } else
        --pos;

    accept();
    AppState::state.shopping_dialog->openPlan(*plan,
                                              pos,
                                              amount_edit->value(),
                                              dependencies_cb->isChecked());
}

} // namespace planner
