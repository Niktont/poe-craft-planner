#include "StepWidget.h"
#include "AppState.h"
#include "CostWidget.h"
#include "DescriptionEdit.h"
#include "MainWidget.h"
#include "Plan.h"
#include "PlanWidget.h"
#include "Settings.h"
#include "StepCopyState.h"
#include "StepItemModel.h"
#include "StepItemView.h"
#include "StepItemsWidget.h"
#include <QCheckBox>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QToolBar>
#include <QVBoxLayout>

namespace planner {
StepWidget::StepWidget(PlanWidget& plan_widget, QWidget* parent)
    : QFrame{parent}
    , plan_widget{plan_widget}
{
    setFrameStyle(QFrame::Panel | QFrame::Raised);

    auto layout = new QVBoxLayout{};
    layout->setVerticalSizeConstraint(QLayout::SetFixedSize);
    setLayout(layout);

    auto title_layout = new QHBoxLayout{};
    layout->addLayout(title_layout);

    name_edit = new QLineEdit{this};
    title_layout->addWidget(name_edit);
    name_edit->setFixedWidth(280);
    connect(name_edit, &QLineEdit::editingFinished, this, &StepWidget::setNameFromEdit);

    cost_widget = new CostWidget{this};
    title_layout->addWidget(cost_widget);

    auto toolbar = new QToolBar{this};
    toolbar->setIconSize({16, 16});

    duplicate_action = addAction(tr("Duplicate"), this, [this]() {
        this->plan_widget.duplicateStep(step_pos);
    });
    copy_action = addAction(tr("Copy Reference"), this, [this]() {
        if (plan)
            StepCopyState::state = {plan->game, plan->id(), plan->steps[step_pos].id};
    });
    paste_action = addAction(tr("Paste"), this, [this]() { this->plan_widget.pasteStep(step_pos); });

    final_step_cb = new QCheckBox{this};
    final_step_cb->setToolTip(tr("Final"));
    toolbar->addWidget(final_step_cb);
    connect(final_step_cb, &QCheckBox::clicked, this, [this](bool checked) {
        this->plan_widget.setFinalStep(step_pos, checked);
    });

    move_up_action = toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowUp),
                                        tr("Move Up"),
                                        this,
                                        [this] { this->plan_widget.moveStep(step_pos, true); });
    move_up_action->setIconVisibleInMenu(false);

    move_down_action = toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowDown),
                                          tr("Move Down"),
                                          this,
                                          [this] { this->plan_widget.moveStep(step_pos, false); });
    move_down_action->setIconVisibleInMenu(false);

    delete_action = toolbar->addAction(style()->standardIcon(QStyle::SP_TitleBarCloseButton),
                                       tr("Delete"),
                                       this,
                                       &StepWidget::deleteStep);
    delete_action->setIconVisibleInMenu(false);

    title_layout->addStretch(1);

    title_layout->addWidget(toolbar);

    edit_widget = new QWidget{this};
    auto edit_layout = new QVBoxLayout{};
    edit_layout->setVerticalSizeConstraint(QLayout::SetFixedSize);
    edit_layout->setContentsMargins(0, 0, 0, 0);
    edit_widget->setLayout(edit_layout);
    layout->addWidget(edit_widget);

    description = new DescriptionEdit{edit_widget};
    connect(description,
            &DescriptionEdit::planLinkClicked,
            AppState::state.main_widget,
            &MainWidget::openPlanLink);

    edit_layout->addWidget(description);

    auto table_layout = new QVBoxLayout{};
    table_layout->setVerticalSizeConstraint(QLayout::SetFixedSize);
    edit_layout->addLayout(table_layout);
    table_layout->setContentsMargins(5, 0, 0, 0);

    resources_model = new StepItemModel{true, this};
    resources_widget = new StepItemsWidget{*resources_model, plan_widget, edit_widget};

    results_model = new StepItemModel{false, this};
    results_widget = new StepItemsWidget{*results_model, plan_widget, edit_widget};

    connect(resources_model,
            &StepItemModel::planLinkClicked,
            AppState::state.main_widget,
            &MainWidget::openPlanLink);
    connect(results_model,
            &StepItemModel::planLinkClicked,
            AppState::state.main_widget,
            &MainWidget::openPlanLink);

    connect(resources_model,
            &StepItemModel::stepLinkClicked,
            &plan_widget,
            &PlanWidget::scrollToStep);
    connect(results_model, &StepItemModel::stepLinkClicked, &plan_widget, &PlanWidget::scrollToStep);

    results_widget->setOtherView(*resources_widget);

    table_layout->addWidget(resources_widget);
    table_layout->addWidget(results_widget);
    connect(description->edit,
            &QPlainTextEdit::textChanged,
            this,
            &StepWidget::setDescriptionChanged);
}

Step& StepWidget::currentStep()
{
    assert(plan);
    return plan->steps[step_pos];
}

void StepWidget::displayCost()
{
    assert(plan);
    cost_widget->setCost(plan->game, &currentStep());
}

void StepWidget::setStep(Plan* plan_, size_t step_pos_)
{
    plan = plan_;
    step_pos = step_pos_;

    resources_model->setStep(plan, step_pos);
    results_model->setStep(plan, step_pos);

    resources_widget->setStep(plan, step_pos);
    results_widget->setStep(plan, step_pos);

    if (!plan)
        return;

    resources_widget->view->hideNotUsedItems();
    results_widget->view->hideNotUsedItems();
    if (!resources_widget->view->syncColumns())
        resources_widget->view->syncSize();

    hideEmptyResources();
    hideEmptyResults();

    auto& step = currentStep();
    setName(step.name);
    displayCost();

    description->edit->hide();

    is_description_changed = true;
    description->edit->setPlainText(step.description);
    is_description_changed = false;

    description->browser->show();
    description->browser->setMarkdown(step.description);

    description->adjustBrowserSize();

    updateMoveActions();
}

void StepWidget::setFinal(bool checked)
{
    final_step_cb->setChecked(checked);
}

void StepWidget::setName(QString name)
{
    if (name.isEmpty())
        name = tr("Step %1").arg(step_pos + 1);
    currentStep().name = name;
    name_edit->setText(name);
}

bool StepWidget::updateCost(bool current_updated)
{
    if (!plan)
        return false;

    bool size_changed = false;
    if (current_updated) {
        displayCost();
        size_changed = hideNotUsedItems();
    }

    resources_model->updateCosts();
    results_model->updateCosts();
    return size_changed;
}

void StepWidget::clearStep(const QUuid& deleted_step)
{
    resources_model->clearStep(deleted_step);
    results_model->clearStep(deleted_step);
}

void StepWidget::updateStepName(const QUuid& changed_step)
{
    resources_model->updateStepName(changed_step);
    results_model->updateStepName(changed_step);
}

void StepWidget::updatePlanName(const QUuid& changed_plan)
{
    resources_model->updatePlanName(changed_plan);
    results_model->updatePlanName(changed_plan);
}

void StepWidget::setDescription()
{
    if (!plan || !is_description_changed)
        return;

    currentStep().description = description->edit->toPlainText();
    is_description_changed = false;
}

void StepWidget::hideDescription()
{
    bool hide = Settings::get<Settings::windows_main_hide_descriptions>();
    if (hide != description->isHidden())
        description->setHidden(hide);
}

bool StepWidget::hideEmptyResources()
{
    if (!plan)
        return false;

    bool hide = currentStep().resources.empty()
                && Settings::get<Settings::windows_main_hide_empty_resources>();
    if (hide != resources_widget->isHidden()) {
        resources_widget->setHidden(hide);
        return true;
    }
    return false;
}

bool StepWidget::hideEmptyResults()
{
    if (!plan)
        return false;

    bool hide = currentStep().results.empty()
                && Settings::get<Settings::windows_main_hide_empty_results>();
    if (hide != results_widget->isHidden()) {
        results_widget->setHidden(hide);
        return true;
    }
    return false;
}

bool StepWidget::hideNotUsedItems()
{
    if (!plan)
        return false;

    bool resources_changed = resources_widget->view->hideNotUsedItems();
    bool results_changed = results_widget->view->hideNotUsedItems();
    bool size_changed = resources_changed || results_changed;

    if (size_changed && !resources_widget->view->syncColumns()) {
        if (resources_changed)
            resources_widget->view->setFixedSize(resources_widget->view->sizeHint());
        if (results_changed)
            results_widget->view->setFixedSize(results_widget->view->sizeHint());
    }
    return size_changed;
}

void StepWidget::hideTitleCurrencyName()
{
    cost_widget->hideCurrencyName();
}

void StepWidget::updateMoveActions()
{
    move_up_action->setEnabled(step_pos != 0);
    move_down_action->setEnabled(static_cast<long long>(step_pos) < std::ssize(plan->steps) - 1);
}

void StepWidget::deleteStep()
{
    auto modifiers = QGuiApplication::keyboardModifiers();
    bool delete_step = modifiers.testFlag(Qt::ShiftModifier);
    if (!delete_step) {
        QMessageBox msg{this};
        msg.setWindowTitle(tr("Delete Step"));
        msg.setText(tr("Delete \"%1\"?").arg(currentStep().name));
        msg.addButton(QMessageBox::Ok);
        msg.addButton(QMessageBox::Cancel);
        delete_step = msg.exec() == QMessageBox::Ok;
    }
    if (delete_step)
        plan_widget.deleteStep(step_pos);
}

void StepWidget::setNameFromEdit()
{
    if (!plan)
        return;

    auto& step = currentStep();
    auto name = name_edit->text();
    if (name != step.name) {
        step.name = name;
        plan->setChanged();
        plan_widget.updateStepNames(step_pos);
    }
}

void StepWidget::setDescriptionChanged()
{
    assert(plan);
    if (!is_description_changed) {
        plan->setChanged();
        is_description_changed = true;
    }
}

void StepWidget::contextMenuEvent(QContextMenuEvent* event)
{
    if (!plan)
        return;

    auto menu = new QMenu{this};
    menu->setAttribute(Qt::WA_DeleteOnClose);

    menu->addAction(duplicate_action);
    menu->addAction(copy_action);
    if (StepCopyState::haveCopy(plan->game))
        menu->addAction(paste_action);
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

void StepWidget::clearTradeRequest(const TradeRequestKey& request)
{
    resources_model->clearTradeRequest(request);
    results_model->clearTradeRequest(request);
}

} // namespace planner
