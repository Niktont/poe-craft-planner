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
#include <QHBoxLayout>
#include <QLabel>
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

    auto steps_scroll = new QScrollArea{};
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

    steps_scroll->setWidget(steps_widget);

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
            &TradeRequestCache::defaultTimeChanged,
            this,
            &PlanWidget::updateTradeTime);
    connect(mw()->trade_cache_poe2,
            &TradeRequestCache::defaultTimeChanged,
            this,
            &PlanWidget::updateTradeTime);

    connect(mw()->trade_cache_poe1,
            &TradeRequestCache::dataChanged,
            this,
            [this](const QModelIndex& idx) {
                updateTradeName(mw()->trade_cache_poe1->cache.nth(idx.row())->first);
            });
    connect(mw()->trade_cache_poe2,
            &TradeRequestCache::dataChanged,
            this,
            [this](const QModelIndex& idx) {
                updateTradeName(mw()->trade_cache_poe2->cache.nth(idx.row())->first);
            });

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
    if (pos >= step_widgets.size()) {
        addStepWidget(pos);
    } else {
        auto step_widget = step_widgets[pos];
        step_widget->show();
        step_widget->setStep(plan_, pos);
    }
    if (plan_->finalStepId().isNull())
        cost_widget->hide();
    plan_->setChanged();
}

void PlanWidget::addStepWidget(size_t i)
{
    auto step_widget = step_widgets.emplace_back(new StepWidget{this});
    step_widget->hideDescription(is_descriptions_hidden);
    steps_widget->layout()->addWidget(step_widget);
    step_widget->setStep(plan_, i);
}

void PlanWidget::displayCost()
{
    if (!plan_)
        return;

    cost_widget->setCost(plan_->game, plan_->costStep());
}

void PlanWidget::clear()
{
    setEnabled(false);
    name_label->setText({});
    league_label->setText({});
    cost_widget->hide();

    for (size_t i = 0; i < step_widgets.size(); ++i) {
        step_widgets[i]->hide();
        step_widgets[i]->setStep(nullptr, 0);
    }
}

void PlanWidget::updateCost(Plan* plan)
{
    if (!plan || plan != plan_)
        return;

    league_label->setText(plan->league);

    displayCost();

    for (size_t i = 0; i < plan->steps.size(); ++i)
        step_widgets[i]->updateCost(plan);
}

void PlanWidget::setDescriptions(Plan* plan)
{
    if (plan_ != plan)
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
    steps_widget->layout()->addWidget(widget);

    bool is_final_changed = &plan_->steps[step_pos] == plan_->costStep();
    if (is_final_changed)
        plan_->setFinalStep({});

    auto deleted_id = plan_->steps[step_pos].id;

    plan_->steps.erase(plan_->steps.begin() + step_pos);
    plan_->setChanged();

    for (size_t i = step_pos; i < plan_->steps.size(); ++i)
        step_widgets[i]->updateStepNames(deleted_id, true);

    if (is_final_changed)
        displayCost();
}

void PlanWidget::duplicateStep(size_t step_pos)
{
    if (!plan_ || step_pos >= plan_->steps.size())
        return;

    for (size_t i = step_pos + 1; i < plan_->steps.size(); ++i)
        step_widgets[i]->updatePos(step_widgets[i]->stepPos() + 1);

    auto step_it = plan_->steps.emplace(plan_->steps.begin() + step_pos + 1, plan_->steps[step_pos]);
    step_it->name += tr(" - Copy");

    if (plan_->steps.size() < step_widgets.size()) {
        auto widget = step_widgets.back();
        step_widgets.pop_back();
        step_widgets.insert(step_widgets.begin() + step_pos + 1, widget);
        steps_widget->layout()->removeWidget(widget);
        widget->show();
        widget->setStep(plan_, step_pos + 1);
        static_cast<QVBoxLayout*>(steps_widget->layout())->insertWidget(step_pos + 1, widget);
    } else {
        auto widget_it = *step_widgets.emplace(step_widgets.begin() + step_pos + 1,
                                               new StepWidget{this});
        widget_it->hideDescription(is_descriptions_hidden);
        widget_it->setStep(plan_, step_pos + 1);
        static_cast<QVBoxLayout*>(steps_widget->layout())->insertWidget(step_pos + 1, widget_it);
    }
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
        displayCost();
}

void PlanWidget::updateStepNames(size_t renamed_step)
{
    if (!plan_)
        return;

    auto& step = plan_->steps[renamed_step];
    for (size_t i = renamed_step + 1; i < plan_->steps.size(); ++i)
        step_widgets[i]->updateStepNames(step.id, false);
}

void PlanWidget::updatePlanName(Plan* renamed_plan)
{
    if (renamed_plan == plan_)
        name_label->setText(plan_->name);
}

void PlanWidget::updateTradeName(const TradeRequestKey& request)
{
    if (!plan_)
        return;

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->updateTradeName(request);
}

void PlanWidget::updateTradeTime(const TradeRequestKey& request)
{
    if (!plan_)
        return;

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->updateTradeTime(request);
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

    if (plan_)
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

    if (plan_)
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
        addStepWidget(i);

    if (plan->steps.empty())
        addStep();

    displayCost();

    for (size_t i = 0; i < plan_->steps.size(); ++i)
        step_widgets[i]->displayCost();
}

} // namespace planner
