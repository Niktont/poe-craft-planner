#include "PlanWidget.h"
#include "AppState.h"
#include "CostWidget.h"
#include "ExchangeRequestCache.h"
#include "MainWindow.h"
#include "Plan.h"
#include "PlanItem.h"
#include "PlanModel.h"
#include "PlanTitleWidget.h"
#include "PlanTreeView.h"
#include "Settings.h"
#include "SettingsDialog.h"
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
#include <QTextEdit>
#include <QTreeView>
#include <QVBoxLayout>

namespace planner {
PlanWidget::PlanWidget(MainWindow& mw)
    : QWidget{&mw}
{
    auto main_layout = new QVBoxLayout{};
    setLayout(main_layout);

    title_widget = new PlanTitleWidget{};
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
    main_layout->addWidget(title_widget);

    steps_scroll = new QScrollArea{};
    steps_scroll->setWidgetResizable(true);
    steps_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    steps_scroll->setFrameShape(QFrame::Box);
    steps_scroll->setFrameShadow(QFrame::Plain);
    steps_scroll->setContentsMargins(0, 0, 0, 0);
    layout()->addWidget(steps_scroll);

    steps_widget = new QWidget{};

    auto steps_layout = new QVBoxLayout{};
    steps_layout->setVerticalSizeConstraint(QLayout::SetFixedSize);
    steps_widget->setLayout(steps_layout);
    steps_layout->setContentsMargins(0, 0, 0, 0);
    steps_layout->addStretch(1);

    steps_scroll->setWidget(steps_widget);

    paste_step_action = addAction(tr("Paste Step"), this, [this]() {
        if (plan_)
            pasteStep(plan_->steps.size());
    });

    setEnabled(false);
}

void PlanWidget::connectSignals()
{
    connect(AppState::state.plan_model_poe1,
            &PlanModel::rowsAboutToBeRemoved,
            this,
            &PlanWidget::checkDeletingPlans);
    connect(AppState::state.plan_model_poe2,
            &PlanModel::rowsAboutToBeRemoved,
            this,
            &PlanWidget::checkDeletingPlans);

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

    connect(mw()->plan_view_poe1, &QTreeView::clicked, this, &PlanWidget::setPlanOnClick);
    connect(mw()->plan_view_poe2, &QTreeView::clicked, this, &PlanWidget::setPlanOnClick);

    connect(mw()->plan_view_poe1->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this,
            &PlanWidget::setPlanOnCurrentChange);
    connect(mw()->plan_view_poe2->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this,
            &PlanWidget::setPlanOnCurrentChange);

    connect(mw()->plan_view_poe1, &PlanTreeView::planSelected, this, &PlanWidget::selectPlan);
    connect(mw()->plan_view_poe2, &PlanTreeView::planSelected, this, &PlanWidget::selectPlan);

    connect(AppState::state.plan_model_poe1,
            &PlanModel::currentNeedsReselecting,
            this,
            &PlanWidget::reselectCurrent);
    connect(AppState::state.plan_model_poe2,
            &PlanModel::currentNeedsReselecting,
            this,
            &PlanWidget::reselectCurrent);

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

    connect(mw()->settings_dialog,
            &SettingsDialog::tradeTimeChanged,
            this,
            &PlanWidget::updateTradeDefaultTime);
    connect(mw()->settings_dialog,
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

MainWindow* PlanWidget::mw() const
{
    return static_cast<MainWindow*>(parent());
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

    if (pos >= step_widgets.size()) {
        emplaceStepWidget(pos);
    } else {
        auto step_widget = step_widgets[pos];
        step_widget->show();
        step_widget->setStep(plan_, pos);
    }
    if (plan_->finalStepId().isNull())
        updateDisplayedCost();
}

void PlanWidget::emplaceStepWidget(size_t i)
{
    auto step_widget = *step_widgets.emplace(step_widgets.begin() + i, new StepWidget{this});
    step_widget->hideDescription();
    static_cast<QVBoxLayout*>(steps_widget->layout())->insertWidget(i, step_widget);
    step_widget->setStep(plan_, i);
}

void PlanWidget::displayCost()
{
    title_widget->cost_widget->setCost(plan_->game, plan_->costStep());
}

void PlanWidget::updateCosts(bool current_updated)
{
    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->updateCost(current_updated);
}

void PlanWidget::updateBack()
{
    mw()->back_action->setEnabled(history_it != navigation_history.begin());
}

void PlanWidget::updateForward()
{
    mw()->forward_action->setEnabled(history_it != navigation_history.end()
                                     && std::next(history_it) != navigation_history.end());
}

void PlanWidget::clear()
{
    setEnabled(false);

    plan_ = nullptr;
    current_model = nullptr;

    title_widget->hide();

    for (size_t i = 0; i < step_widgets.size(); ++i) {
        step_widgets[i]->hide();
        step_widgets[i]->setStep(nullptr, 0);
    }
}

void PlanWidget::updateCost(Game game, const std::vector<std::pair<Plan*, bool>>& updated_plans)
{
    auto model = AppState::planModel(game);
    auto updateModelCost = [](Plan* plan, PlanModel* model) {
        auto idx = plan->item()->index();
        model->updateCost(idx);
    };

    if (!plan_ || plan_->game != game) {
        for (auto& p : updated_plans)
            updateModelCost(p.first, model);
        return;
    }

    bool current_updated = false;
    bool final_changed = false;
    for (auto& p : updated_plans) {
        if (p.first != plan_)
            updateModelCost(p.first, model);
        else {
            current_updated = true;
            final_changed = p.second;
        }
    }

    if (current_updated) {
        title_widget->league_label->setText(plan_->league);
        updateDisplayedCost();
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

void PlanWidget::openPlan(const QUuid& plan_id, Game game)
{
    if (plan_ && plan_id == plan_->id())
        return;

    mw()->planView(game)->selectPlan(plan_id);
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

    for (size_t i = step_pos + 1; i < plan_->steps.size(); ++i)
        step_widgets[i]->updatePos(step_widgets[i]->stepPos() + 1);

    auto step_it = plan_->steps.emplace(plan_->steps.begin() + step_pos + 1, plan_->steps[step_pos]);
    step_it->name += tr(" - Copy");
    plan_->setChanged();

    step_widgets[step_pos]->updateMoveActions();

    if (plan_->steps.size() < step_widgets.size()) {
        auto widget = step_widgets.back();
        step_widgets.pop_back();
        step_widgets.insert(step_widgets.begin() + step_pos + 1, widget);
        steps_widget->layout()->removeWidget(widget);
        static_cast<QVBoxLayout*>(steps_widget->layout())->insertWidget(step_pos + 1, widget);
        widget->show();
        widget->setStep(plan_, step_pos + 1);
    } else
        emplaceStepWidget(step_pos + 1);
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
    steps_scroll->ensureWidgetVisible(step_widgets[pos]);
}

void PlanWidget::pasteStep(size_t step_pos)
{
    if (!plan_ || !StepCopyState::haveCopy(plan_->game))
        return;

    auto source_plan_it = current_model->plans.find(StepCopyState::state.plan_id);
    if (source_plan_it == current_model->plans.end())
        return;
    auto& source_plan = source_plan_it->second;

    auto source_step_it = source_plan.findStepIt(StepCopyState::state.step_id);
    if (source_step_it == source_plan.steps.cend())
        return;

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

    step_pos = std::min(step_pos, plan_->steps.size());
    auto copy_size = steps_to_copy.size();
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

    bool is_last_changed = step_pos == plan_->steps.size();
    auto move_begin = std::move_iterator(copied_steps.begin());
    auto move_end = std::move_iterator(copied_steps.end());
    plan_->steps.insert(plan_->steps.begin() + step_pos, move_begin, move_end);
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
            widget->show();
            widget->setStep(plan_, i);
        }
    } else {
        for (size_t i = step_pos; i < step_pos + copy_size; ++i)
            emplaceStepWidget(i);
    }

    if (plan_->finalStepId().isNull() && is_last_changed)
        updateDisplayedCost();
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

    static_cast<QVBoxLayout*>(steps_widget->layout())->insertWidget(step_pos, bottom_widget);
    bool is_final_changed = plan_->finalStepId().isNull() && (bottom_it + 1) == plan_->steps.end();
    if (is_final_changed)
        updateDisplayedCost();
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
    if (&renamed_plan == plan_)
        title_widget->name_label->setText(plan_->name);

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

void PlanWidget::hideDescriptions(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_descriptions>(hide);
    for (auto step : step_widgets)
        step->hideDescription();
}

void PlanWidget::hideEmptyResources(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_empty_resources>(hide);
    if (!plan_)
        return;

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->hideEmptyResources();
}

void PlanWidget::hideEmptyResults(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_empty_results>(hide);
    if (!plan_)
        return;

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->hideEmptyResults();
}

void PlanWidget::hideNotUsedItems(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_not_used_items>(hide);
    if (!plan_)
        return;

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->hideNotUsedItems();
}

void PlanWidget::hideTitleCurrencyName(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_title_currency_name>(hide);
    if (!plan_)
        return;

    title_widget->cost_widget->hideCurrencyName();
    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->hideTitleCurrencyName();
}

void PlanWidget::goBack()
{
    if (history_it == navigation_history.begin()) {
        updateBack();
        return;
    }

    auto current_id = history_it != navigation_history.end() ? history_it->first : QUuid{};

    auto prev_it = std::prev(history_it);
    auto plan_model = AppState::planModel(prev_it->second);
    auto plan_it = plan_model->plans.find(prev_it->first);
    while (prev_it != navigation_history.begin()
           && (plan_it == plan_model->plans.end() || plan_it->first == current_id)) {
        prev_it = std::prev(prev_it);
        plan_model = AppState::planModel(prev_it->second);
        plan_it = plan_model->plans.find(prev_it->first);
    }

    if (plan_it == plan_model->plans.end() || plan_it->first == current_id) {
        history_it = navigation_history.erase(prev_it, history_it);
        updateBack();
        return;
    }

    if (std::distance(prev_it, history_it) > 1)
        history_it = std::prev(navigation_history.erase(std::next(prev_it), history_it));
    else
        history_it = prev_it;

    mw()->planView(plan_model->game)->selectPlan(plan_it->second);
}

void PlanWidget::goForward()
{
    if (history_it == navigation_history.end()) {
        updateForward();
        return;
    }

    auto next_it = std::next(history_it);
    if (next_it == navigation_history.end()) {
        updateForward();
        return;
    }

    auto back_it = std::prev(navigation_history.end());

    auto plan_model = AppState::planModel(next_it->second);
    auto plan_it = plan_model->plans.find(next_it->first);
    while (next_it != back_it
           && (plan_it == plan_model->plans.end() || plan_it->first == history_it->first)) {
        next_it = std::next(next_it);
        plan_model = AppState::planModel(next_it->second);
        plan_it = plan_model->plans.find(next_it->first);
    }

    if (plan_it == plan_model->plans.end() || plan_it->first == history_it->first) {
        history_it = std::prev(
            navigation_history.erase(std::next(history_it), navigation_history.end()));
        updateForward();
        return;
    }

    if (std::distance(history_it, next_it) > 1)
        history_it = navigation_history.erase(std::next(history_it), next_it);
    else
        history_it = next_it;

    mw()->planView(plan_model->game)->selectPlan(plan_it->second);
}

void PlanWidget::contextMenuEvent(QContextMenuEvent* event)
{
    if (!plan_)
        return;

    auto menu = new QMenu{this};
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->addAction(mw()->add_step_action);
    if (StepCopyState::haveCopy(plan_->game))
        menu->addAction(paste_step_action);

    menu->popup(event->globalPos());
}

void PlanWidget::selectPlan(PlanModel& model, Plan& plan)
{
    if (plan_ != &plan) {
        if (game() != plan.game)
            mw()->raiseDock(plan.game);

        setPlan(&model, &plan);
    }
}

void PlanWidget::reselectCurrent(Game game)
{
    if (plan_ && plan_->game == game)
        mw()->planView(game)->selectPlan(*plan_);
}

void PlanWidget::setPlanChanged()
{
    if (plan_)
        plan_->setChanged();
}

void PlanWidget::setPlanOnClick(const QModelIndex& index)
{
    if (QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier))
        return;

    auto plan_model = static_cast<const PlanModel*>(index.model());

    auto item = plan_model->internalPtr(index);
    if (item->isFolder() || item->plan() == plan_)
        return;

    setPlan(plan_model, item->plan());
}

void PlanWidget::setPlanOnUpdate(Plan& updated_plan)
{
    if (&updated_plan != plan_)
        return;

    setPlan(current_model, &updated_plan, true);
}

void PlanWidget::setPlanOnCurrentChange(const QModelIndex& new_current)
{
    if (QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier)
        && QGuiApplication::mouseButtons().testFlag(Qt::LeftButton))
        return;

    if (!new_current.isValid())
        return;

    auto plan_model = static_cast<const PlanModel*>(new_current.model());

    auto item = plan_model->internalPtr(new_current);
    if (item->isFolder() || item->plan() == plan_)
        return;

    setPlan(plan_model, item->plan());
}

void PlanWidget::checkDeletingPlans(const QModelIndex& parent, int first, int last)
{
    auto model = static_cast<PlanModel*>(sender());
    auto updating_plan = AppState::state.update_cost_dialog->plan();

    bool check_current = plan_ && plan_->game == model->game;
    bool check_updating = updating_plan && updating_plan->game == model->game;

    if (!check_current && !check_updating)
        return;

    auto checkPlan = [](const PlanItem& parent, int first, int last, const Plan& plan) {
        auto descent_row = plan.item()->isAncestor(parent);
        return first <= descent_row && descent_row <= last;
    };

    auto parent_item = model->internalPtr(parent);
    check_current = check_current && checkPlan(*parent_item, first, last, *plan_);
    check_updating = check_updating && checkPlan(*parent_item, first, last, *updating_plan);

    if (check_updating)
        AppState::state.update_cost_dialog->reject();

    if (!check_current)
        return;

    history_it = navigation_history.erase(history_it, navigation_history.end());
    updateBack();
    updateForward();

    setPlan(model, nullptr);
}

void PlanWidget::setPlan(const PlanModel* model, Plan* plan, bool is_update)
{
    if (!plan) {
        clear();
        return;
    }

    if (!is_update && plan_)
        setStepDescriptions();

    auto prev_game = game();
    plan_ = plan;
    current_model = model;

    if (history_it == navigation_history.end())
        history_it = navigation_history.emplace(history_it, plan_->id(), plan_->game);
    else if (history_it->first != plan_->id()) {
        history_it = navigation_history.emplace(navigation_history.erase(std::next(history_it),
                                                                         navigation_history.end()),
                                                plan_->id(),
                                                plan_->game);
    }
    updateBack();
    updateForward();

    if (prev_game != plan_->game)
        emit gameChanged(plan_->game);

    setEnabled(true);

    title_widget->setPlan(*plan_);

    size_t i = 0;
    size_t steps_size = plan_->steps.size();
    for (; i < step_widgets.size(); ++i) {
        if (i < steps_size) {
            step_widgets[i]->show();
            step_widgets[i]->setStep(plan_, i);
        } else
            step_widgets[i]->hide();
    }
    for (; i < steps_size; ++i)
        emplaceStepWidget(i);

    displayCost();
    displayFinalStep();
}

} // namespace planner
