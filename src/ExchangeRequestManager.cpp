#include "ExchangeRequestManager.h"
#include "ExchangeRequestCache.h"
#include "MainWindow.h"
#include "Settings.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QRestAccessManager>
#include <QRestReply>
#include <QUrlQuery>
#include <QWebEnginePage>
#include <QWebEngineView>

using namespace Qt::StringLiterals;

namespace planner {

ExchangeRequestManager::ExchangeRequestManager(QRestAccessManager& manager,
                                               QWebEngineView& web_view,
                                               MainWindow& parent)
    : QObject{&parent}
    , manager{&manager}
    , web_view{&web_view}
{}

QNetworkReply* ExchangeRequestManager::getLeagues(Game game)
{
    QNetworkRequest request;
    if (game == Game::Poe1)
        request.setUrl(QUrl::fromEncoded("https://poe.ninja/poe1/api/data/index-state"));
    else
        request.setUrl(QUrl::fromEncoded("https://poe.ninja/poe2/api/data/index-state"));

    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent());

    request.setRawHeader("accept", "*/*");
    request.setRawHeader("priority", "u=4, i");
    request.setRawHeader("sec-fetch-dest", "empty");
    request.setRawHeader("sec-fetch-mode", "no-cors");
    request.setRawHeader("sec-fetch-site", "none");

    return manager->get(request);
}

QNetworkReply* ExchangeRequestManager::getOverview(Game game, const QString& type)
{
    QUrl url{game == Game::Poe1 ? "https://poe.ninja/poe1/api/economy/exchange/current/overview"
                                : "https://poe.ninja/poe2/api/economy/exchange/current/overview"};

    QUrlQuery query{};
    query.addQueryItem(u"league"_s, Settings::currentLeague(game));
    query.addQueryItem(u"type"_s, type);
    url.setQuery(query);

    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent());

    request.setRawHeader("accept", "*/*");
    request.setRawHeader("priority", "u=4, i");
    request.setRawHeader("sec-fetch-dest", "empty");
    request.setRawHeader("sec-fetch-mode", "no-cors");
    request.setRawHeader("sec-fetch-site", "none");

    return manager->get(request);
}

bool ExchangeRequestManager::parseLeagues(const QJsonObject& obj,
                                          QStringList& names,
                                          QStringList& urls)
{
    const auto leagues_a = obj["economyLeagues"].toArray();
    if (leagues_a.empty())
        return false;

    for (auto league_v : leagues_a) {
        auto league_o = league_v.toObject();
        auto name = league_o["name"].toString();
        auto url = league_o["url"].toString();
        if (name.isEmpty() || url.isEmpty())
            return false;
        names.push_back(std::move(name));
        urls.push_back(std::move(url));
    }

    return true;
}

bool ExchangeRequestManager::parseOverviewItems(const QJsonObject& overview,
                                                const QString& type,
                                                ExchangeRequestCache& cache)
{
    const auto items_a = overview["items"].toArray();
    auto& currency_cache = cache.cache;
    for (auto item_v : items_a) {
        auto item_o{item_v.toObject()};
        auto id = item_o["id"].toString();
        auto name = item_o["name"].toString();
        auto details_id = item_o["detailsId"].toString();
        if (id.isEmpty() || name.isEmpty() || details_id.isEmpty())
            return false;

        auto res = currency_cache.try_emplace(id);
        auto& data = res.first->second;
        data.name = name;
        data.details_id = details_id;
        data.type = type;
        data.is_changed = true;
        cache.cache_changed = true;

        auto link_path = item_o["image"].toString();
        if (!link_path.isEmpty()) {
            QString file_path = cache.iconFileName(id);
            if (!QFile::exists(file_path)) {
                downloadIcon(link_path, file_path);
                data.icon = QIcon{file_path};
            }
        }
    }

    return true;
}

bool ExchangeRequestManager::parseOverviewCosts(const QJsonObject& overview,
                                                ExchangeRequestCache& cache)
{
    const auto lines_a = overview["lines"].toArray();

    auto time = QDateTime::currentDateTimeUtc();
    auto game = cache.game;
    auto& cost_cache = cache.cost_cache[Settings::currentLeague(game)];
    for (auto line_v : lines_a) {
        auto line_o{line_v.toObject()};
        auto id = line_o["id"].toString();
        auto popular_rate = line_o["maxVolumeRate"].toDouble();
        auto popular_id = line_o["maxVolumeCurrency"].toString();
        if (id.isEmpty() || popular_rate == 0.0 || popular_id.isEmpty())
            return false;

        cache.shareCurrencyId(id);
        auto res = cost_cache.costs.try_emplace(id);
        auto& data = res.first->second;
        data.updated = time;
        data.popular.value = 1.0 / popular_rate;
        data.popular.currency = {popular_id};
        cache.prepareCurrency(data.popular.currency);
    }
    cost_cache.is_changed = true;

    return true;
}

bool ExchangeRequestManager::parseCore(const QJsonObject& core, ExchangeRequestCache& cache)
{
    auto game = cache.game;
    QStringList ids;
    const auto items_a = core["items"].toArray();
    if (items_a.isEmpty())
        return true;

    for (auto item_v : items_a) {
        auto item_o{item_v.toObject()};
        auto id = item_o["id"].toString();
        if (id.isEmpty())
            return false;
        ids.push_back(id);
    }

    auto rates_o = core["rates"].toObject();
    std::vector<std::pair<double, QString>> rates;
    QString primary_id;
    for (auto& id : std::as_const(ids)) {
        if (rates_o.contains(id))
            rates.emplace_back(rates_o[id].toDouble(), id);
        else
            primary_id = id;
    }
    if (primary_id.isEmpty())
        return false;

    auto& cost_cache = cache.cost_cache[Settings::currentLeague(game)];
    auto& core_currencies = cost_cache.core_currencies;
    core_currencies.clear();
    for (auto& [rate, id] : rates) {
        auto& core_currency = core_currencies.emplace_back();
        core_currency.ratio_to_primary = rate;
        core_currency.currency.id = id;
        cache.prepareCurrency(core_currency.currency);
    }
    auto& primary = core_currencies.emplace_back();
    primary.ratio_to_primary = 1.0;
    primary.currency.id = primary_id;
    cache.prepareCurrency(primary.currency);

    cost_cache.is_changed = true;

    return true;
}

void ExchangeRequestManager::downloadIcon(const QString& link_path, const QString& file_path)
{
    QNetworkRequest request;
    request.setUrl("https://web.poecdn.com" + link_path);

    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent());

    request.setRawHeader("accept",
                         "image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8");
    request.setRawHeader("priority", "u=1, i");
    request.setRawHeader("sec-fetch-dest", "image");
    request.setRawHeader("sec-fetch-mode", "no-cors");
    request.setRawHeader("sec-fetch-site", "cross-site");
    request.setRawHeader("sec-fetch-storage-access", "active");

    auto reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [file_path, reply] {
        if (reply->error() != QNetworkReply::NoError)
            return;

        QFile file{file_path};
        if (file.open(QFile::NewOnly))
            file.write(reply->readAll());
    });
}

QString ExchangeRequestManager::userAgent()
{
    return web_view->page()->profile()->httpUserAgent();
}

} // namespace planner
