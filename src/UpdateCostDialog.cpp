#include "UpdateCostDialog.h"
#include "AppState.h"
#include "CustomEditDialog.h"
#include "ExchangeRequestCache.h"
#include "ExchangeRequestManager.h"
#include "Plan.h"
#include "PlanModel.h"
#include "Settings.h"
#include "SnapshotModel.h"
#include "StepItem.h"
#include "TradeRequestCache.h"
#include "TradeRequestManager.h"
#include <QCloseEvent>
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

    progress_label = new QLabel{};
    layout()->addWidget(progress_label);

    cancel_button = new QPushButton{};
    layout()->addWidget(cancel_button);
    connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);
    connect(this, &QDialog::finished, this, &UpdateCostDialog::cancelUpdate);

    empty_results_view = new QListView{};
    layout()->addWidget(empty_results_view);
    empty_results_view->setModel(new QStringListModel{this});
    empty_results_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    empty_results_view->hide();
}

void UpdateCostDialog::updatePlan(Plan* plan, bool send_requests)
{
    if (!plan || plan->locked || !this->isHidden())
        return;

    plan_ = plan;
    setWindowTitle(tr("%1 - Update").arg(plan_->name));

    dependencies.clear();
    dependencies.push_back(plan->id());
    auto plan_model = AppState::planModel(plan->game);
    plan->gatherDependencies(*plan_model, dependencies);

    if (!send_requests || AppState::snapshots(plan->game)->current) {
        trade_finished = true;
        exchange_finished = true;
        calculateCost();
        return;
    }

    cancel_button->setText(tr("Cancel"));
    progress_label->setText(tr("Requesting data..."));

    auto trade_cache = AppState::tradeCache(plan->game);
    auto exchange_cache = AppState::exchangeCache(plan->game);

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

    trade_finished = trade_requests.empty();
    if (!trade_finished) {
        checkCurrency({"chaos"}, now, *exchange_cache);
        if (!is_active_trade) {
            QTimer::singleShot(AppState::state.trade_manager->searchDelay(),
                               this,
                               &UpdateCostDialog::requestTradeSearch);
        }
    }

    exchange_finished = exchange_requests.empty();
    if (!exchange_finished && !is_active_exchange)
        requestExchangeCost();

    if (trade_finished && exchange_finished && !is_active_trade && !is_active_exchange)
        calculateCost();
    else
        show();
}

void UpdateCostDialog::cancelUpdate()
{
    plan_ = nullptr;
    clearRequests();
    dependencies.clear();
}

void UpdateCostDialog::closeEvent(QCloseEvent* event)
{
    event->accept();
    if (plan_)
        cancelUpdate();
}

void UpdateCostDialog::requestTradeSearch()
{
    is_active_trade = false;
    if (!plan_)
        return;

    if (trade_requests.empty()) {
        trade_finished = true;
        calculateCost();
        return;
    }

    auto trade_cache = AppState::tradeCache(plan_->game);

    auto node = trade_requests.extract(trade_requests.begin());
    auto it = trade_cache->requestData(node.value());
    while (!trade_requests.empty() && it == trade_cache->cache.end()) {
        node = trade_requests.extract(trade_requests.begin());
        it = trade_cache->requestData(node.value());
    }
    if (it == trade_cache->cache.end()) {
        trade_finished = true;
        calculateCost();
        return;
    }

    is_active_trade = true;
    auto reply = AppState::state.trade_manager->postSearchRequest(plan_->game,
                                                                  it->first,
                                                                  it->second.query());
    connect(reply,
            &QNetworkReply::finished,
            this,
            [=, this, game = plan_->game, request = it->first] {
                parseTradeSearch(game, request, reply);
            });
}

void UpdateCostDialog::requestExchangeCost()
{
    is_active_exchange = false;
    if (!plan_)
        return;

    if (exchange_requests.empty()) {
        exchange_finished = true;
        calculateCost();
        return;
    }

    is_active_exchange = true;
    auto type{std::move(exchange_requests.extract(exchange_requests.begin()).value())};
    auto reply = AppState::state.exchange_manager->getOverview(plan_->game, type);
    connect(reply, &QNetworkReply::finished, this, [this, game = plan_->game, reply] {
        parseExchangeCostData(game, reply);
    });
}

void UpdateCostDialog::parseTradeSearch(Game game,
                                        const TradeRequestKey& request,
                                        QNetworkReply* reply)
{
    is_active_trade = false;

    QRestReply rest(reply);
    if (!rest.isSuccess()) {
        requestFailed();
        return;
    }
    auto json = rest.readJson();
    if (!json) {
        parseFailed();
        return;
    }

    auto items = AppState::state.trade_manager->parseSearchReply(reply,
                                                                 json->object(),
                                                                 trade_requests.size());
    if (items.first == 0) {
        auto trade_cache = AppState::tradeCache(game);
        trade_cache->updateCost(request, {QDateTime::currentDateTimeUtc(), {}});

        if (auto it = trade_cache->requestData(request);
            it != trade_cache->cache.end() && !it->second.name().isEmpty())
            empty_search_results.insert(it->second.name());

        if (!trade_requests.empty()) {
            updateProgress();
            is_active_trade = true;
            QTimer::singleShot(AppState::state.trade_manager->searchDelay(),
                               this,
                               &UpdateCostDialog::requestTradeSearch);
        } else {
            trade_finished = true;
            if (plan_)
                calculateCost();
        }
    } else {
        is_active_trade = true;
        size_t items_to_fetch = std::clamp(items.second.size() / 10, 1ull, 10ull);
        auto fetch_reply = AppState::state.trade_manager->fetchItems(game,
                                                                     request,
                                                                     {items.second.begin(),
                                                                      items_to_fetch});
        connect(fetch_reply, &QNetworkReply::finished, this, [=, this, total = items.first] {
            parseFetchSearch(game, request, total, fetch_reply);
        });
    }
}

void UpdateCostDialog::parseFetchSearch(Game game,
                                        const TradeRequestKey& request,
                                        int total,
                                        QNetworkReply* reply)
{
    is_active_trade = false;

    QRestReply rest(reply);
    if (!rest.isSuccess()) {
        requestFailed();
        return;
    }
    auto json = rest.readJson();
    if (!json) {
        parseFailed();
        return;
    }
    if (!TradeRequestManager::parseFetchReply(json->object(),
                                              request,
                                              total,
                                              *AppState::exchangeCache(game),
                                              *AppState::tradeCache(game))) {
        parseFailed();
        return;
    }

    if (!trade_requests.empty()) {
        updateProgress();
        is_active_trade = true;
        QTimer::singleShot(AppState::state.trade_manager->searchDelay(),
                           this,
                           &UpdateCostDialog::requestTradeSearch);
    } else {
        trade_finished = true;
        if (plan_)
            calculateCost();
    }
}

void UpdateCostDialog::parseExchangeCostData(Game game, QNetworkReply* reply)
{
    is_active_exchange = false;

    QRestReply rest(reply);
    if (!rest.isSuccess()) {
        requestFailed();
        return;
    }
    auto json = rest.readJson();
    if (!json) {
        parseFailed();
        return;
    }

    const auto obj = json->object();
    auto exchange_cache = AppState::exchangeCache(game);
    if (!ExchangeRequestManager::parseOverviewCosts(obj, *exchange_cache)
        || !ExchangeRequestManager::parseCore(obj["core"].toObject(), *exchange_cache)) {
        parseFailed();
        return;
    }
    if (!exchange_requests.empty()) {
        updateProgress();
        is_active_exchange = true;
        QTimer::singleShot(Settings::exchangeRequestDelay(),
                           this,
                           &UpdateCostDialog::requestExchangeCost);
    } else {
        exchange_finished = true;
        if (plan_)
            calculateCost();
    }
}

void UpdateCostDialog::checkCurrency(const Currency& currency,
                                     QDateTime now,
                                     const ExchangeRequestCache& exchange_cache)
{
    if (!exchange_cache.isOutdated(currency, now))
        return;

    if (auto it = exchange_cache.currencyData(currency); it != exchange_cache.cache.end())
        exchange_requests.insert(it->second.type);
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
            trade_requests.insert(it->first);
    } else if (auto exchange = item.exchange())
        checkCurrency(exchange->currency, now, exchange_cache);
    else if (auto custom = item.custom())
        checkCurrency(custom->cost.currency, now, exchange_cache);
}

void UpdateCostDialog::clearRequests()
{
    trade_requests.clear();
    exchange_requests.clear();
    empty_search_results.clear();
    empty_results_view->hide();
}

void UpdateCostDialog::requestFailed()
{
    clearRequests();
    progress_label->setText(tr("Request failed."));
    cancel_button->setText(tr("Ok"));
}

void UpdateCostDialog::parseFailed()
{
    clearRequests();
    progress_label->setText(tr("Failed to parse reply."));
    cancel_button->setText(tr("Ok"));
}

void UpdateCostDialog::updateProgress()
{
    auto request_count = trade_requests.size() + exchange_requests.size();

    auto trade_requests_estimation = AppState::state.trade_manager->currentSearchDelay()
                                     * std::ssize(trade_requests);
    auto exchange_requests_estimation = Settings::exchangeRequestDelay()
                                        * std::ssize(exchange_requests);
    seconds estimation{
        duration_cast<seconds>(std::max(trade_requests_estimation, exchange_requests_estimation))};

    progress_label->setText(
        tr("%1 requests left (%2 seconds).").arg(request_count).arg(estimation.count()));
}

void UpdateCostDialog::calculateCost()
{
    if (!plan_ || !trade_finished || !exchange_finished || plan_->locked)
        return;

    auto plan_model = AppState::planModel(plan_->game);

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

    auto snapshot_name = AppState::snapshots(plan_->game)->currentName();
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

        if (snapshot_name.isEmpty())
            plan->league = Settings::currentLeague(plan->game);
        else
            plan->league = snapshot_name;

        plan->setChanged();
    }
    emit costUpdated(plan_->game, plans);

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
    auto trade_cache = AppState::tradeCache(step_plan.game);
    auto exchange_cache = AppState::exchangeCache(step_plan.game);
    auto plan_model = AppState::planModel(step_plan.game);

    auto& custom_visitor = AppState::state.custom_edit_dialog->calc.visitor;
    custom_visitor.setPlan(step_plan);
    custom_visitor.setModels(*exchange_cache, *trade_cache, *plan_model);

    if (step.resource_calc == ResourceCalcMethod::Custom) {
        if (!calculateStepCustomCost(true, step_plan, step))
            return false;
    } else {
        std::vector<std::optional<ItemCost>> resources_cost;
        for (auto& item : step.resources) {
            item.not_used = !resources_cost
                                 .emplace_back(item.calculateCost(step_plan,
                                                                  *exchange_cache,
                                                                  *trade_cache,
                                                                  *plan_model))
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

    std::vector<std::pair<std::optional<ItemCost>, bool>> results_cost;
    auto calcResult = [&](const StepItem& item) -> std::optional<ItemCost>& {
        auto& cost = results_cost
                         .emplace_back(item.calculateCost(step_plan,
                                                          *exchange_cache,
                                                          *trade_cache,
                                                          *plan_model),
                                       item.is_success_result)
                         .first;
        if (cost && item.type() != StepItemType::Step && item.type() != StepItemType::Plan) {
            if (!cost->isValid())
                cost.reset();
            else {
                cost->gold = 0.0;
                cost->time = {};
            }
        }
        return cost;
    };

    if (step.result_calc == ResultCalcMethod::Custom) {
        if (!calculateStepCustomCost(false, step_plan, step))
            return false;

        for (auto& item : step.results) {
            if (item.is_success_result) {
                results_cost.emplace_back(std::optional<ItemCost>{}, true);
                continue;
            }
            auto& cost = calcResult(item);
            if (item.not_used)
                item.not_used = !cost.has_value();
        }
    } else {
        for (auto& item : step.results) {
            auto& cost = calcResult(item);
            item.not_used = !cost.has_value();
        }

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
    auto& calc = AppState::state.custom_edit_dialog->calc;

    calc.visitor.setItems(items);

    std::optional<CustomResult> result;
    try {
        result = calc.calculate(custom_data);
    } catch (ParseException&) {
        custom_data.tree.reset();
    }
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
