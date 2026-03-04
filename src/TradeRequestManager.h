#ifndef TRADEREQUESTMANAGER_H
#define TRADEREQUESTMANAGER_H

#include "CurrencyCost.h"
#include "Game.h"
#include <QObject>

class QRestAccessManager;
class QNetworkReply;
class QWebEngineView;

namespace planner {
class TradeRequestKey;
class TradeRequestCache;

class TradeRequestManager : public QObject
{
    Q_OBJECT
public:
    explicit TradeRequestManager(QRestAccessManager& manager,
                                 QWebEngineView& web_view,
                                 QObject* parent = nullptr);

    QNetworkReply* postSearchRequest(Game game,
                                     const TradeRequestKey& key,
                                     const QJsonDocument& query) const;

    QNetworkReply* fetchItems(Game game,
                              const TradeRequestKey& key,
                              const std::span<QString>& items) const;

    std::pair<int, std::vector<QString>> parseSearchReply(QNetworkReply* reply,
                                                          const QJsonObject& reply_o,
                                                          size_t request_count);

    std::chrono::milliseconds searchDelay() const;
    std::chrono::milliseconds currentSearchDelay() const { return trade_search_delay; }

    static bool parseFetchReply(const QJsonObject& reply,
                                const TradeRequestKey& request,
                                int total,
                                const ExchangeRequestCache& exchange_cache,
                                TradeRequestCache& trade_cache);

private:
    QRestAccessManager* manager;
    QWebEngineView* web_view;
    QString userAgent() const;

    QDateTime trade_search_finished_time;
    std::chrono::milliseconds trade_search_delay{2000};

    static std::chrono::milliseconds findDelay(QNetworkReply* reply, size_t request_count);
};

} // namespace planner

#endif // TRADEREQUESTMANAGER_H
