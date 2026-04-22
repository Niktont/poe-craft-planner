#include "UpdateCostDialog.h"
#include "AppState.h"
#include "CustomCalculation.h"
#include "ExchangeRequestCache.h"
#include "ExchangeRequester.h"
#include "Plan.h"
#include "PlanItem.h"
#include "PlanModel.h"
#include "Settings.h"
#include "SnapshotModel.h"
#include "StepItem.h"
#include "TradeRequestCache.h"
#include "TradeRequestManager.h"
#include "TradeRequester.h"
#include <QLabel>
#include <QListView>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPushButton>
#include <QRestReply>
#include <QStringListModel>
#include <QTimer>
#include <QVBoxLayout>

using namespace std::chrono;

namespace planner {

UpdateCostDialog::UpdateCostDialog(QWidget* parent)
    : QDialog{parent}
{
    auto main_layout = new QVBoxLayout{};
    main_layout->setVerticalSizeConstraint(QLayout::SetFixedSize);
    setLayout(main_layout);

    setMinimumWidth(300);

    progress_label = new QLabel{this};
    layout()->addWidget(progress_label);

    cancel_button = new QPushButton{this};
    layout()->addWidget(cancel_button);
    connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);
    connect(this, &QDialog::finished, this, &UpdateCostDialog::cancelUpdate);

    empty_results_view = new QListView{this};
    layout()->addWidget(empty_results_view);
    empty_results_view->setModel(new QStringListModel{this});
    empty_results_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    empty_results_view->hide();

    trade_requester = new TradeRequester{this};
    connect(trade_requester, &TradeRequester::requestFailed, this, &UpdateCostDialog::requestFailed);
    connect(trade_requester, &TradeRequester::parseFailed, this, &UpdateCostDialog::parseFailed);
    connect(trade_requester,
            &TradeRequester::noResultsFound,
            this,
            [this](Game game, const TradeRequestKey& request) {
                auto trade_cache = AppState::tradeCache(game);
                if (auto it = trade_cache->requestData(request);
                    it != trade_cache->cache.end() && !it->second.name().isEmpty())
                    empty_search_results.insert(it->second.name());
                updateProgress();
            });
    connect(trade_requester,
            &TradeRequester::requestFinished,
            this,
            &UpdateCostDialog::updateProgress);

    exchange_requester = new ExchangeRequester{this};
    connect(exchange_requester,
            &ExchangeRequester::requestFailed,
            this,
            &UpdateCostDialog::requestFailed);
    connect(exchange_requester,
            &ExchangeRequester::parseFailed,
            this,
            &UpdateCostDialog::parseFailed);
    connect(exchange_requester,
            &ExchangeRequester::requestFinished,
            this,
            &UpdateCostDialog::updateProgress);
}

void UpdateCostDialog::updatePlanItem(PlanItem& item, bool send_requests)
{
    if (isUpdateActive())
        return;

    dependencies.clear();
    item.gatherCostDependencies(dependencies);
    if (!dependencies.empty()) {
        game_ = item.game();
        setWindowTitle(tr("%1 - Update").arg(item.name()));
        startUpdate(send_requests);
    }
}

void UpdateCostDialog::updatePlan(Plan& plan, bool send_requests)
{
    if (isUpdateActive())
        return;

    dependencies.clear();
    plan.item()->gatherCostDependencies(dependencies);
    if (!dependencies.empty()) {
        game_ = plan.game;
        setWindowTitle(tr("%1 - Update").arg(plan.name));
        startUpdate(send_requests);
    }
}

void UpdateCostDialog::cancelUpdate()
{
    game_ = Game::Unknown;
    dependencies.clear();

    trade_requester->cancelRequests();
    exchange_requester->cancelRequests();

    empty_search_results.clear();
    empty_results_view->hide();
}

void UpdateCostDialog::startUpdate(bool send_requests)
{
    if (Settings::offline_mode || !send_requests || AppState::snapshots(game_)->current) {
        calculateCost();
        return;
    }

    auto plan_model = AppState::planModel(game_);
    auto trade_cache = AppState::tradeCache(game_);
    auto exchange_cache = AppState::exchangeCache(game_);

    auto now = QDateTime::currentDateTimeUtc();
    for (auto& id : dependencies) {
        auto it = plan_model->plans.find(id);
        if (it == plan_model->plans.end() || it->second.locked)
            continue;

        for (auto& step : it->second.steps) {
            for (auto& item : step.resources)
                checkItem(item, now, *exchange_cache, *trade_cache);
            for (auto& item : step.results)
                checkItem(item, now, *exchange_cache, *trade_cache);
        }
    }

    bool trade_finished = trade_requester->requests.empty();
    bool exchange_finished = exchange_requester->requests.empty();
    if (trade_finished && exchange_finished) {
        calculateCost();
        return;
    }

    if (!trade_finished) {
        checkCurrency({"chaos"}, now, *exchange_cache);
        trade_requester->startRequests();
    }

    if (!exchange_finished)
        exchange_requester->startRequests();

    cancel_button->setText(tr("Cancel"));
    progress_label->setText(tr("Requesting data..."));
    show();
}

void UpdateCostDialog::checkCurrency(const Currency& currency,
                                     QDateTime now,
                                     const ExchangeRequestCache& exchange_cache)
{
    if (!exchange_cache.isOutdated(currency, now))
        return;

    if (auto it = exchange_cache.currencyData(currency); it != exchange_cache.cache.end())
        exchange_requester->requests.emplace(it->second.type, game_);
}

void UpdateCostDialog::checkItem(const StepItem& item,
                                 QDateTime now,
                                 const ExchangeRequestCache& exchange_cache,
                                 const TradeRequestCache& trade_cache)
{
    if (auto trade = item.trade()) {
        if (!trade_cache.isOutdated(trade->request_key, now))
            return;
        if (auto it = trade_cache.requestData(trade->request_key); it != trade_cache.cache.end())
            trade_requester->requests.emplace(it->first, game_);
    } else if (auto exchange = item.exchange())
        checkCurrency(exchange->currency, now, exchange_cache);
    else if (auto custom = item.custom())
        checkCurrency(custom->cost.currency, now, exchange_cache);
}

void UpdateCostDialog::requestFailed()
{
    cancelUpdate();
    progress_label->setText(tr("Request failed."));
    cancel_button->setText(tr("Ok"));
}

void UpdateCostDialog::parseFailed()
{
    cancelUpdate();
    progress_label->setText(tr("Failed to parse reply."));
    cancel_button->setText(tr("Ok"));
}

void UpdateCostDialog::updateProgress()
{
    auto trade_count = std::ssize(trade_requester->requests);
    auto exchange_count = std::ssize(exchange_requester->requests);
    if (trade_count == 0 && exchange_count == 0) {
        QTimer::singleShot(0, this, &UpdateCostDialog::calculateCost);
        return;
    }

    auto trade_requests_estimation = AppState::state.trade_manager->currentSearchDelay()
                                     * trade_count;
    auto exchange_requests_estimation = Settings::exchangeRequestDelay() * exchange_count;
    seconds estimation{
        duration_cast<seconds>(std::max(trade_requests_estimation, exchange_requests_estimation))};

    progress_label->setText(tr("%1 requests left (%2 seconds).")
                                .arg(trade_count + exchange_count)
                                .arg(estimation.count()));
}

void UpdateCostDialog::calculateCost()
{
    if (!isUpdateActive())
        return;

    auto trade_cache = AppState::tradeCache(game_);
    auto exchange_cache = AppState::exchangeCache(game_);
    auto plan_model = AppState::planModel(game_);

    auto& cost_visitor = AppState::state.custom_calc->visitor.cost_visitor;
    cost_visitor.setModels(*exchange_cache, *trade_cache, *plan_model);

    std::vector<std::pair<Plan*, bool>> plans;
    for (auto& id : std::views::reverse(dependencies)) {
        auto it = plan_model->plans.find(id);
        if (it == plan_model->plans.end() || it->second.locked)
            continue;

        auto plan = plans.emplace_back(&it->second, false).first;
        for (auto& step : plan->steps) {
            step.resources_cost = {};
            step.results_cost = {};
            step.failed_cost = {};
        }
    }

    auto snapshot_name = AppState::snapshots(game_)->currentName();
    auto league = Settings::currentLeague(game_);
    bool parse_failed = false;
    for (auto& [plan, final_changed] : plans) {
        if (!parse_failed) {
            for (auto& step : plan->steps) {
                if (!calculateStepCost(*plan, step)) {
                    parse_failed = true;
                    break;
                }
            }
            final_changed = plan->autoSelectFinalStep();
        }

        if (snapshot_name.isNull())
            plan->league = league;
        else
            plan->league = snapshot_name;

        plan->setChanged();
    }
    emit costUpdated(game_, plans);

    if (empty_search_results.empty()) {
        if (parse_failed)
            reject();
        else
            accept();
    } else {
        progress_label->setText(tr("No results found for this searches:"));
        cancel_button->setText(tr("Ok"));
        QStringList list;
        while (!empty_search_results.empty()) {
            auto node = empty_search_results.extract(empty_search_results.begin());
            list.append(std::move(node.value()));
        }
        static_cast<QStringListModel*>(empty_results_view->model())->setStringList(list);

        empty_results_view->show();
    }
}

bool UpdateCostDialog::calculateStepCost(const Plan& step_plan, Step& step)
{
    auto& cost_visitor = AppState::state.custom_calc->visitor.cost_visitor;
    cost_visitor.setPlan(step_plan);
    cost_visitor.setIsResource(true);

    if (step.resource_calc == ResourceCalcMethod::Custom) {
        if (!calculateStepCustomCost(true, step_plan, step))
            return false;
    } else {
        std::vector<std::optional<ItemCost>> resources_cost;
        for (auto& item : step.resources) {
            item.not_used = !resources_cost.emplace_back(cost_visitor.calculateCost(item))
                                 .has_value();
        }

        auto value = [](const std::optional<ItemCost>& opt) { return *opt; };
        auto resources_view = std::views::filter(resources_cost,
                                                 &std::optional<ItemCost>::has_value);
        switch (step.resource_calc) {
        case ResourceCalcMethod::Sum:
            for (auto& cost : resources_view)
                step.resources_cost += *cost;
            break;
        case ResourceCalcMethod::Min: {
            if (auto min_it = std::ranges::min_element(resources_view, std::less{}, value);
                min_it != resources_view.end()) {
                for (auto& item : step.resources)
                    item.not_used = true;

                auto pos = std::distance(resources_cost.begin(), min_it.base());
                step.resources[pos].not_used = false;
                step.resources_cost = *(*min_it);
            }
            break;
        }
        case ResourceCalcMethod::Custom:
            break;
        }
    }

    cost_visitor.setIsResource(false);
    std::vector<std::pair<std::optional<ItemCost>, bool>> results_cost;
    auto calcResult = [&](StepItem& item) {
        auto& cost = results_cost
                         .emplace_back(cost_visitor.calculateCost(item), item.is_success_result)
                         .first;
        item.not_used = !cost.has_value();
    };

    if (step.result_calc == ResultCalcMethod::Custom) {
        if (!calculateStepCustomCost(false, step_plan, step))
            return false;

        for (auto& item : step.results) {
            if (item.is_success_result) {
                results_cost.emplace_back(std::optional<ItemCost>{}, true);
                continue;
            }
            calcResult(item);
        }
    } else {
        for (auto& item : step.results)
            calcResult(item);

        auto hasSuccess = [](auto& p) { return p.first.has_value() && p.second; };
        auto resultValue = [](auto& p) { return *p.first; };
        auto success_view = std::views::filter(results_cost, hasSuccess);
        switch (step.result_calc) {
        case ResultCalcMethod::Sum:
            for (auto& cost : success_view)
                step.results_cost += *cost.first;
            break;
        case ResultCalcMethod::Max:
            if (auto max_it = std::ranges::max_element(success_view, std::less{}, resultValue);
                max_it != success_view.end()) {
                for (auto& item : step.results) {
                    if (item.is_success_result)
                        item.not_used = true;
                }

                auto pos = std::distance(results_cost.begin(), max_it.base());
                step.results[pos].not_used = false;
                step.results_cost = *max_it->first;
            }
            break;
        case ResultCalcMethod::Custom:
            break;
        }
    }

    auto hasFailed = [](auto& p) { return p.first.has_value() && !p.second; };
    auto failed_view = std::views::filter(results_cost, hasFailed);
    for (auto& cost : failed_view)
        step.failed_cost += *cost.first;

    return true;
}

bool UpdateCostDialog::calculateStepCustomCost(bool is_resource_cost,
                                               const Plan& step_plan,
                                               Step& step)
{
    auto& cost = is_resource_cost ? step.resources_cost : step.results_cost;
    auto& items = is_resource_cost ? step.resources : step.results;
    auto& custom_data = is_resource_cost ? step.custom_resource_data : step.custom_result_data;

    AppState::state.custom_calc->visitor.setItems(items, is_resource_cost);

    auto result = AppState::state.custom_calc->calculate(custom_data);
    if (!result) {
        auto msg = new QMessageBox{this};
        msg->setAttribute(Qt::WA_DeleteOnClose);
        msg->setWindowTitle(tr("Parse Failed"));
        if (is_resource_cost)
            msg->setText(
                tr("Failed to parse custom expression for resources of step \"%1\" in plan \"%2\".")
                    .arg(step.name, step_plan.name));
        else
            msg->setText(
                tr("Failed to parse custom expression for results of step \"%1\" in plan \"%2\".")
                    .arg(step.name, step_plan.name));
        msg->open();
        return false;
    }

    if (result->cost)
        cost = std::move(*result->cost);

    for (int i = 0; i < std::ssize(items); ++i)
        items[i].not_used = !result->used_items.contains(i);

    return true;
}

} // namespace planner
