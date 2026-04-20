#ifndef TRADEREQUESTER_H
#define TRADEREQUESTER_H

#include "Game.h"
#include <map>
#include <QObject>

class QNetworkReply;

namespace planner {
class TradeRequestKey;

class TradeRequester : public QObject
{
    Q_OBJECT
public:
    explicit TradeRequester(QObject* parent = nullptr);

    std::map<TradeRequestKey, Game> requests;

    void startRequests();
    void cancelRequests();

    bool isActive() const { return is_delay_active || reply; }

signals:
    void requestFailed();
    void parseFailed();

    void requestFinished(size_t requests_left);
    void noResultsFound(planner::Game game, const planner::TradeRequestKey& request);

private slots:
    void requestSearch();

private:
    bool is_delay_active{false};

    QNetworkReply* reply{};
    QMetaObject::Connection connection;

    void requestWithDelay();

    void parseSearch(Game game, const TradeRequestKey& request);
    void parseFetch(Game game, const TradeRequestKey& request, int total);
};

} // namespace planner

#endif // TRADEREQUESTER_H
