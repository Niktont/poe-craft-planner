#include "ExchangeRequester.h"
#include "AppState.h"
#include "ExchangeRequestManager.h"
#include "Settings.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRestReply>
#include <QTimer>

namespace planner {

ExchangeRequester::ExchangeRequester(QObject* parent)
    : QObject{parent}
{}

void ExchangeRequester::startRequests(bool parse_items_)
{
    parse_items = parse_items_;
    if (!isActive())
        requestOverview();
}

void ExchangeRequester::cancelRequests()
{
    requests.clear();
    if (reply) {
        reply->disconnect(connection);
        reply = nullptr;
    }
}

void ExchangeRequester::requestWithDelay()
{
    if (requests.empty())
        return;

    is_delay_active = true;
    QTimer::singleShot(Settings::exchangeRequestDelay(), this, &ExchangeRequester::requestOverview);
}

void ExchangeRequester::requestOverview()
{
    is_delay_active = false;
    if (requests.empty())
        return;

    auto node = requests.extract(requests.begin());

    reply = AppState::state.exchange_manager->getOverview(node.value().second, node.value().first);
    connection = connect(reply, &QNetworkReply::finished, this, [this, node = std::move(node)] {
        parseOverview(node.value().second, node.value().first);
    });
}

void ExchangeRequester::parseOverview(Game game, const QString& type)
{
    QRestReply rest{reply};
    reply = nullptr;
    if (!rest.isSuccess()) {
        emit requestFailed();
        requestWithDelay();
        return;
    }

    auto json = rest.readJson();
    if (!json) {
        emit parseFailed();
        requestWithDelay();
        return;
    }

    auto cache = AppState::exchangeCache(game);
    const auto obj = json->object();
    if ((parse_items && !AppState::state.exchange_manager->parseOverviewItems(obj, type, *cache))
        || !ExchangeRequestManager::parseOverviewCosts(obj, *cache)
        || !ExchangeRequestManager::parseCore(obj["core"].toObject(), *cache)) {
        emit parseFailed();
        requestWithDelay();
        return;
    }

    emit requestFinished(requests.size());
    requestWithDelay();
}

} // namespace planner
