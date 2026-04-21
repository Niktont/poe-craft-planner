#include "PlanWidget.h"
#include "AppState.h"
#include "CostWidget.h"
#include "CustomEditDialog.h"
#include "ExchangeRequestCache.h"
#include "MainWindow.h"
#include "Plan.h"
#include "PlanItem.h"
#include "PlanModel.h"
#include "PlanTitleWidget.h"
#include "RequestEditDialog.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "ShoppingDialog.h"
#include "ShoppingSetupDialog.h"
#include "SnapshotModel.h"
#include "StepCopyState.h"
#include "StepItem.h"
#include "StepWidget.h"
#include "TradeRequestCache.h"
#include "UpdateCostDialog.h"
#include <QCheckBox>
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

namespace planner {
PlanWidget::PlanWidget(bool is_main, QWidget* parent, Qt::WindowFlags flags)
    : QWidget{parent, flags}
    , is_main{is_main}
{
    setFocusPolicy(Qt::ClickFocus);
    setAttribute(Qt::WA_StyledBackground);
    if (is_main)
        setEnabled(false);
    else {
        setAttribute(Qt::WA_DeleteOnClose);
        setAttribute(Qt::WA_QuitOnClose, false);
    }

    auto layout = new QVBoxLayout{};
    setLayout(layout);

    title_widget = new PlanTitleWidget{this};
    connect(title_widget->is_auto_final_cb, &QCheckBox::clicked, this, [this](bool checked) {
        if (!plan_)
            return;

        plan_->is_auto_final = checked;
        plan_->setChanged();
    });
    connect(title_widget->locked_cb, &QCheckBox::clicked, this, [this](bool checked) {
        if (!plan_)
            return;

        plan_->locked = checked;
        plan_->setChanged();
    });
    title_widget->hide();
    layout->addWidget(title_widget);

    steps_scroll = new QScrollArea{this};
    steps_scroll->setWidgetResizable(true);
    steps_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    steps_scroll->setFrameShape(QFrame::Box);
    steps_scroll->setFrameShadow(QFrame::Plain);
    steps_scroll->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(steps_scroll);

    steps_widget = new QWidget{steps_scroll};
    auto steps_layout = new QVBoxLayout{};
    steps_layout->setVerticalSizeConstraint(QLayout::SetFixedSize);
    steps_layout->setSpacing(0);
    steps_layout->setContentsMargins(0, 0, 0, 0);
    steps_widget->setLayout(steps_layout);

    steps_layout->addStretch(1);

    steps_scroll->setWidget(steps_widget);

    add_step_action = addAction(tr("Add Step"), this, &PlanWidget::addStep);

    paste_step_action = addAction(tr("Paste Step"), this, [this] {
        if (plan_)
            pasteStep(plan_->steps.size());
    });

    update_costs_action = addAction(tr("Update Costs"), this, [this] {
        if (!plan_)
            return;
        bool send_requests = !QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
        AppState::state.update_cost_dialog->updatePlan(*plan_, send_requests);
    });
    shopping_mode_action = addAction(tr("Shopping Mode"), this, &PlanWidget::openShoppingDialog);
}

void PlanWidget::connectSignals()
{
    if (!is_main) {
        connect(AppState::state.plan_model_poe1,
                &PlanModel::rowsAboutToBeRemoved,
                this,
                &PlanWidget::checkDeletingItems);
        connect(AppState::state.plan_model_poe2,
                &PlanModel::rowsAboutToBeRemoved,
                this,
                &PlanWidget::checkDeletingItems);
    }

    connect(AppState::state.plan_model_poe1,
            &PlanModel::planUpdated,
            this,
            &PlanWidget::setPlanOnUpdate);
    connect(AppState::state.plan_model_poe2,
            &PlanModel::planUpdated,
            this,
            &PlanWidget::setPlanOnUpdate);

    connect(AppState::state.plan_model_poe1,
            &PlanModel::planRenamed,
            this,
            &PlanWidget::updatePlanName);
    connect(AppState::state.plan_model_poe2,
            &PlanModel::planRenamed,
            this,
            &PlanWidget::updatePlanName);

    connect(AppState::state.plan_model_poe1,
            &PlanModel::descriptionsNeeded,
            this,
            &PlanWidget::setDescriptions);
    connect(AppState::state.plan_model_poe2,
            &PlanModel::descriptionsNeeded,
            this,
            &PlanWidget::setDescriptions);

    connect(AppState::state.trade_cache_poe1,
            &TradeRequestCache::rowsAboutToBeRemoved,
            this,
            &PlanWidget::checkDeletingTradeRequests);
    connect(AppState::state.trade_cache_poe2,
            &TradeRequestCache::rowsAboutToBeRemoved,
            this,
            &PlanWidget::checkDeletingTradeRequests);

    connect(AppState::state.trade_cache_poe1,
            &TradeRequestCache::dataChanged,
            this,
            &PlanWidget::updateTradeRequests);
    connect(AppState::state.trade_cache_poe2,
            &TradeRequestCache::dataChanged,
            this,
            &PlanWidget::updateTradeRequests);

    connect(AppState::state.exchange_cache_poe1,
            &ExchangeRequestCache::defaultTimeChanged,
            this,
            &PlanWidget::updateCurrencyTime);
    connect(AppState::state.exchange_cache_poe2,
            &ExchangeRequestCache::defaultTimeChanged,
            this,
            &PlanWidget::updateCurrencyTime);

    connect(AppState::state.mw->settings_dialog,
            &SettingsDialog::tradeTimeChanged,
            this,
            &PlanWidget::updateTradeDefaultTime);
    connect(AppState::state.mw->settings_dialog,
            &SettingsDialog::exchangeTimeChanged,
            this,
            &PlanWidget::updateExchangeDefaultTime);

    connect(AppState::state.snapshots_poe1,
            &SnapshotModel::currentChanged,
            this,
            &PlanWidget::updateOnSnapshotChange);
    connect(AppState::state.snapshots_poe2,
            &SnapshotModel::currentChanged,
            this,
            &PlanWidget::updateOnSnapshotChange);
}

Game PlanWidget::game() const
{
    return plan_ ? plan_->game : Game::Unknown;
}

void PlanWidget::addStep()
{
    if (!plan_)
        return;

    auto pos = plan_->steps.size();
    plan_->steps.emplace_back();
    plan_->setChanged();

    reuseStepWidget(pos);
    QTimer::singleShot(0, this, [this, pos] {
        steps_scroll->ensureWidgetVisible(step_widgets[pos], 0);
    });

    if (plan_->finalStepId().isNull())
        updateDisplayedCost();
}

void PlanWidget::emplaceStepWidget(size_t i)
{
    auto step_widget = *step_widgets.emplace(step_widgets.begin() + i,
                                             new StepWidget{*this, steps_widget});
    static_cast<QVBoxLayout*>(steps_widget->layout())->insertWidget(i, step_widget);
    step_widget->setStep(plan_, i);
    step_widget->hideDescription();
}

void PlanWidget::reuseStepWidget(size_t i)
{
    if (i < step_widgets.size()) {
        step_widgets[i]->setStep(plan_, i);
        step_widgets[i]->show();
    } else
        emplaceStepWidget(i);
}

void PlanWidget::displayCost()
{
    title_widget->cost_widget->setCost(plan_->game, plan_->costStep());
}

PlanWidget::ScrollState PlanWidget::scrollState()
{
    ScrollState result;
    result.focus_widget = steps_widget->focusWidget();
    if (plan_->steps.size() < 2)
        return result;

    result.y_diff = steps_scroll->verticalScrollBar()->value();

    result.step_widget = result.focus_widget;
    while (result.step_widget && !qobject_cast<StepWidget*>(result.step_widget))
        result.step_widget = result.step_widget->parentWidget();

    if (!result.step_widget) {
        auto middle_y = std::min(steps_widget->height(),
                                 steps_scroll->verticalScrollBar()->value()
                                     + steps_scroll->height() / 2);
        result.step_widget = steps_widget->childAt(0, middle_y);
        if (!result.step_widget)
            result.step_widget = step_widgets[plan_->steps.size() - 1];
    }
    result.y_diff -= result.step_widget->y();

    return result;
}

void PlanWidget::restoreScrollState(const ScrollState& state)
{
    if (state.step_widget) {
        QTimer::singleShot(0, this, [this, w = state.step_widget, yd = state.y_diff] {
            steps_scroll->verticalScrollBar()->setValue(yd + w->y());
        });
    }
}

template<auto fun, typename... Args>
void PlanWidget::applyResizeFunction(Args&&... args)
{
    auto state = scrollState();

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->hide();

    bool size_changed = false;
    for (size_t i = 0; i < plan_->steps.size(); ++i) {
        size_changed |= (step_widgets[i]->*fun)(std::forward<Args>(args)...);
        step_widgets[i]->show();
    }

    if (size_changed)
        steps_widget->adjustSize();

    restoreScrollState(state);
    if (state.focus_widget)
        state.focus_widget->setFocus();
}

void PlanWidget::updateCosts(bool current_updated)
{
    if (current_updated && Settings::get<settings::windows_main_hide_not_used_items>())
        applyResizeFunction<&StepWidget::updateCost>(true);
    else {
        for (size_t i = 0; i < plan_->steps.size(); ++i)
            step_widgets[i]->updateCost(current_updated);
    }
}

void PlanWidget::clear()
{
    if (!is_main) {
        close();
        return;
    }

    setEnabled(false);
    plan_ = nullptr;

    title_widget->hide();

    for (size_t i = 0; i < step_widgets.size(); ++i) {
        step_widgets[i]->hide();
        step_widgets[i]->setStep(nullptr, 0);
    }
}

void PlanWidget::updateCost(Game game, const std::vector<std::pair<Plan*, bool>>& updated_plans)
{
    if (!plan_ || plan_->game != game)
        return;

    auto it = std::ranges::find(updated_plans, plan_, &std::pair<Plan*, bool>::first);
    bool current_updated = false;
    bool final_changed = false;
    if (it != updated_plans.end()) {
        current_updated = true;
        final_changed = it->second;
    }

    if (current_updated) {
        title_widget->league_label->setText(plan_->league);
        displayCost();
        if (final_changed)
            displayFinalStep();
    }

    updateCosts(current_updated);
}

void PlanWidget::setDescriptions(Game game, const Plan* target_plan)
{
    if (!plan_ || plan_->game != game || (target_plan && plan_ != target_plan))
        return;

    setStepDescriptions();
}

void PlanWidget::setStepDescriptions()
{
    if (!plan_->is_changed)
        return;

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->setDescription();
}

void PlanWidget::deleteStep(size_t step_pos)
{
    if (!plan_ || step_pos >= plan_->steps.size())
        return;

    for (size_t i = step_pos + 1; i < plan_->steps.size(); ++i)
        step_widgets[i]->updatePos(step_widgets[i]->stepPos() - 1);

    auto widget = step_widgets[step_pos];
    step_widgets.erase(step_widgets.begin() + step_pos);
    step_widgets.push_back(widget);

    steps_widget->layout()->removeWidget(widget);
    widget->hide();
    widget->setStep(nullptr, 0);
    static_cast<QVBoxLayout*>(steps_widget->layout())->insertWidget(step_widgets.size() - 1, widget);

    bool is_final_changed = &plan_->steps[step_pos] == plan_->costStep();
    if (is_final_changed)
        plan_->setFinalStep({});

    auto deleted_id = plan_->steps[step_pos].id;

    plan_->steps.erase(plan_->steps.begin() + step_pos);
    plan_->setChanged();

    if (step_pos > 0) {
        if (step_pos == plan_->steps.size())
            step_widgets[step_pos - 1]->updateMoveActions();
    } else if (!plan_->steps.empty())
        step_widgets[step_pos]->updateMoveActions();

    for (size_t i = step_pos; i < plan_->steps.size(); ++i)
        step_widgets[i]->clearStep(deleted_id);

    if (StepCopyState::state.step_id == deleted_id)
        StepCopyState::state.game = Game::Unknown;

    if (is_final_changed) {
        updateDisplayedCost();
        displayFinalStep();
    }
}

void PlanWidget::duplicateStep(size_t step_pos)
{
    if (!plan_ || step_pos >= plan_->steps.size())
        return;
    auto copy_pos = step_pos + 1;
    for (size_t i = copy_pos; i < plan_->steps.size(); ++i)
        step_widgets[i]->updatePos(step_widgets[i]->stepPos() + 1);

    auto step_it = plan_->steps.emplace(plan_->steps.begin() + copy_pos, plan_->steps[step_pos]);
    step_it->name += tr(" - Copy");
    plan_->setChanged();

    step_widgets[step_pos]->updateMoveActions();

    if (plan_->steps.size() < step_widgets.size()) {
        auto widget = step_widgets.back();
        step_widgets.pop_back();
        step_widgets.insert(step_widgets.begin() + copy_pos, widget);
        steps_widget->layout()->removeWidget(widget);
        static_cast<QVBoxLayout*>(steps_widget->layout())->insertWidget(copy_pos, widget);
        widget->setStep(plan_, copy_pos);
        widget->show();
    } else
        emplaceStepWidget(copy_pos);

    QTimer::singleShot(0, this, [this, copy_pos] {
        steps_scroll->ensureWidgetVisible(step_widgets[copy_pos], 0);
    });
}

void PlanWidget::setFinalStep(size_t step_pos, bool checked)
{
    if (!plan_ || step_pos >= plan_->steps.size())
        return;

    auto old_final = plan_->costStepIt();
    if (checked)
        plan_->setFinalStep(plan_->steps[step_pos].id);
    else
        plan_->setFinalStep({});

    if (auto new_final = plan_->costStepIt(); new_final != old_final)
        updateDisplayedCost();

    displayFinalStep();
}

void PlanWidget::scrollToStep(const QUuid& step_id)
{
    if (!plan_)
        return;

    auto it = plan_->findStepIt(step_id);
    if (it == plan_->steps.end())
        return;

    auto pos = std::distance(plan_->steps.cbegin(), it);
    steps_scroll->ensureWidgetVisible(step_widgets[pos], 0);
}

void PlanWidget::pasteStep(size_t step_pos)
{
    if (!plan_ || !StepCopyState::haveCopy(plan_->game))
        return;

    auto& plans = AppState::planModel(plan_->game)->plans;
    auto source_plan_it = plans.find(StepCopyState::state.plan_id);
    if (source_plan_it == plans.end())
        return;
    auto& source_plan = source_plan_it->second;

    auto source_step_it = source_plan.findStepIt(StepCopyState::state.step_id);
    if (source_step_it == source_plan.steps.cend())
        return;

    step_pos = std::min(step_pos, plan_->steps.size());
    bool is_last_changed = step_pos == plan_->steps.size();

    size_t copy_size;
    if (&source_plan_it->second != plan_) {
        std::set<Plan::Steps::const_iterator> steps_to_copy;
        auto add_step_item = [&](const auto& add_step, const StepItem& item) {
            if (auto step_item = item.step()) {
                auto it = source_plan.findStepIt(step_item->step_id);
                if (it == source_plan.steps.cend())
                    return;
                add_step(add_step, it);
            }
        };
        auto add_step = [&](const auto& self, Plan::Steps::const_iterator step_it) {
            if (!steps_to_copy.emplace(step_it).second)
                return;

            for (auto& item : step_it->resources)
                add_step_item(self, item);
            for (auto& item : step_it->results)
                add_step_item(self, item);
        };
        add_step(add_step, source_step_it);

        copy_size = steps_to_copy.size();
        for (size_t i = step_pos; i < plan_->steps.size(); ++i)
            step_widgets[i]->updatePos(step_widgets[i]->stepPos() + copy_size);

        std::vector<Step> copied_steps;
        copied_steps.reserve(copy_size);
        boost::container::flat_map<QUuid, QUuid> changed_ids;
        for (auto& it : steps_to_copy) {
            auto id_it = changed_ids.try_emplace(it->id).first;
            id_it->second = copied_steps.emplace_back(*it).id;
        }
        for (auto& step : copied_steps) {
            step.updateIds(changed_ids);
            step.name = step.name + tr(" - Copy");
        }
        auto move_begin = std::move_iterator(copied_steps.begin());
        auto move_end = std::move_iterator(copied_steps.end());
        plan_->steps.insert(plan_->steps.begin() + step_pos, move_begin, move_end);
    } else {
        copy_size = 1;
        auto it = plan_->steps.insert(plan_->steps.begin() + step_pos, *source_step_it);
        it->name = it->name + tr(" - Copy");
    }
    plan_->setChanged();

    if (step_pos > 0) {
        if (is_last_changed)
            step_widgets[step_pos - 1]->updateMoveActions();
    } else if (!step_widgets.empty())
        step_widgets[step_pos]->updateMoveActions();

    if (plan_->steps.size() < step_widgets.size()) {
        for (size_t i = step_pos; i < step_pos + copy_size; ++i) {
            auto widget = step_widgets.back();
            step_widgets.pop_back();
            step_widgets.insert(step_widgets.begin() + i, widget);
            steps_widget->layout()->removeWidget(widget);
            static_cast<QVBoxLayout*>(steps_widget->layout())->insertWidget(i, widget);
            widget->setStep(plan_, i);
            widget->show();
        }
    } else {
        for (size_t i = step_pos; i < step_pos + copy_size; ++i)
            emplaceStepWidget(i);
    }

    if (plan_->finalStepId().isNull() && is_last_changed)
        updateDisplayedCost();
}

CustomEditDialog& PlanWidget::customEdit()
{
    if (!custom_edit_dialog)
        custom_edit_dialog = new CustomEditDialog{this};
    return *custom_edit_dialog;
}

ShoppingSetupDialog& PlanWidget::shoppingSetup()
{
    if (!shopping_setup)
        shopping_setup = new ShoppingSetupDialog{this};
    return *shopping_setup;
}

RequestEditDialog& PlanWidget::requestEdit()
{
    if (!request_edit)
        request_edit = new RequestEditDialog{this};
    return *request_edit;
}

void PlanWidget::displayFinalStep()
{
    for (auto step_widget : step_widgets)
        step_widget->setFinal(false);

    if (plan_->finalStepId().isNull())
        return;

    if (auto it = plan_->costStepIt(); it != plan_->steps.cend()) {
        auto pos = std::distance(plan_->steps.cbegin(), it);
        step_widgets[pos]->setFinal(true);
    }
}

void PlanWidget::updateDisplayedCost()
{
    if (!plan_)
        return;

    displayCost();

    auto idx = plan_->item()->index();
    AppState::planModel(plan_->game)->updateCost(idx);
}

void PlanWidget::moveStep(size_t step_pos, bool up)
{
    if (!plan_ || step_pos >= plan_->steps.size())
        return;

    auto widget_to_move = step_widgets[step_pos];

    size_t bottom_pos;
    if (up) {
        if (step_pos == 0)
            return;
        bottom_pos = step_pos;
        step_pos = step_pos - 1;
    } else {
        if (static_cast<long long>(step_pos) == std::ssize(plan_->steps) - 1)
            return;
        bottom_pos = step_pos + 1;
    }
    auto widget = step_widgets[step_pos];
    auto bottom_widget = step_widgets[bottom_pos];

    steps_widget->layout()->removeWidget(bottom_widget);

    widget->updatePos(bottom_pos);
    bottom_widget->updatePos(step_pos);

    widget->updateMoveActions();
    bottom_widget->updateMoveActions();

    auto it = plan_->steps.begin() + step_pos;
    auto bottom_it = plan_->steps.begin() + bottom_pos;
    std::iter_swap(it, bottom_it);
    plan_->setChanged();

    auto it_w = step_widgets.begin() + step_pos;
    auto bottom_it_w = step_widgets.begin() + bottom_pos;
    std::iter_swap(it_w, bottom_it_w);

    auto prev_y = widget_to_move->y();
    static_cast<QVBoxLayout*>(steps_widget->layout())->insertWidget(step_pos, bottom_widget);
    bool is_final_changed = plan_->finalStepId().isNull() && (bottom_it + 1) == plan_->steps.end();
    if (is_final_changed)
        updateDisplayedCost();

    QTimer::singleShot(0, this, [this, widget_to_move, prev_y] {
        steps_scroll->verticalScrollBar()->setValue(steps_scroll->verticalScrollBar()->value()
                                                    + widget_to_move->y() - prev_y);
    });
}

void PlanWidget::updateStepNames(size_t renamed_step)
{
    if (!plan_)
        return;

    auto& step = plan_->steps[renamed_step];
    for (size_t i = renamed_step + 1; i < plan_->steps.size(); ++i)
        step_widgets[i]->updateStepName(step.id);
}

void PlanWidget::updatePlanName(const Plan& renamed_plan)
{
    if (&renamed_plan == plan_) {
        title_widget->name_label->setText(plan_->name);
        if (!is_main)
            setWindowTitle(renamed_plan.name + " - " APP_NAME);
    }

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->updatePlanName(renamed_plan.id());
}

void PlanWidget::updateTradeRequests(const QModelIndex& top_left, const QModelIndex& bottom_right)
{
    if (!plan_)
        return;

    auto model = static_cast<const TradeRequestCache*>(top_left.model());
    if (plan_->game != model->game)
        return;

    auto col_left = static_cast<TradeRequestColumn>(top_left.column());
    auto col_right = static_cast<TradeRequestColumn>(bottom_right.column());
    if (col_left <= TradeRequestColumn::Name && TradeRequestColumn::Name <= col_right)
        updateTradeName(top_left.row(), *model);
    if (col_left <= TradeRequestColumn::Time && TradeRequestColumn::Time <= col_right)
        updateTradeTime(top_left.row(), *model);
}

void PlanWidget::checkDeletingTradeRequests(const QModelIndex&, int first, int last)
{
    if (!plan_)
        return;

    auto model = static_cast<TradeRequestCache*>(sender());
    if (plan_->game != model->game)
        return;

    for (int i = first; i <= last; ++i) {
        auto it = model->cache.nth(i);
        for (size_t j = 0; j < plan_->steps.size(); ++j)
            step_widgets[j]->clearTradeRequest(it->first);
    }
}

void PlanWidget::updateTradeName(int row, const TradeRequestCache& cache)
{
    auto it = cache.cache.nth(row);
    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->updateTradeName(it->first);
}

void PlanWidget::updateTradeTime(int row, const TradeRequestCache& cache)
{
    auto it = cache.cache.nth(row);
    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->updateTradeTime(it->first);
}

void PlanWidget::updateCurrencyTime(const Currency& currency)
{
    if (!plan_)
        return;

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->updateCurrencyTime(currency);
}

void PlanWidget::updateTradeDefaultTime()
{
    if (!plan_)
        return;

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->updateTradeTime({});
}

void PlanWidget::updateExchangeDefaultTime()
{
    if (!plan_)
        return;

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->updateCurrencyTime({});
}

void PlanWidget::updateOnSnapshotChange(Game game)
{
    if (plan_ && plan_->game == game)
        updateCosts(false);
}

void PlanWidget::openShoppingDialog()
{
    if (!plan_ || plan_->steps.empty())
        return;

    bool have_resources = std::ranges::find_if(plan_->steps,
                                               [](const auto& step) {
                                                   return !step.resources.empty();
                                               })
                          != plan_->steps.end();
    if (!have_resources)
        return;

    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
        shoppingSetup().openPlan(*plan_);
    else
        AppState::state.shopping_dialog->openPlan(*plan_);
}

void PlanWidget::hideDescriptions()
{
    if (!plan_) {
        for (auto step : step_widgets)
            step->hideDescription();
        return;
    }

    auto state = scrollState();

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->hide();

    size_t i = 0;
    for (; i < plan_->steps.size(); ++i) {
        step_widgets[i]->hideDescription();
        step_widgets[i]->show();
    }
    for (; i < step_widgets.size(); ++i)
        step_widgets[i]->hideDescription();

    steps_widget->adjustSize();
    restoreScrollState(state);
    if (state.focus_widget)
        state.focus_widget->setFocus();
}

void PlanWidget::hideEmptyResources()
{
    if (!plan_)
        return;

    applyResizeFunction<&StepWidget::hideEmptyResources>();
}

void PlanWidget::hideEmptyResults()
{
    if (!plan_)
        return;

    applyResizeFunction<&StepWidget::hideEmptyResults>();
}

void PlanWidget::hideNotUsedItems()
{
    if (!plan_)
        return;

    applyResizeFunction<&StepWidget::hideNotUsedItems>();
}

void PlanWidget::hideTitleCurrencyName()
{
    if (!plan_)
        return;

    title_widget->cost_widget->hideCurrencyName();
    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->hideTitleCurrencyName();
}

void PlanWidget::contextMenuEvent(QContextMenuEvent* event)
{
    if (!plan_)
        return;

    auto menu = new QMenu{this};
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->addAction(add_step_action);
    if (StepCopyState::haveCopy(plan_->game))
        menu->addAction(paste_step_action);

    menu->addSeparator();
    menu->addAction(update_costs_action);
    menu->addAction(shopping_mode_action);

    menu->popup(event->globalPos());
}

void PlanWidget::closeEvent(QCloseEvent* event)
{
    if (!is_main) {
        emit aboutToClose(*this);
        event->accept();
    }
}

void PlanWidget::setPlanChanged()
{
    if (plan_)
        plan_->setChanged();
}

void PlanWidget::setPlanOnUpdate(Plan& updated_plan)
{
    if (&updated_plan != plan_)
        return;

    setPlan(&updated_plan, true);
}

bool PlanWidget::checkDeletingPlans(const PlanItem& parent_item, int first, int last)
{
    return plan_ && plan_->game == parent_item.game()
           && parent_item.isDescendantDeleting(*plan_->item(), first, last);
}

void PlanWidget::checkDeletingItems(const QModelIndex& parent, int first, int last)
{
    auto model = static_cast<PlanModel*>(sender());
    auto parent_item = model->internalPtr(parent);

    if (checkDeletingPlans(*parent_item, first, last))
        setPlan(nullptr);
}

void PlanWidget::setPlan(Plan* plan, bool is_update)
{
    if (shopping_setup && shopping_setup->isVisible())
        shopping_setup->reject();
    if (custom_edit_dialog && custom_edit_dialog->isVisible())
        custom_edit_dialog->reject();
    if (request_edit && request_edit->isVisible())
        request_edit->reject();

    if (!plan) {
        clear();
        return;
    }

    ScrollState state;
    size_t step_count{};
    if (is_update) {
        state = scrollState();
        step_count = plan_->steps.size();
    } else if (plan_)
        setStepDescriptions();

    plan_ = plan;

    setEnabled(true);

    title_widget->setPlan(*plan_);
    if (!is_main)
        setWindowTitle(plan_->name + " - " APP_NAME);

    for (auto w : step_widgets)
        w->hide();

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        reuseStepWidget(i);

    steps_widget->adjustSize();
    if (is_update && step_count == plan_->steps.size()) {
        restoreScrollState(state);
        if (state.focus_widget)
            state.focus_widget->setFocus();
    }

    displayCost();
    displayFinalStep();
}

} // namespace planner
