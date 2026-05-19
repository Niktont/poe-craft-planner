#include "TradeRequester.h"
#include "AppState.h"
#include "TradeRequestCache.h"
#include "TradeRequestManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRestReply>
#include <QTimer>

namespace planner {

TradeRequester::TradeRequester(QObject* parent)
    : QObject{parent}
{}

void TradeRequester::startRequests()
{
    if (!isActive())
        requestWithDelay();
}

void TradeRequester::cancelRequests()
{
    requests.clear();
    if (reply) {
        reply->disconnect(connection);
        reply = nullptr;
    }
}

void TradeRequester::requestWithDelay()
{
    if (requests.empty())
        return;

    is_delay_active = true;
    QTimer::singleShot(AppState::state.trade_manager->searchDelay(),
                       this,
                       &TradeRequester::requestSearch);
}

void TradeRequester::requestSearch()
{
    is_delay_active = false;
    if (requests.empty())
        return;

    auto node = requests.extract(requests.begin());
    auto trade_cache = AppState::tradeCache(node.mapped());
    auto it = trade_cache->requestData(node.key());
    while (it == trade_cache->cache.end() && !requests.empty()) {
        node = requests.extract(requests.begin());
        trade_cache = AppState::tradeCache(node.mapped());
        it = trade_cache->requestData(node.key());
    }
    if (it == trade_cache->cache.end())
        return;

    reply = AppState::state.trade_manager->postSearchRequest(node.mapped(),
                                                             it->first,
                                                             it->second.query());
    connection = connect(reply, &QNetworkReply::finished, this, [this, node = std::move(node)] {
        parseSearch(node.mapped(), node.key());
    });
}

void TradeRequester::parseSearch(Game game, const TradeRequestKey& request)
{
    auto result = AppState::state.trade_manager->parseSearchReply(*reply, requests.size());
    reply = nullptr;
    if (result.has_error()) {
        switch (result.assume_error()) {
        case TradeRequestManager::RequestFailed:
            emit requestFailed();
            break;
        case TradeRequestManager::ParseFailed:
            emit parseFailed();
            break;
        }
        requestWithDelay();
        return;
    }

    auto& [total, items] = result.assume_value();
    if (items.empty()) {
        AppState::tradeCache(game)->updateCost(request, {QDateTime::currentDateTimeUtc(), {}});
        emit noResultsFound(game, request);
        requestWithDelay();
        return;
    }

    auto items_to_fetch = std::clamp(items.size() / 10, 1ull, 10ull);
    reply = AppState::state.trade_manager->fetchItems(game,
                                                      request,
                                                      {items.begin(), items_to_fetch});
    connection = connect(reply, &QNetworkReply::finished, this, [this, game, request, total] {
        parseFetch(game, request, total);
    });
}

void TradeRequester::parseFetch(Game game, const TradeRequestKey& request, int total)
{
    auto result = TradeRequestManager::parseFetchReply(*reply,
                                                       request,
                                                       total,
                                                       *AppState::exchangeCache(game),
                                                       *AppState::tradeCache(game));
    reply = nullptr;
    if (result.has_error()) {
        switch (result.assume_error()) {
        case TradeRequestManager::RequestFailed:
            emit requestFailed();
            break;
        case TradeRequestManager::ParseFailed:
            emit parseFailed();
            break;
        }
        requestWithDelay();
        return;
    }

    emit requestFinished(requests.size());
    requestWithDelay();
}

} // namespace planner
