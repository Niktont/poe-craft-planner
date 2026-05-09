#include "DataUpdater.h"
#include <array>
#include <set>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QRestAccessManager>
#include <QRestReply>
#include <QString>
#include <QTimer>

using namespace Qt::StringLiterals;

namespace planner {
struct Tables
{
    std::array<QStringView, 2> arr;
    constexpr QStringView forGame(DataUpdater::Game game) const
    {
        return arr[static_cast<size_t>(game)];
    }
};

static constexpr Tables currency_data{u"currency_data_poe1", u"currency_data_poe2"};
static constexpr Tables filters{u"filters_poe1", u"filters_poe2"};
static constexpr Tables filter_options{u"filter_options_poe1", u"filter_options_poe2"};
static constexpr Tables stats{u"stats_poe1", u"stats_poe2"};
static constexpr Tables stat_types{u"stat_types_poe1", u"stat_types_poe1"};

static std::array languages{
    u"en"_sv,
    u"br"_sv,
    u"ru"_sv,
    u"th"_sv,
    u"de"_sv,
    u"fr"_sv,
    u"es"_sv,
    u"jp"_sv,
};
static std::array domains{
    u"www.pathofexile.com"_s,
    u"br.pathofexile.com"_s,
    u"ru.pathofexile.com"_s,
    u"th.pathofexile.com"_s,
    u"de.pathofexile.com"_s,
    u"fr.pathofexile.com"_s,
    u"es.pathofexile.com"_s,
    u"jp.pathofexile.com"_s,
};

static std::set ignored_types_poe1{
    u"Sanctum"_s,
    u"Heist"_s,
    u"Incubators"_s,
    u"Beasts"_s,
    u"MapKey"_s,
    u"MapsSpecial"_s,
    u"MapsUnique"_s,
    u"Legacy"_s,
    u"Misc"_s,
};
static std::set ignored_types_poe2{
    u"Waystones"_s,
    u"Misc"_s,
};

static std::set ignored_ids_poe1{
    u"provisioning-wombgift"_s,
    u"lavish-wombgift"_s,
    u"ancient-wombgift"_s,
    u"mysterious-wombgift"_s,
    u"sep"_s,
};
static std::set ignored_ids_poe2{
    u"sep"_s,
};

static std::array data_types{
    u"static"_s,
    u"filters"_s,
    u"stats"_s,
};

DataUpdater::DataUpdater(QObject* parent)
    : QObject{parent}
    , network_manager{new QNetworkAccessManager{this}}
    , rest_manager{new QRestAccessManager{network_manager, this}}
{
    network_manager->setAutoDeleteReplies(true);
    initAddConnection();
}

bool DataUpdater::initAddConnection()
{
    auto db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(DB_PATH);
    return db.open();
}

QSqlQuery DataUpdater::insertText(QStringView table, QStringView lang)
{
    QSqlQuery query;
    query.prepare("INSERT INTO " % table % "(id, " % lang
                  % ") VALUES (?, ?) ON CONFLICT (id) DO UPDATE "
                    "SET "
                  % lang % " = excluded." % lang % ";");
    return query;
}

QSqlQuery DataUpdater::insertName(Game game, QStringView lang)
{
    return insertText(currency_data.forGame(game), lang);
}

QSqlQuery DataUpdater::insertImageAndName(Game game, QStringView lang)
{
    QSqlQuery query;
    query.prepare("INSERT INTO " % currency_data.forGame(game) % "(id, image, " % lang
                  % ") VALUES (?, ?, ?) ON CONFLICT (id) DO UPDATE "
                    "SET image = excluded.image, "
                  % lang % " = excluded." % lang % ";");
    return query;
}

QSqlQuery DataUpdater::insertFilter(Game game, QStringView lang)
{
    return insertText(filters.forGame(game), lang);
}

QSqlQuery DataUpdater::insertFilterOption(Game game, QStringView lang)
{
    return insertText(filter_options.forGame(game), lang);
}

QSqlQuery DataUpdater::insertStat(Game game, QStringView lang)
{
    return insertText(stats.forGame(game), lang);
}

QSqlQuery DataUpdater::insertStatType(Game game, QStringView lang)
{
    return insertText(stat_types.forGame(game), lang);
}

void DataUpdater::nextLang()
{
    ++lang;
    if (lang < languages.size())
        QTimer::singleShot(10000, this, &DataUpdater::getData);
    else {
        qInfo() << "Update finished";
        QCoreApplication::quit();
    }
}

std::optional<QJsonDocument> DataUpdater::readJson(QRestReply& reply)
{
    if (!reply.isSuccess()) {
        qWarning() << QString{"Request failed for " % languages[lang]};
        nextLang();
        return {};
    }
    auto json = reply.readJson();
    if (!json) {
        qWarning() << QString{"No json for " % languages[lang]};
        nextLang();
    }
    return json;
}

const QString DataUpdater::user_agent{
    u"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
    " (KHTML, like Gecko) PoeCraftPlanner/"_s
    % APP_VERSION % u" Chrome/134.0.0.0 Safari/537.36"_s};

void DataUpdater::getData()
{
    QNetworkRequest request;

    QUrl url;
    if (game == Poe1)
        url.setUrl(u"https://"_s % domains[lang] % u"/api/trade/data/"_s % data_types[data_type]);
    else
        url.setUrl(u"https://"_s % domains[lang] % u"/api/trade2/data/"_s % data_types[data_type]);
    request.setUrl(url);

    request.setHeader(QNetworkRequest::UserAgentHeader, user_agent);

    request.setRawHeader("accept", "*/*");
    request.setRawHeader("priority", "u=4, i");
    request.setRawHeader("sec-fetch-dest", "empty");
    request.setRawHeader("sec-fetch-mode", "no-cors");
    request.setRawHeader("sec-fetch-site", "none");

    qInfo() << QString{"Sending request for " % languages[lang]};
    switch (data_type) {
    case Static:
        rest_manager->get(request, this, &DataUpdater::parseStaticData);
        break;
    case Filters:
        rest_manager->get(request, this, &DataUpdater::parseFiltersData);
        break;
    case Stats:
        rest_manager->get(request, this, &DataUpdater::parseStatsData);
        break;
    }
}

void DataUpdater::parseStaticData(QRestReply& reply)
{
    auto json = readJson(reply);
    if (!json)
        return;

    auto db = QSqlDatabase::database();
    db.transaction();
    bool insert_image = lang == 0;
    auto query = insert_image ? insertImageAndName(game, languages[lang])
                              : insertName(game, languages[lang]);

    auto& ignored_types = game == Poe1 ? ignored_types_poe1 : ignored_types_poe2;
    auto& ignored_ids = game == Poe1 ? ignored_ids_poe1 : ignored_ids_poe2;

    const auto json_o = json->object();
    const auto result_a = json_o["result"].toArray();
    for (auto& type_v : result_a) {
        const auto type_o = type_v.toObject();

        auto type_id = type_o["id"].toString();
        if (ignored_types.contains(type_id))
            continue;

        const auto entries_a = type_o["entries"].toArray();
        for (auto& entrie_v : entries_a) {
            const auto entrie_o = entrie_v.toObject();
            auto id = entrie_o["id"].toString();
            if (id.isEmpty() || ignored_ids.contains(id))
                continue;
            auto text = entrie_o["text"].toString();

            query.addBindValue(id);
            if (insert_image) {
                auto image = entrie_o["image"].toString();
                query.addBindValue(image);
            }
            query.addBindValue(text);
            if (!query.exec())
                qWarning() << "Insert query failed";
        }
    }
    db.commit();

    nextLang();
}

void DataUpdater::parseFiltersData(QRestReply& reply)
{
    auto json = readJson(reply);
    if (!json)
        return;

    auto db = QSqlDatabase::database();
    db.transaction();
    auto query_filter = insertFilter(game, languages[lang]);
    auto query_option = insertFilterOption(game, languages[lang]);

    const auto json_o = json->object();
    const auto result_a = json_o["result"].toArray();
    for (auto& group_v : result_a) {
        const auto group_o = group_v.toObject();

        const auto filters_a = group_o["filters"].toArray();
        for (auto& filter_v : filters_a) {
            const auto filter_o = filter_v.toObject();
            auto filter_id = filter_o["id"].toString();
            if (filter_id.isEmpty())
                continue;

            auto text = filter_o["text"].toString();
            if (text.isEmpty())
                continue;

            query_filter.addBindValue(filter_id);
            query_filter.addBindValue(text);
            if (!query_filter.exec()) {
                qWarning() << "Insert query failed";
                continue;
            }

            const auto option_v = filter_o["option"];
            if (option_v.isUndefined())
                continue;

            const auto option_o = option_v.toObject();
            const auto options_a = option_o["options"].toArray();
            for (auto option_v : options_a) {
                auto option_o = option_v.toObject();

                auto id = option_o["id"].toString();
                if (id.isEmpty())
                    continue;

                auto text = option_o["text"].toString();
                if (text.isEmpty())
                    continue;

                query_option.addBindValue(id);
                query_option.addBindValue(text);
                if (!query_option.exec())
                    qWarning() << "Insert query failed";
            }
        }
    }
    db.commit();

    nextLang();
}

void DataUpdater::parseStatsData(QRestReply& reply)
{
    auto json = readJson(reply);
    if (!json)
        return;

    auto db = QSqlDatabase::database();
    db.transaction();
    auto query_type = insertStatType(game, languages[lang]);
    auto query_stat = insertStat(game, languages[lang]);

    const auto json_o = json->object();
    const auto result_a = json_o["result"].toArray();
    for (auto& type_v : result_a) {
        const auto type_o = type_v.toObject();

        auto type_id = type_o["id"].toString();
        if (type_id.isEmpty())
            continue;

        auto text = type_o["label"].toString();
        if (text.isEmpty())
            continue;

        query_type.addBindValue(type_id);
        query_type.addBindValue(text);
        if (!query_type.exec()) {
            qWarning() << "Insert query failed";
            continue;
        }

        const auto entries_a = type_o["entries"].toArray();
        for (auto entries_v : entries_a) {
            auto entrie_o = entries_v.toObject();

            auto id = entrie_o["id"].toString();
            auto parts = id.split(u'.');
            if (parts.size() != 2)
                continue;

            id = parts.back();
            if (id.isEmpty())
                continue;

            auto text = entrie_o["text"].toString();
            if (text.isEmpty())
                continue;

            query_stat.addBindValue(id);
            query_stat.addBindValue(text);
            if (!query_stat.exec())
                qWarning() << "Insert query failed";
        }
    }
    db.commit();

    nextLang();
}

} // namespace planner
