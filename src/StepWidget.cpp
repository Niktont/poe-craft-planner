#include "StepWidget.h"
#include "CostWidget.h"
#include "DescriptionEdit.h"
#include "Plan.h"
#include "PlanWidget.h"
#include "StepItemDelegate.h"
#include "StepItemView.h"
#include "StepItemsWidget.h"
#include <QAbstractTextDocumentLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QTableView>
#include <QTextBrowser>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>

namespace planner {
StepWidget::StepWidget(PlanWidget* plan_widget, QWidget* parent)
    : QFrame{parent}
    , plan_widget{plan_widget}
{
    setFrameStyle(QFrame::Panel | QFrame::Raised);

    auto layout = new QVBoxLayout{};
    setLayout(layout);

    auto title_layout = new QHBoxLayout{};
    layout->addLayout(title_layout);

    name_edit = new QLineEdit{};
    name_edit->setMaximumWidth(240);
    name_edit->setMaxLength(30);
    title_layout->addWidget(name_edit);
    connect(name_edit, &QLineEdit::editingFinished, this, &StepWidget::setNameFromEdit);

    cost_widget = new CostWidget(*plan_widget->mw());
    title_layout->addWidget(cost_widget);

    auto toolbar = new QToolBar{};
    toolbar->setIconSize({16, 16});

    duplicate_action = addAction(tr("Duplicate"), this, [this]() {
        this->plan_widget->duplicateStep(step_pos);
    });

    move_up_action = toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowUp),
                                        tr("Move Up"),
                                        this,
                                        [this] { this->plan_widget->moveStep(step_pos, true); });
    move_up_action->setIconVisibleInMenu(false);

    move_down_action = toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowDown),
                                          tr("Move Down"),
                                          this,
                                          [this] { this->plan_widget->moveStep(step_pos, false); });
    move_down_action->setIconVisibleInMenu(false);

    delete_action = toolbar->addAction(style()->standardIcon(QStyle::SP_TitleBarCloseButton),
                                       tr("Delete"),
                                       this,
                                       &StepWidget::deleteStep);
    delete_action->setIconVisibleInMenu(false);

    title_layout->addStretch(1);

    title_layout->addWidget(toolbar);

    edit_widget = new QWidget{};
    auto step_layout = new QBoxLayout{QBoxLayout::TopToBottom};
    edit_widget->setLayout(step_layout);
    layout->addWidget(edit_widget);
    step_layout->setContentsMargins(0, 0, 0, 0);

    description = new DescriptionEdit{this};
    connect(description->edit, &QPlainTextEdit::textChanged, this, [this] {
        if (!is_text_reset)
            this->plan_widget->setPlanChanged();
    });

    step_layout->addWidget(description);

    auto table_layout = new QVBoxLayout{};
    step_layout->addLayout(table_layout);
    table_layout->setContentsMargins(5, 0, 0, 0);

    resources_model = new StepItemModel{true, *plan_widget};
    resources_widget = new StepItemsWidget{*resources_model};

    results_model = new StepItemModel{false, *plan_widget};
    results_widget = new StepItemsWidget{*results_model};

    results_widget->setOtherView(*resources_widget);

    table_layout->addWidget(resources_widget);
    table_layout->addWidget(results_widget);
}

Step* StepWidget::currentStep()
{
    return plan ? &plan->steps[step_pos] : nullptr;
}

void StepWidget::displayCost()
{
    if (!plan)
        return;

    cost_widget->setCost(plan->game, currentStep());
}

void StepWidget::setStep(Plan* plan, size_t step_pos)
{
    this->plan = plan;
    this->step_pos = step_pos;

    resources_model->setStep(plan, step_pos);
    results_model->setStep(plan, step_pos);

    resources_widget->setStep(plan, step_pos);
    results_widget->setStep(plan, step_pos);

    if (!plan)
        return;

    resources_widget->view->syncColumns();

    auto step = currentStep();
    setName(step->name);

    description->edit->hide();
    is_text_reset = true;
    description->edit->setPlainText(step->description);
    is_text_reset = false;

    description->browser->show();
    description->browser->setMarkdown(step->description);
    //description->browser->document()->setTextWidth(description->edit->maximumWidth());

    description->adjustBrowserSize();

    move_up_action->setEnabled(step_pos != 0);
    move_down_action->setEnabled(static_cast<long long>(step_pos) < std::ssize(plan->steps) - 1);
}

void StepWidget::setName(QString name)
{
    auto step = currentStep();
    if (name.isEmpty())
        name = tr("Step %1").arg(step_pos + 1);
    step->name = name;
    name_edit->setText(name);
}

void StepWidget::updateCost(Plan* plan)
{
    if (!plan || plan != this->plan)
        return;

    displayCost();

    resources_model->updateCosts();
    results_model->updateCosts();
}

void StepWidget::updateStepNames(const QUuid& changed_step, bool deleted)
{
    resources_model->updateStepName(changed_step, deleted);
    results_model->updateStepName(changed_step, deleted);
}

void StepWidget::setDescription()
{
    auto step = currentStep();
    if (!step)
        return;

    step->description = description->edit->toPlainText();
}

void StepWidget::hideDescription(bool hide)
{
    description->setHidden(hide);
}

void StepWidget::deleteStep()
{
    auto modifiers = QGuiApplication::keyboardModifiers();
    bool delete_step = (modifiers & Qt::ShiftModifier);
    if (!delete_step) {
        QMessageBox msg;
        msg.setWindowTitle(tr("Delete Step"));
        msg.setText(tr("Delete \"%1\"?").arg(currentStep()->name));
        msg.addButton(QMessageBox::Ok);
        msg.addButton(QMessageBox::Cancel);
        delete_step = msg.exec() == QMessageBox::Ok;
    }
    if (delete_step)
        plan_widget->deleteStep(step_pos);
}

void StepWidget::setNameFromEdit()
{
    if (!plan)
        return;
    auto step = currentStep();
    auto name = name_edit->text();
    if (name != step->name) {
        step->name = name;
        plan->setChanged();
        planWidget()->updateStepNames(step_pos);
    }
}

void StepWidget::contextMenuEvent(QContextMenuEvent* event)
{
    auto menu = new QMenu{this};
    menu->setAttribute(Qt::WA_DeleteOnClose);

    menu->addAction(duplicate_action);
    menu->addAction(move_up_action);
    menu->addAction(move_down_action);

    menu->addSeparator();
    menu->addAction(delete_action);

    menu->popup(event->globalPos());
}

void StepWidget::updatePos(size_t new_pos)
{
    step_pos = new_pos;
    resources_model->updatePos(new_pos);
    results_model->updatePos(new_pos);
    resources_widget->updatePos(new_pos);
    results_widget->updatePos(new_pos);

    move_up_action->setEnabled(step_pos != 0);
    move_down_action->setEnabled(static_cast<long long>(step_pos) < std::ssize(plan->steps) - 1);
}

void StepWidget::updateTradeName(const TradeRequestKey& request)
{
    resources_model->updateTradeName(request);
    results_model->updateTradeName(request);
}

void StepWidget::updateTradeTime(const TradeRequestKey& request)
{
    resources_model->updateTime(request);
}

void StepWidget::updateCurrencyTime(const Currency& currency)
{
    resources_model->updateTime(currency);
}

} // namespace planner
