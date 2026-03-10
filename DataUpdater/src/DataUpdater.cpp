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
static constexpr QStringView currency_data_poe1{u"currency_data_poe1"};
static constexpr QStringView currency_data_poe2{u"currency_data_poe2"};

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

QSqlQuery DataUpdater::insertName(Game game, QStringView lang)
{
    QSqlQuery query;
    auto table = game == Poe1 ? currency_data_poe1 : currency_data_poe2;
    query.prepare("INSERT INTO " % table % "(id, " % lang
                  % ") VALUES (?, ?) ON CONFLICT (id) DO UPDATE "
                    "SET "
                  % lang % " = excluded." % lang % ";");
    return query;
}

QSqlQuery DataUpdater::insertImageAndName(Game game, QStringView lang)
{
    QSqlQuery query;
    auto table = game == Poe1 ? currency_data_poe1 : currency_data_poe2;
    query.prepare("INSERT INTO " % table % "(id, image, " % lang
                  % ") VALUES (?, ?, ?) ON CONFLICT (id) DO UPDATE "
                    "SET image = excluded.image, "
                  % lang % " = excluded." % lang % ";");
    return query;
}

void DataUpdater::getData()
{
    QNetworkRequest request;

    QUrl url;
    if (game == Poe1)
        url.setUrl(u"https://"_s % domains[lang] % u"/api/trade/data/static"_s);
    else
        url.setUrl(u"https://"_s % domains[lang] % u"/api/trade2/data/static"_s);
    request.setUrl(url);

    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QString{u"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
                              " (KHTML, like Gecko) PoeCraftPlanner/"_s
                              % APP_VERSION % u" Chrome/134.0.0.0 Safari/537.36"_s});

    request.setRawHeader("accept", "*/*");
    request.setRawHeader("priority", "u=4, i");
    request.setRawHeader("sec-fetch-dest", "empty");
    request.setRawHeader("sec-fetch-mode", "no-cors");
    request.setRawHeader("sec-fetch-site", "none");

    qInfo() << QString{"Sending request for " % languages[lang]};
    rest_manager->get(request, this, &DataUpdater::parseData);
}

void DataUpdater::parseData(QRestReply& reply)
{
    if (reply.isSuccess()) {
        if (auto json = reply.readJson()) {
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
        } else
            qWarning() << QString{"No json for " % languages[lang]};
    } else
        qWarning() << QString{"Request failed for " % languages[lang]};

    ++lang;
    if (lang < languages.size())
        QTimer::singleShot(10000, this, &DataUpdater::getData);
    else {
        qInfo() << "Update finished";
        QCoreApplication::quit();
    }
}

} // namespace planner
