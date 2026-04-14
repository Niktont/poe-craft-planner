#include "PlanTitleWidget.h"
#include "CostWidget.h"
#include "Plan.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>

namespace planner {

PlanTitleWidget::PlanTitleWidget(QWidget* parent)
    : QWidget{parent}
{
    auto layout = new QHBoxLayout{};
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    name_label = new QLabel{};
    auto f = name_label->font();
    f.setBold(true);
    name_label->setFont(f);
    layout->addWidget(name_label);

    is_auto_final_cb = new QCheckBox{tr("Auto")};
    is_auto_final_cb->setToolTip(tr("On costs update, set step with max profit as Final"));
    layout->addWidget(is_auto_final_cb);

    locked_cb = new QCheckBox{tr("Lock")};
    locked_cb->setToolTip(tr("Steps costs of locked plan won't be updated"));
    layout->addWidget(locked_cb);

    league_label = new QLabel{};
    f = league_label->font();
    f.setItalic(true);
    league_label->setFont(f);
    layout->addWidget(league_label);

    cost_widget = new CostWidget{};
    layout->addWidget(cost_widget);

    layout->addStretch(1);
}

void PlanTitleWidget::setPlan(const Plan& plan)
{
    name_label->setText(plan.name);
    is_auto_final_cb->setChecked(plan.is_auto_final);
    locked_cb->setChecked(plan.locked);
    league_label->setText(plan.league);
    show();
}

} // namespace planner
