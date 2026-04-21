#include "MainWidget.h"
#include "AppState.h"
#include "MainWindow.h"
#include "Plan.h"
#include "PlanItem.h"
#include "PlanModel.h"
#include "PlanTreeView.h"
#include "PlanWidget.h"
#include "Settings.h"
#include <QApplication>
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
            &PlanTreeView::planWindowRequested,
            this,
            &MainWidget::openPlanWindow);
    connect(AppState::state.mw->plan_view_poe2,
            &PlanTreeView::planWindowRequested,
            this,
            &MainWidget::openPlanWindow);

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

void MainWidget::openPlanLink(const QUuid& plan_id, Game game, bool need_window)
{
    if (need_window)
        openPlanWindow(plan_id, game);
    else
        openPlan(plan_id, game);
}

void MainWidget::openPlan(const QUuid& plan_id, Game game)
{
    if (isCurrentPlan(plan_id))
        return raisePlanWindow(*plan_widget);
    if (auto it = plan_windows.find(plan_id); it != plan_windows.end())
        return raisePlanWindow(*it->second);

    AppState::state.mw->planView(game)->selectPlan(plan_id);
}

void MainWidget::openPlanWindow(const QUuid& plan_id, Game game)
{
    if (isCurrentPlan(plan_id)) {
        goBack();
        if (isCurrentPlan(plan_id)) {
            goForward();
            if (isCurrentPlan(plan_id))
                return raisePlanWindow(*plan_widget);
        }
    }

    auto& plans = AppState::planModel(game)->plans;
    auto plan_it = plans.find(plan_id);
    if (plan_it == plans.end())
        return;

    auto [window_it, added] = plan_windows.try_emplace(plan_id);
    if (!added)
        return raisePlanWindow(*window_it->second);

    Qt::WindowFlags flags{};
    if (Settings::stays_on_top)
        flags |= Qt::WindowStaysOnTopHint;
    window_it->second = new PlanWidget{false, nullptr, flags};
    window_it->second->connectSignals();
    window_it->second->setPlan(&plan_it->second);

    auto mouse_pos = QCursor::pos();
    mouse_pos.rx() -= 5;
    mouse_pos.ry() += 5;
    window_it->second->setGeometry({mouse_pos, plan_widget->size()});
    window_it->second->show();

    connect(window_it->second, &PlanWidget::aboutToClose, this, &MainWidget::eraseWindow);
}

void MainWidget::addStep()
{
    plan_widget->addStep();
}

void MainWidget::savePlan()
{
    Plan* plan{};
    if (auto active_window = QApplication::activeWindow()) {
        if (auto plan_window = qobject_cast<PlanWidget*>(active_window)) {
            plan = plan_window->plan();
        }
    }

    if (plan || (plan = plan_widget->plan())) {
        AppState::planModel(plan->game)->savePlan(*plan->item());
    }
}

void MainWidget::updateCosts()
{
    if (auto active_window = QApplication::activeWindow()) {
        if (auto plan_window = qobject_cast<PlanWidget*>(active_window)) {
            plan_window->update_costs_action->trigger();
            return;
        }
    }

    plan_widget->update_costs_action->trigger();
}

void MainWidget::openShoppingDialog()
{
    if (auto active_window = QApplication::activeWindow()) {
        if (auto plan_window = qobject_cast<PlanWidget*>(active_window)) {
            plan_window->shopping_mode_action->trigger();
            return;
        }
    }

    plan_widget->shopping_mode_action->trigger();
}

void MainWidget::updateCost(Game game, const std::vector<std::pair<Plan*, bool> >& updated_plans)
{
    auto model = AppState::planModel(game);
    for (auto& p : updated_plans) {
        auto idx = p.first->item()->index();
        model->updateCost(idx);
    }

    plan_widget->updateCost(game, updated_plans);
    for (auto& p : plan_windows)
        p.second->updateCost(game, updated_plans);
}

void MainWidget::hideDescriptions(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_descriptions>(hide);
    plan_widget->hideDescriptions();
    for (auto& p : plan_windows)
        p.second->hideDescriptions();
}

void MainWidget::hideEmptyResources(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_empty_resources>(hide);
    plan_widget->hideEmptyResources();
    for (auto& p : plan_windows)
        p.second->hideEmptyResources();
}

void MainWidget::hideEmptyResults(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_empty_results>(hide);
    plan_widget->hideEmptyResults();
    for (auto& p : plan_windows)
        p.second->hideEmptyResults();
}

void MainWidget::hideNotUsedItems(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_not_used_items>(hide);
    plan_widget->hideNotUsedItems();
    for (auto& p : plan_windows)
        p.second->hideNotUsedItems();
}

void MainWidget::hideTitleCurrencyName(bool hide)
{
    Settings::setCache<Settings::windows_main_hide_title_currency_name>(hide);
    plan_widget->hideTitleCurrencyName();
    for (auto& p : plan_windows)
        p.second->hideTitleCurrencyName();
}

void MainWidget::goBack()
{
    if (history_it == navigation_history.begin()) {
        updateBack();
        return;
    }

    PlanModel* plan_model{};
    PlanModel::Plans::iterator plan_it;
    auto reverse_it = std::find_if(std::make_reverse_iterator(history_it),
                                   navigation_history.rend(),
                                   [&](const auto& p) {
                                       plan_model = AppState::planModel(p.second);
                                       plan_it = plan_model->plans.find(p.first);
                                       return plan_it != plan_model->plans.end()
                                              && !isPlanActive(p.first);
                                   });

    history_it = navigation_history.erase(reverse_it.base(), history_it);
    if (history_it == navigation_history.begin()) {
        updateBack();
        return;
    }
    --history_it;

    AppState::state.mw->planView(plan_model->game)->selectPlan(plan_it->second);
}

void MainWidget::goForward()
{
    if (history_it == navigation_history.end()) {
        updateForward();
        return;
    }

    PlanModel* plan_model{};
    PlanModel::Plans::iterator plan_it;
    auto next_it = std::find_if(std::next(history_it), navigation_history.end(), [&](const auto& p) {
        plan_model = AppState::planModel(p.second);
        plan_it = plan_model->plans.find(p.first);
        return plan_it != plan_model->plans.end() && !isPlanActive(p.first);
    });
    history_it = navigation_history.erase(std::next(history_it), next_it);

    if (history_it == navigation_history.end()) {
        --history_it;
        updateForward();
        return;
    }

    AppState::state.mw->planView(plan_model->game)->selectPlan(plan_it->second);
}

void MainWidget::selectPlan(Plan& selected_plan)
{
    if (game() != selected_plan.game)
        AppState::state.mw->raiseDock(selected_plan.game);

    setMainPlan(selected_plan);
}

void MainWidget::reselectCurrent(Game game)
{
    if (plan() && plan()->game == game)
        AppState::state.mw->planView(game)->selectPlan(*plan());
}

void MainWidget::checkDeletingPlans(const QModelIndex& parent, int first, int last)
{
    auto model = static_cast<PlanModel*>(sender());
    auto parent_item = model->internalPtr(parent);

    bool current_is_deleting = plan_widget->checkDeletingPlans(*parent_item, first, last);
    if (current_is_deleting) {
        PlanModel::Plans::iterator it;
        auto isValid = [&](const std::pair<QUuid, Game>& p) {
            if (plan_windows.contains(p.first))
                return false;

            auto& plans = AppState::planModel(p.second)->plans;
            it = plans.find(p.first);
            return it != plans.end()
                   && !parent_item->isDescendantDeleting(*it->second.item(), first, last);
        };

        auto reverse_it = std::find_if(std::make_reverse_iterator(history_it),
                                       navigation_history.rend(),
                                       isValid);
        history_it = navigation_history.erase(reverse_it.base(), std::next(history_it));

        if (history_it == navigation_history.begin()) {
            history_it = std::find_if(history_it, navigation_history.end(), isValid);
            history_it = navigation_history.erase(navigation_history.begin(), history_it);

            if (history_it == navigation_history.end()) {
                plan_widget->setPlan(nullptr);
                updateBack();
                updateForward();
                return;
            }
        } else
            history_it = std::prev(history_it);

        setMainPlan(it->second, false);
        updateBack();
        updateForward();
        reselectCurrent(plan()->game);
    } else if (plan() && plan()->game == model->game)
        reselectCurrent(plan()->game);
}

void MainWidget::setMainPlan(Plan& new_plan, bool update_history)
{
    if (plan() == &new_plan)
        return;

    if (auto it = plan_windows.find(new_plan.id()); it != plan_windows.end())
        return;

    auto prev_game = game();
    plan_widget->setPlan(&new_plan);

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

        if (navigation_history.size() > 500) {
            auto pos = std::distance(navigation_history.begin(), history_it);
            auto del_pos = std::min(250ll, pos);
            navigation_history.erase(navigation_history.begin(),
                                     navigation_history.begin() + del_pos);
            history_it = navigation_history.begin() + pos - del_pos;
        }
    }

    updateBack();
    updateForward();
}

void MainWidget::eraseWindow(PlanWidget& window)
{
    if (window.plan())
        plan_windows.erase(window.plan()->id());
}

bool MainWidget::isCurrentPlan(const QUuid& id) const
{
    return plan() && plan()->id() == id;
}

bool MainWidget::isPlanActive(const QUuid& id) const
{
    return isCurrentPlan(id) || plan_windows.contains(id);
}

void MainWidget::raisePlanWindow(PlanWidget& window)
{
    if (!window.isActiveWindow()) {
        window.activateWindow();
        window.raise();
    }
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
