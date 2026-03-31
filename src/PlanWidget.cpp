#include "PlanWidget.h"
#include "CostWidget.h"
#include "ExchangeRequestCache.h"
#include "MainWindow.h"
#include "Plan.h"
#include "PlanModel.h"
#include "PlanTreeView.h"
#include "SettingsDialog.h"
#include "StepItem.h"
#include "StepWidget.h"
#include "TradeRequestCache.h"
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

    auto title_layout = new QHBoxLayout{};
    main_layout->addLayout(title_layout);

    name_label = new QLabel{};
    title_layout->addWidget(name_label);

    league_label = new QLabel{};
    title_layout->addWidget(league_label);

    cost_widget = new CostWidget(mw);
    title_layout->addWidget(cost_widget);

    title_layout->addStretch();

    steps_scroll = new QScrollArea{};
    steps_scroll->setWidgetResizable(true);
    steps_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    steps_scroll->setFrameShape(QFrame::Box);
    steps_scroll->setFrameShadow(QFrame::Plain);
    steps_scroll->setContentsMargins(0, 0, 0, 0);
    layout()->addWidget(steps_scroll);

    steps_widget = new QWidget{};

    auto steps_layout = new QVBoxLayout{};
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
    connect(mw()->plan_model_poe1,
            &PlanModel::rowsAboutToBeRemoved,
            this,
            &PlanWidget::checkDeletingPlans);
    connect(mw()->plan_model_poe2,
            &PlanModel::rowsAboutToBeRemoved,
            this,
            &PlanWidget::checkDeletingPlans);

    connect(mw()->plan_model_poe1, &PlanModel::planUpdated, this, &PlanWidget::setPlanOnUpdate);
    connect(mw()->plan_model_poe2, &PlanModel::planUpdated, this, &PlanWidget::setPlanOnUpdate);

    connect(mw()->plan_model_poe1, &PlanModel::planRenamed, this, &PlanWidget::updatePlanName);
    connect(mw()->plan_model_poe2, &PlanModel::planRenamed, this, &PlanWidget::updatePlanName);

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

    connect(mw()->trade_cache_poe1,
            &TradeRequestCache::rowsAboutToBeRemoved,
            this,
            &PlanWidget::checkDeletingTradeRequests);
    connect(mw()->trade_cache_poe2,
            &TradeRequestCache::rowsAboutToBeRemoved,
            this,
            &PlanWidget::checkDeletingTradeRequests);

    connect(mw()->trade_cache_poe1,
            &TradeRequestCache::dataChanged,
            this,
            &PlanWidget::updateTradeRequests);
    connect(mw()->trade_cache_poe2,
            &TradeRequestCache::dataChanged,
            this,
            &PlanWidget::updateTradeRequests);

    connect(mw()->exchange_cache_poe1,
            &ExchangeRequestCache::defaultTimeChanged,
            this,
            &PlanWidget::updateCurrencyTime);
    connect(mw()->exchange_cache_poe2,
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
    step_widget->hideDescription(is_descriptions_hidden);
    static_cast<QVBoxLayout*>(steps_widget->layout())->insertWidget(i, step_widget);
    step_widget->setStep(plan_, i);
}

void PlanWidget::displayCost()
{
    cost_widget->setCost(plan_->game, plan_->costStep());
}

void PlanWidget::clear()
{
    setEnabled(false);
    name_label->clear();
    league_label->clear();
    cost_widget->hide();

    for (size_t i = 0; i < step_widgets.size(); ++i) {
        step_widgets[i]->hide();
        step_widgets[i]->setStep(nullptr, 0);
    }
}

void PlanWidget::updateCost(Game game, const std::vector<QUuid>& updated_plans)
{
    auto model = mw()->planModel(game);
    auto updateModelCost = [](const QUuid& id, PlanModel* model) {
        if (auto it = model->plans.find(id); it != model->plans.end()) {
            auto idx = it->second.item()->index();
            model->updateCost(idx);
        }
    };

    bool current_updated = false;
    if (plan_) {
        for (auto& id : updated_plans) {
            if (id != plan_->id())
                updateModelCost(id, model);
            else
                current_updated = true;
        }
    } else {
        for (auto& id : updated_plans)
            updateModelCost(id, model);
        return;
    }

    if (current_updated) {
        league_label->setText(plan_->league);
        updateDisplayedCost();
    }

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->updateCost(current_updated);
}

void PlanWidget::setDescriptions(Plan* plan)
{
    if (!plan_)
        return;

    if (plan && plan_ != plan)
        return;

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->setDescription();
}

void PlanWidget::openPlan(QUuid plan_id, Game game)
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

    for (size_t i = step_pos; i < plan_->steps.size(); ++i)
        step_widgets[i]->updateStepName(deleted_id, true);

    if (step_copy_state.second == deleted_id)
        step_copy_state = {};

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

void PlanWidget::scrollToStep(QUuid step_id)
{
    if (!plan_)
        return;

    auto it = plan_->findStepIt(step_id);
    if (it == plan_->steps.end())
        return;

    auto pos = std::distance(plan_->steps.cbegin(), it);
    steps_scroll->ensureWidgetVisible(step_widgets[pos]);
}

void PlanWidget::copyStep(size_t step_pos)
{
    if (!plan_ || step_pos >= plan_->steps.size())
        return;

    step_copy_state = {plan_->id(), plan_->steps[step_pos].id};
}

void PlanWidget::pasteStep(size_t step_pos)
{
    if (!plan_)
        return;

    auto source_plan_it = current_model->plans.find(step_copy_state.first);
    if (source_plan_it == current_model->plans.end())
        return;
    auto& source_plan = source_plan_it->second;

    auto source_step_it = source_plan.findStepIt(step_copy_state.second);
    if (source_step_it == source_plan.steps.cend())
        return;

    std::set<std::vector<Step>::const_iterator> steps_to_copy;
    auto add_step_item = [&](const auto& add_step, const StepItem& item) {
        if (auto step_item = item.step()) {
            auto it = source_plan.findStepIt(step_item->step_id);
            if (it == source_plan.steps.cend())
                return;
            add_step(add_step, it);
        }
    };
    auto add_step = [&](const auto& self, std::vector<Step>::const_iterator step_it) {
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
    for (auto& step : copied_steps)
        step.updateIds(changed_ids);

    bool is_final_changed = plan_->finalStepId().isNull() && step_pos == plan_->steps.size();
    auto move_begin = std::move_iterator(copied_steps.begin());
    auto move_end = std::move_iterator(copied_steps.end());
    plan_->steps.insert(plan_->steps.begin() + step_pos, move_begin, move_end);
    plan_->setChanged();

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

    if (is_final_changed)
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
    mw()->planModel(plan_->game)->updateCost(idx);
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
        step_widgets[i]->updateStepName(step.id, false);
}

void PlanWidget::updatePlanName(Plan* renamed_plan)
{
    if (renamed_plan == plan_)
        name_label->setText(plan_->name);

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->updatePlanName(renamed_plan->id());
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
        for (size_t i = 0; i < plan_->steps.size(); ++i)
            step_widgets[i]->clearTradeRequest(it->first);
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

void PlanWidget::hideDescriptions(bool hide)
{
    is_descriptions_hidden = hide;
    for (auto step : step_widgets)
        step->hideDescription(hide);
}

void PlanWidget::hideEmptyResources(bool hide)
{
    is_empty_resources_hidden = hide;
    if (!plan_)
        return;

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->hideEmptyResources(hide);
}

void PlanWidget::hideEmptyResults(bool hide)
{
    is_empty_results_hidden = hide;
    if (!plan_)
        return;

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->hideEmptyResults(hide);
}

void PlanWidget::contextMenuEvent(QContextMenuEvent* event)
{
    auto menu = new QMenu{this};
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->addAction(mw()->add_step_action);
    if (haveCopyStep())
        menu->addAction(paste_step_action);

    menu->popup(event->globalPos());
}

void PlanWidget::setPlanChanged()
{
    if (plan_)
        plan_->setChanged();
}

void PlanWidget::setPlanOnClick(const QModelIndex& index)
{
    auto plan_model = static_cast<const PlanModel*>(index.model());

    auto item = plan_model->internalPtr(index);
    if (item->isFolder() || item->plan() == plan_)
        return;

    setDescriptions(plan_);

    setPlan(plan_model, item->plan());
}

void PlanWidget::setPlanOnUpdate(Plan* new_plan, const Plan* old_plan)
{
    if (old_plan != plan_)
        return;

    auto model = static_cast<PlanModel*>(sender());
    setPlan(model, new_plan);
}

void PlanWidget::setPlanOnCurrentChange(const QModelIndex& new_current)
{
    if (!new_current.isValid())
        return;

    auto plan_model = static_cast<const PlanModel*>(new_current.model());

    auto item = plan_model->internalPtr(new_current);
    if (item->isFolder() || item->plan() == plan_ || (current_model && current_model != plan_model))
        return;

    setDescriptions(plan_);

    setPlan(plan_model, item->plan());
}

void PlanWidget::checkDeletingPlans(const QModelIndex& parent, int first, int last)
{
    auto model = static_cast<PlanModel*>(sender());
    if (model != current_model || !plan_)
        return;

    auto parent_item = model->internalPtr(parent);
    bool current_is_deleting = false;
    for (int i = first; i <= last; ++i) {
        auto deleting_item = parent_item->child(i);
        if (!deleting_item->isFolder()) {
            if (deleting_item->plan() == plan_) {
                current_is_deleting = true;
                break;
            }
        } else if (deleting_item->isDescendant(plan_->item())) {
            current_is_deleting = true;
            break;
        }
    }

    if (!current_is_deleting)
        return;

    setPlan(model, nullptr);
}

void PlanWidget::setPlan(const PlanModel* model, Plan* plan)
{
    current_model = model;
    plan_ = plan;
    if (!plan) {
        clear();
        return;
    }
    setEnabled(true);

    name_label->setText(plan->name);
    league_label->setText(plan->league);

    size_t i = 0;
    size_t steps_size = plan->steps.size();
    for (; i < step_widgets.size(); ++i) {
        if (i < steps_size) {
            step_widgets[i]->show();
            step_widgets[i]->setStep(plan, i);
        } else
            step_widgets[i]->hide();
    }
    for (; i < steps_size; ++i)
        emplaceStepWidget(i);

    displayCost();
    displayFinalStep();
}

} // namespace planner
