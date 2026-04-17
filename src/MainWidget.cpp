#include "MainWidget.h"
#include "AppState.h"
#include "MainWindow.h"
#include "Plan.h"
#include "PlanItem.h"
#include "PlanModel.h"
#include "PlanTreeView.h"
#include "PlanWidget.h"
#include "Settings.h"
#include "UpdateCostDialog.h"
#include <QGuiApplication>
#include <QVBoxLayout>

namespace planner {

MainWidget::MainWidget(QWidget* parent)
    : QWidget{parent}
{
    auto layout = new QVBoxLayout{};
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    plan_widget = new PlanWidget{true, this};
    layout->addWidget(plan_widget);
}

void MainWidget::connectSignals()
{
    connect(AppState::state.mw->plan_view_poe1,
            &QTreeView::clicked,
            this,
            &MainWidget::setPlanOnClick);
    connect(AppState::state.mw->plan_view_poe2,
            &QTreeView::clicked,
            this,
            &MainWidget::setPlanOnClick);

    connect(AppState::state.mw->plan_view_poe1->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this,
            &MainWidget::setPlanOnCurrentChange);
    connect(AppState::state.mw->plan_view_poe2->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this,
            &MainWidget::setPlanOnCurrentChange);

    connect(AppState::state.mw->plan_view_poe1,
            &PlanTreeView::planSelected,
            this,
            &MainWidget::selectPlan);
    connect(AppState::state.mw->plan_view_poe2,
            &PlanTreeView::planSelected,
            this,
            &MainWidget::selectPlan);

    connect(AppState::state.plan_model_poe1,
            &PlanModel::currentNeedsReselecting,
            this,
            &MainWidget::reselectCurrent);
    connect(AppState::state.plan_model_poe2,
            &PlanModel::currentNeedsReselecting,
            this,
            &MainWidget::reselectCurrent);

    plan_widget->connectSignals();
}

Game MainWidget::game() const
{
    return plan_widget->game();
}

Plan* MainWidget::plan() const
{
    return plan_widget->plan();
}

void MainWidget::openPlan(const QUuid& plan_id, Game game)
{
    if (plan() && plan_id == plan()->id())
        return;

    AppState::state.mw->planView(game)->selectPlan(plan_id);
}

void MainWidget::addStep()
{
    plan_widget->addStep();
}

void MainWidget::updateCost(Game game, const std::vector<std::pair<Plan*, bool> >& updated_plans)
{
    auto model = AppState::planModel(game);
    for (auto& p : updated_plans) {
        auto idx = p.first->item()->index();
        model->updateCost(idx);
    }

    plan_widget->updateCost(game, updated_plans);
}

void MainWidget::hideDescriptions(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_descriptions>(hide);
    plan_widget->hideDescriptions();
}

void MainWidget::hideEmptyResources(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_empty_resources>(hide);
    plan_widget->hideEmptyResources();
}

void MainWidget::hideEmptyResults(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_empty_results>(hide);
    plan_widget->hideEmptyResults();
}

void MainWidget::hideNotUsedItems(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_not_used_items>(hide);
    plan_widget->hideNotUsedItems();
}

void MainWidget::hideTitleCurrencyName(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_title_currency_name>(hide);
    plan_widget->hideTitleCurrencyName();
}

void MainWidget::goBack()
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

    AppState::state.mw->planView(plan_model->game)->selectPlan(plan_it->second);
}

void MainWidget::goForward()
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

    AppState::state.mw->planView(plan_model->game)->selectPlan(plan_it->second);
}

void MainWidget::setPlanOnClick(const QModelIndex& index)
{
    if (QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier))
        return;

    auto plan_model = static_cast<const PlanModel*>(index.model());

    auto item = plan_model->internalPtr(index);
    if (item->isFolder() || item->plan() == plan())
        return;

    setPlan(plan_model, item->plan());
}

void MainWidget::setPlanOnCurrentChange(const QModelIndex& new_current)
{
    if (QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier)
        && QGuiApplication::mouseButtons().testFlag(Qt::LeftButton))
        return;

    if (!new_current.isValid())
        return;

    auto plan_model = static_cast<const PlanModel*>(new_current.model());

    auto item = plan_model->internalPtr(new_current);
    if (item->isFolder() || item->plan() == plan())
        return;

    setPlan(plan_model, item->plan());
}

void MainWidget::selectPlan(PlanModel& model, Plan& selected_plan)
{
    if (plan() != &selected_plan) {
        if (game() != selected_plan.game)
            AppState::state.mw->raiseDock(selected_plan.game);

        setPlan(&model, &selected_plan);
    }
}

void MainWidget::reselectCurrent(Game game)
{
    if (plan() && plan()->game == game)
        AppState::state.mw->planView(game)->selectPlan(*plan());
}

void MainWidget::checkDeletingPlans(const QModelIndex& parent, int first, int last)
{
    auto model = static_cast<PlanModel*>(sender());
    auto updating_plan = AppState::state.update_cost_dialog->plan();

    bool check_updating = updating_plan && updating_plan->game == model->game;

    auto parent_item = model->internalPtr(parent);
    check_updating = check_updating
                     && parent_item->isDescendantDeleting(*updating_plan->item(), first, last);

    if (check_updating)
        AppState::state.update_cost_dialog->reject();

    bool current_is_deleting = plan_widget->checkDeletingPlans(*model, *parent_item, first, last);
    if (current_is_deleting) {
        auto isValid = [&](const std::pair<QUuid, Game>& p) {
            auto model = AppState::planModel(p.second);
            if (auto it = model->plans.find(p.first); it != model->plans.end()) {
                auto res = parent_item->isDescendantDeleting(*it->second.item(), first, last);
                return !res;
            }
            return false;
        };

        auto reverse_it = std::find_if(std::make_reverse_iterator(history_it),
                                       navigation_history.rend(),
                                       isValid);
        history_it = navigation_history.erase(reverse_it.base(), std::next(history_it));

        if (history_it == navigation_history.begin()) {
            history_it = std::find_if(history_it, navigation_history.end(), isValid);
            history_it = navigation_history.erase(navigation_history.begin(), history_it);

            if (history_it == navigation_history.end()) {
                plan_widget->setPlan(model, nullptr);
                updateBack();
                updateForward();
                return;
            }
        } else
            history_it = std::prev(history_it);

        auto new_model = AppState::planModel(history_it->second);
        setPlan(new_model, &new_model->plans.at(history_it->first), false);
        reselectCurrent(plan()->game);
    } else if (plan() && plan()->game == model->game)
        reselectCurrent(plan()->game);
}

void MainWidget::setPlan(const PlanModel* model, Plan* new_plan, bool update_history)
{
    auto prev_game = game();
    plan_widget->setPlan(model, new_plan);

    if (prev_game != plan()->game)
        emit gameChanged(plan()->game);

    if (update_history) {
        if (history_it == navigation_history.end())
            history_it = navigation_history.emplace(history_it, plan()->id(), plan()->game);
        else if (history_it->first != plan()->id()) {
            history_it = navigation_history
                             .emplace(navigation_history.erase(std::next(history_it),
                                                               navigation_history.end()),
                                      plan()->id(),
                                      plan()->game);
        }
    }

    updateBack();
    updateForward();
}

void MainWidget::updateBack()
{
    AppState::state.mw->back_action->setEnabled(history_it != navigation_history.begin());
}

void MainWidget::updateForward()
{
    AppState::state.mw->forward_action->setEnabled(
        history_it != navigation_history.end() && std::next(history_it) != navigation_history.end());
}

} // namespace planner
