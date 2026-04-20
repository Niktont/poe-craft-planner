#ifndef EXCHANGEREQUESTER_H
#define EXCHANGEREQUESTER_H

#include "Game.h"
#include <unordered_map>
#include <QObject>

class QNetworkReply;

namespace planner {

class ExchangeRequester : public QObject
{
    Q_OBJECT
public:
    explicit ExchangeRequester(QObject* parent = nullptr);

    std::unordered_map<QString, Game> requests;

    void startRequests(bool parse_items = false);
    void cancelRequests();

    bool isActive() const { return is_delay_active || reply; }

signals:
    void requestFailed();
    void parseFailed();

    void requestFinished(size_t requests_left);

private slots:
    void requestOverview();

private:
    bool parse_items{false};
    bool is_delay_active{false};

    QNetworkReply* reply{};
    QMetaObject::Connection connection;

    void requestWithDelay();

    void parseOverview(Game game, const QString& type);
};

} // namespace planner

#endif // EXCHANGEREQUESTER_H
