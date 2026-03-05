#include "UpdateCostDialog.h"
#include "ExchangeRequestCache.h"
#include "ExchangeRequestManager.h"
#include "MainWindow.h"
#include "Plan.h"
#include "Settings.h"
#include "TradeRequestManager.h"
#include <QCloseEvent>
#include <QLabel>
#include <QListView>
#include <QNetworkReply>
#include <QPushButton>
#include <QRestReply>
#include <QStringListModel>
#include <QTimer>
#include <QVBoxLayout>

using namespace std::chrono;

namespace planner {

UpdateCostDialog::UpdateCostDialog(MainWindow& mw)
    : QDialog{&mw}
{
    setWindowTitle(tr("Update Costs"));

    auto main_layout = new QVBoxLayout{};
    setLayout(main_layout);

    setMinimumWidth(300);

    progress_label = new QLabel{};
    layout()->addWidget(progress_label);

    cancel_button = new QPushButton{};
    layout()->addWidget(cancel_button);
    connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);
    connect(this, &QDialog::rejected, this, &UpdateCostDialog::cancelUpdate);

    empty_results_view = new QListView{};
    layout()->addWidget(empty_results_view);
    empty_results_view->setModel(new QStringListModel{});
    empty_results_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    empty_results_view->hide();

    main_layout->addStretch();
}

void UpdateCostDialog::updatePlan(Plan* plan)
{
    if (!plan)
        return;

    this->plan = plan;
    cancel_button->setText(tr("Cancel"));
    progress_label->setText(tr("Requesting cost data..."));

    auto trade_cache = mw()->tradeCache(plan->game);

    auto now = QDateTime::currentDateTimeUtc();
    for (auto& step : plan->steps) {
        for (auto& item : step.resources)
            checkItem(item, now, *trade_cache);
        for (auto& item : step.results)
            checkItem(item, now, *trade_cache);
    }
    trade_finished = trade_requests.empty();
    if (!trade_finished) {
        checkCurrency({"chaos"}, now);
        if (!is_active_trade) {
            QTimer::singleShot(mw()->trade_manager->searchDelay(),
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
        open();
}

void UpdateCostDialog::cancelUpdate()
{
    plan = nullptr;
    clearRequests();
}

void UpdateCostDialog::closeEvent(QCloseEvent* event)
{
    event->accept();
    if (plan)
        cancelUpdate();
}

void UpdateCostDialog::requestTradeSearch()
{
    is_active_trade = false;
    if (!plan)
        return;

    if (trade_requests.empty()) {
        trade_finished = true;
        calculateCost();
        return;
    }

    auto it = trade_requests.front();
    trade_requests.pop();

    is_active_trade = true;
    auto reply = mw()->trade_manager->postSearchRequest(plan->game, it->first, it->second.query());
    connect(reply, &QNetworkReply::finished, this, [=, this, game = plan->game, request = it->first] {
        parseTradeSearch(game, request, reply);
    });
}

void UpdateCostDialog::requestExchangeCost()
{
    is_active_exchange = false;
    if (!plan)
        return;

    if (exchange_requests.empty()) {
        exchange_finished = true;
        calculateCost();
        return;
    }

    is_active_exchange = true;
    auto type{std::move(exchange_requests.extract(exchange_requests.begin()).value())};
    auto reply = mw()->exchange_manager->getOverview(plan->game, type);
    connect(reply, &QNetworkReply::finished, this, [this, game = plan->game, reply] {
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

    auto items = mw()->trade_manager->parseSearchReply(reply, json->object(), trade_requests.size());
    if (items.first == 0) {
        auto trade_cache = mw()->tradeCache(game);
        trade_cache->updateCost(request, {QDateTime::currentDateTimeUtc(), {}});

        if (auto it = trade_cache->requestData(request);
            it != trade_cache->cache.end() && !it->second.name().isEmpty())
            empty_search_results.insert(it->second.name());

        if (!trade_requests.empty()) {
            updateProgress();
            is_active_trade = true;
            QTimer::singleShot(mw()->trade_manager->searchDelay(),
                               this,
                               &UpdateCostDialog::requestTradeSearch);
        } else {
            trade_finished = true;
            if (plan)
                calculateCost();
        }
    } else {
        is_active_trade = true;
        size_t items_to_fetch = std::clamp(items.second.size() / 10, 1ull, 10ull);
        auto fetch_reply = mw()->trade_manager->fetchItems(game,
                                                           request,
                                                           {items.second.begin(), items_to_fetch});
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
                                              *mw()->exchangeCache(game),
                                              *mw()->tradeCache(game))) {
        parseFailed();
        return;
    }

    if (!trade_requests.empty()) {
        updateProgress();
        is_active_trade = true;
        QTimer::singleShot(mw()->trade_manager->searchDelay(),
                           this,
                           &UpdateCostDialog::requestTradeSearch);
    } else {
        trade_finished = true;
        if (plan)
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
    auto exchange_cache = mw()->exchangeCache(game);
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
        if (plan)
            calculateCost();
    }
}

MainWindow* UpdateCostDialog::mw() const
{
    return static_cast<MainWindow*>(parent());
}

void UpdateCostDialog::checkCurrency(const Currency& currency, QDateTime now)
{
    auto exchange_cache = mw()->exchangeCache(plan->game);
    if (!exchange_cache->isOutdated(currency, now))
        return;

    if (auto it = exchange_cache->currencyData(currency); it != exchange_cache->cache.end())
        exchange_requests.insert(it->second.type);
}

void UpdateCostDialog::checkItem(const StepItem& item,
                                 QDateTime now,
                                 const TradeRequestCache& trade_cache)
{
    if (auto trade = item.trade()) {
        if (!trade_cache.isOutdated(trade->request_key, now))
            return;
        if (auto it = trade_cache.requestData(trade->request_key); it != trade_cache.cache.end())
            trade_requests.push(it);
    } else if (auto exchange = item.exchange())
        checkCurrency(exchange->currency, now);
    else if (auto custom = item.custom())
        checkCurrency(custom->cost.currency, now);
}

void UpdateCostDialog::clearRequests()
{
    while (!trade_requests.empty())
        trade_requests.pop();

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

    auto trade_requests_estimation = mw()->trade_manager->currentSearchDelay()
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
    if (!plan || !trade_finished || !exchange_finished)
        return;

    for (auto& step : plan->steps)
        calculateStepCost(step);

    plan->league = Settings::currentLeague(plan->game);
    plan->setChanged();

    emit costUpdated(plan);

    if (empty_search_results.empty())
        accept();
    else {
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

void UpdateCostDialog::calculateStepCost(Step& step)
{
    auto trade_cache = mw()->tradeCache(plan->game);
    auto exchange_cache = mw()->exchangeCache(plan->game);
    std::vector<ItemCost> resources_cost;
    for (auto& item : step.resources)
        resources_cost.push_back(item.calculateCost(*plan, *exchange_cache, *trade_cache));

    std::vector<ItemCost> results_cost;
    std::vector<ItemCost> failed_cost;
    for (auto& item : step.results) {
        if (item.is_success_result)
            results_cost.push_back(item.calculateCost(*plan, *exchange_cache, *trade_cache));
        else {
            auto& cost = failed_cost.emplace_back(
                item.calculateCost(*plan, *exchange_cache, *trade_cache));
            if (item.type() != StepItemType::Step) {
                cost.gold = 0.0;
                cost.time = {};
            }
        }
    }

    step.resources_cost = {};
    switch (step.resource_calc) {
    case ResourceCalcMethod::Sum:
        step.resources_cost = std::accumulate(resources_cost.begin(),
                                              resources_cost.end(),
                                              step.resources_cost);
        break;
    case ResourceCalcMethod::Min: {
        if (auto min_it = std::min_element(resources_cost.begin(), resources_cost.end());
            min_it != resources_cost.end())
            step.resources_cost = *min_it;
        break;
    }
    }
    step.results_cost = {};
    switch (step.result_calc) {
    case ResultCalcMethod::Sum:
        step.results_cost = std::accumulate(results_cost.begin(),
                                            results_cost.end(),
                                            step.results_cost);
        break;
    case ResultCalcMethod::Max: {
        if (auto max_it = std::max_element(results_cost.begin(), results_cost.end());
            max_it != results_cost.end())
            step.results_cost = *max_it;
        break;
    }
    }
    step.failed_cost = std::accumulate(failed_cost.begin(), failed_cost.end(), ItemCost{});
}

} // namespace planner
