#include "TradeRequestKey.h"
#include <algorithm>
#include <array>
#include <QStringBuilder>
#include <QUrl>

#include "Settings.h"

using namespace Qt::Literals;

namespace planner {
static const std::array domains{
    u"www.pathofexile.com"_s,
    u"br.pathofexile.com"_s,
    u"ru.pathofexile.com"_s,
    u"th.pathofexile.com"_s,
    u"de.pathofexile.com"_s,
    u"fr.pathofexile.com"_s,
    u"es.pathofexile.com"_s,
    u"jp.pathofexile.com"_s,
    u"poe.game.daum.net"_s,
};

static const QStringView main_without_www{u"pathofexile.com"};

static const std::array gamesUrl{
    u"/trade"_s,
    u"/trade2"_s,
};

TradeRequestKey::Result TradeRequestKey::fromUrl(const QString& userUrl, Game game)
{
    QUrl url = QUrl::fromUserInput(userUrl);
    if (!url.isValid())
        return {ParseError::NotValidLink};

    TradeRequestKey result;

    auto host = url.host();
    if (host == main_without_www)
        result.domain = Domain::Main;
    else {
        result.domain = domainId(host);
        if (result.domain == Domain::Unknown)
            return {ParseError::NotValidLink};
    }

    auto path = url.path();
    Game game_url;
    if (path.startsWith(gamesUrl[1]))
        game_url = Game::Poe2;
    else if (path.startsWith(gamesUrl[0]))
        game_url = Game::Poe1;
    else
        return {ParseError::NotValidLink};

    result.request_id = url.fileName();
    if (result.request_id.isEmpty() || result.request_id.contains(' '))
        return {ParseError::NotValidLink};

    if (game_url != game)
        return {ParseError::GameMismatch};

    return result;
}

QString TradeRequestKey::toUrl(Game game) const
{
    if (!isValid())
        return {};

    auto settings = Settings::get();

    QString league;
    QString realm;
    if (game == Game::Poe1) {
        realm = settings.value(Settings::poe1_realm).toString();
        league = settings.value(Settings::poe1_league).toString();
    } else {
        realm = u"poe2"_s;
        league = settings.value(Settings::poe2_league).toString();
    }

    QString domain_url = domainUrl(domain);
    QString game_url = gameUrl(game);
    if (realm.isEmpty())
        return u"https://"_s % domain_url % game_url % u"/search/"_s % league % u'/' % request_id;
    else
        return u"https://"_s % domain_url % game_url % u"/search/"_s % realm % u'/' % league % u'/'
               % request_id;
}

QString TradeRequestKey::toSearchUrl(Game game) const
{
    if (!isValid())
        return {};

    auto settings = Settings::get();

    QString league;
    QString realm;
    if (game == Game::Poe1) {
        realm = settings.value(Settings::poe1_realm).toString();
        league = settings.value(Settings::poe1_league).toString();
    } else {
        realm = u"poe2"_s;
        league = settings.value(Settings::poe2_league).toString();
    }

    QString domain_url = domainUrl(domain);
    QString game_url = gameUrl(game);
    if (realm.isEmpty())
        return u"https://"_s % domain_url % u"/api"_s % game_url % u"/search/"_s % league;
    else
        return u"https://"_s % domain_url % u"/api"_s % game_url % u"/search/"_s % realm % u'/'
               % league;
}

QUrl TradeRequestKey::toFetchUrl(Game game, const std::span<QString>& items) const
{
    if (!isValid() || items.empty())
        return {};

    auto settings = Settings::get();

    QString league;
    QString realm;
    if (game == Game::Poe1) {
        realm = settings.value(Settings::poe1_realm).toString();
    } else {
        realm = u"poe2"_s;
    }

    QString domain_url = domainUrl(domain);
    QString game_url = gameUrl(game);

    QString joined_items;
    joined_items.reserve(items.size() * 65 - 1);
    joined_items.append(items.front());
    for (auto it = items.begin() + 1; it < items.end(); ++it) {
        joined_items.append(u',');
        joined_items.append(*it);
    }
    if (realm.isEmpty())
        return {u"https://"_s % domain_url % u"/api"_s % game_url % u"/fetch/"_s % joined_items
                % u"?query="_s % request_id};
    else
        return {u"https://"_s % domain_url % u"/api"_s % game_url % u"/fetch/"_s % joined_items
                % u"?query="_s % request_id % u"&realm="_s % realm};
}

QJsonObject TradeRequestKey::toJson() const
{
    QJsonObject key_o;
    key_o["request_id"] = request_id;
    key_o["domain"] = static_cast<std::underlying_type_t<Domain>>(domain);
    return key_o;
}

TradeRequestKey TradeRequestKey::fromJson(const QJsonObject& key_o)
{
    TradeRequestKey key;
    key.request_id = key_o["request_id"].toString();
    key.domain = static_cast<Domain>(key_o["domain"].toInt());
    return key;
}

Domain TradeRequestKey::domainId(const QString& domain)
{
    auto it = std::ranges::find(domains, domain);
    if (it == domains.end())
        return Domain::Unknown;

    auto pos = std::distance(domains.begin(), it);
    return static_cast<Domain>(pos);
}

QString TradeRequestKey::domainUrl(Domain domain_id)
{
    auto pos = static_cast<unsigned char>(domain_id);
    if (pos >= domains.size())
        return {};
    return domains[pos];
}

QString TradeRequestKey::gameUrl(Game game_id)
{
    auto pos = static_cast<unsigned char>(game_id);
    if (pos >= gamesUrl.size())
        return {};
    return gamesUrl[pos];
}

} // namespace planner
