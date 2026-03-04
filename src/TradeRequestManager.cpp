#include "TradeRequestManager.h"

#include "ExchangeRequestCache.h"
#include "TradeRequestCache.h"
#include "TradeRequestKey.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRestAccessManager>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>

namespace planner {

TradeRequestManager::TradeRequestManager(QRestAccessManager& manager,
                                         QWebEngineView& web_view,
                                         QObject* parent)
    : QObject{parent}
    , manager{&manager}
    , web_view{&web_view}
{
}

QNetworkReply* TradeRequestManager::postSearchRequest(Game game,
                                                      const TradeRequestKey& key,
                                                      const QJsonDocument& query) const
{
    QNetworkRequest request;

    request.setUrl(key.toSearchUrl(game));

    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent());

    request.setRawHeader("accept", "application/json");
    request.setRawHeader("content-type", "application/json");
    request.setRawHeader("priority", "u=4, i");
    request.setRawHeader("sec-fetch-dest", "empty");
    request.setRawHeader("sec-fetch-mode", "no-cors");
    request.setRawHeader("sec-fetch-site", "none");

    return manager->post(request, query);
}

QNetworkReply* TradeRequestManager::fetchItems(Game game,
                                               const TradeRequestKey& key,
                                               const std::span<QString>& items) const
{
    QNetworkRequest request;

    request.setUrl(key.toFetchUrl(game, items));

    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent());

    request.setRawHeader("accept", "*/*");
    request.setRawHeader("priority", "u=4, i");
    request.setRawHeader("sec-fetch-dest", "empty");
    request.setRawHeader("sec-fetch-mode", "no-cors");
    request.setRawHeader("sec-fetch-site", "none");

    return manager->get(request);
}

std::pair<int, std::vector<QString>> TradeRequestManager::parseSearchReply(
    QNetworkReply* reply, const QJsonObject& reply_o, size_t request_count)
{
    std::pair<int, std::vector<QString>> result;
    const auto result_a = reply_o["result"].toArray();

    result.second.reserve(result_a.size());
    for (auto item_v : result_a)
        result.second.push_back(item_v.toString());
    result.first = reply_o["total"].toInt();

    trade_search_delay = findDelay(reply, request_count);
    trade_search_finished_time = QDateTime::currentDateTimeUtc();

    return result;
}

std::chrono::milliseconds TradeRequestManager::searchDelay() const
{
    auto now = QDateTime::currentDateTimeUtc();
    if (trade_search_finished_time + trade_search_delay <= now)
        return {};
    return trade_search_delay - (now - trade_search_finished_time);
}

bool TradeRequestManager::parseFetchReply(const QJsonObject& reply,
                                          const TradeRequestKey& request,
                                          int total,
                                          const ExchangeRequestCache& exchange_cache,
                                          TradeRequestCache& trade_cache)
{
    double fee{};
    const auto result_a = reply["result"].toArray();
    std::unordered_map<QString, std::pair<double, int>> prices;
    for (auto item_v : result_a) {
        auto item_o = item_v.toObject();
        auto listing_o = item_o["listing"].toObject();
        fee += listing_o["fee"].toDouble();

        auto price_o = listing_o["price"].toObject();
        auto currency = price_o["currency"].toString();
        if (currency.isEmpty())
            continue;
        auto it = prices.try_emplace(currency).first;
        it->second.first += price_o["amount"].toDouble();
        it->second.second += 1;
    }
    if (prices.empty())
        return false;
    fee /= result_a.size();

    auto common_it = std::ranges::max_element(prices, [](const auto& l, const auto& r) {
        return l.second.second < r.second.second;
    });
    CurrencyCost cost;
    cost.value = common_it->second.first / common_it->second.second;
    cost.currency.id = common_it->first;

    exchange_cache.prepareCurrency(cost.currency);
    trade_cache.updateCost(request, {QDateTime::currentDateTimeUtc(), cost, fee, total});

    return true;
}

std::chrono::milliseconds TradeRequestManager::findDelay(QNetworkReply* reply, size_t request_count)
{
    static constexpr QByteArrayView rule_names_header{"x-rate-limit-rules"};
    static constexpr QByteArrayView base{"x-rate-limit-"};

    auto headers = reply->headers();
    const auto rule_names = headers.combinedValue(rule_names_header).split(',');
    QList<QByteArray> rules;
    QList<QByteArray> states;
    for (auto rule : rule_names) {
        rules.append(headers.combinedValue(base % rule).split(','));
        states.append(headers.combinedValue(base % rule % "-state").split(','));
    }
    std::vector<std::pair<size_t, double>> delays;
    for (qsizetype i = 0; i < rules.size(); ++i) {
        auto rule_parts = rules[i].split(':');
        auto state_parts = states[i].split(':');

        int max_hits = rule_parts[0].toInt();
        double test_period = rule_parts[1].toInt();
        int state = state_parts[0].toInt();
        int hits_left = max_hits - state + 1;

        delays.emplace_back(hits_left, test_period / hits_left);
    }
    if (delays.empty())
        return std::chrono::milliseconds{2500};

    std::ranges::sort(delays, [](const auto& l, const auto& r) { return l.first < r.first; });
    auto it = std::ranges::find_if(delays, [request_count](const auto& p) {
        return p.first >= request_count;
    });

    auto max_it = std::ranges::max_element(delays.begin(),
                                           it == delays.end() ? it : ++it,
                                           [](const auto& l, const auto& r) {
                                               return l.second < r.second;
                                           });
    double delay = max_it->second;

    return std::chrono::milliseconds{static_cast<long long>(std::ceil(delay * 1000))};
}

QString TradeRequestManager::userAgent() const
{
    return web_view->page()->profile()->httpUserAgent();
}
} // namespace planner
