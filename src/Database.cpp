#include "Database.h"

#include "ExchangeRequestCache.h"
#include <array>
#include <QJsonDocument>

using namespace Qt::StringLiterals;

namespace planner {
struct Tables
{
    std::array<QStringView, 2> arr;
    constexpr QStringView forGame(Game game) const { return arr[static_cast<size_t>(game)]; }
};

// type TEXT, url TEXT
static constexpr Tables currency_types{u"currency_types_poe1", u"currency_types_poe2"};
// id TEXT, fee REAL
static constexpr Tables currency_data{u"currency_data_poe1", u"currency_data_poe2"};

static constexpr Tables plans{u"plans_poe1", u"plans_poe2"};

static constexpr Tables trade_cache{u"trade_cache_poe1", u"trade_cache_poe2"};
static constexpr Tables trade_cost_cache{u"trade_cost_cache_poe1", u"trade_cost_cache_poe2"};

static constexpr Tables exchange_cache{u"exchange_cache_poe1", u"exchange_cache_poe2"};
static constexpr Tables exchange_cost_cache{u"exchange_cost_cache_poe1",
                                            u"exchange_cost_cache_poe2"};

static const QString additional_db{u"additional"_s};

const QString Database::db_version_key{u"db_version"_s};
const int Database::db_version{1};

bool Database::initConnection()
{
    auto db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("main.sqlite3");
    return db.open();
}

bool Database::initAddConnection()
{
    auto db = QSqlDatabase::addDatabase("QSQLITE", additional_db);
    db.setDatabaseName("additional.sqlite3");
    return db.open();
}

QSqlQuery Database::selectCurrencyType(Game game)
{
    QSqlQuery query{QSqlDatabase::database(additional_db)};
    query.prepare("SELECT type, url FROM " % currency_types.forGame(game) % ";");
    return query;
}

QSqlQuery Database::selectAdditionalData(Game game)
{
    QSqlQuery query{QSqlDatabase::database(additional_db)};
    query.prepare("SELECT id, fee FROM " % currency_data.forGame(game) % ";");
    return query;
}

bool Database::createInfoTable()
{
    QSqlQuery query;
    return query.exec(
        "CREATE TABLE IF NOT EXISTS info (key TEXT PRIMARY KEY NOT NULL, value TEXT);");
}

QVariant Database::selectInfo(const QString& key)
{
    QSqlQuery query;
    query.prepare("SELECT value FROM info WHERE key = ?;");
    query.addBindValue(key);
    if (!query.exec() || !query.next())
        return {};
    return query.value(0);
}

bool Database::updateInfo(const QString& key, const QVariant& value)
{
    QSqlQuery query;
    query.prepare("INSERT INTO info"
                  "(key, value) VALUES (?, ?) ON CONFLICT (key) DO UPDATE "
                  "SET value = excluded.value;");
    query.addBindValue(key);
    query.addBindValue(value);
    return query.exec();
}

bool Database::createPlansTable(Game game)
{
    QSqlQuery query;
    return query.exec("CREATE TABLE IF NOT EXISTS " % plans.forGame(game)
                      % "(id TEXT PRIMARY KEY NOT NULL, name TEXT, is_folder INTEGER, item TEXT);");
}

QSqlQuery Database::selectPlan(Game game)
{
    QSqlQuery query;
    query.prepare("SELECT id, name, is_folder, item FROM " % plans.forGame(game) % " WHERE id = ?;");

    return query;
}

QSqlQuery Database::renamePlan(Game game)
{
    QSqlQuery query;
    query.prepare("UPDATE " % plans.forGame(game) % " SET name = ? WHERE id = ?;");

    return query;
}

QSqlQuery Database::savePlan(Game game)
{
    QSqlQuery query;
    query.prepare("INSERT INTO " % plans.forGame(game)
                  % "(id, name, is_folder, item) VALUES (?, ?, ?, ?) ON CONFLICT (id) DO UPDATE "
                    "SET item = excluded.item;");

    return query;
}

QSqlQuery Database::deletePlan(Game game)
{
    QSqlQuery query;
    query.prepare("DELETE FROM " % plans.forGame(game) % " WHERE id = ?;");
    return query;
}

bool Database::createExchangeCacheTable(Game game)
{
    QSqlQuery query;
    return query.exec("CREATE TABLE IF NOT EXISTS " % exchange_cache.forGame(game)
                      % "(id TEXT PRIMARY KEY NOT NULL, name TEXT, details_id TEXT, type TEXT,"
                        " default_time REAL);");
}

bool Database::createExchangeCostCacheTable(Game game)
{
    QSqlQuery query;
    return query.exec("CREATE TABLE IF NOT EXISTS " % exchange_cost_cache.forGame(game)
                      % "(league TEXT PRIMARY KEY NOT NULL, cost_cache TEXT);");
}

QSqlQuery planner::Database::selectExchangeCache(Game game)
{
    QSqlQuery query;
    query.prepare("SELECT id, name, details_id, type, default_time FROM "
                  % exchange_cache.forGame(game) % " ORDER BY id;");

    return query;
}

QSqlQuery planner::Database::selectExchangeCostCache(Game game)
{
    QSqlQuery query;
    query.prepare("SELECT league, cost_cache FROM " % exchange_cost_cache.forGame(game)
                  % " ORDER BY league;");

    return query;
}

QSqlQuery planner::Database::insertExchangeCache(Game game)
{
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO " % exchange_cache.forGame(game)
                  % "(id, name, details_id, type, default_time) VALUES (?, ?, ?, ?, ?);");

    return query;
}

QSqlQuery planner::Database::insertExchangeCostCache(Game game)
{
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO " % exchange_cost_cache.forGame(game)
                  % "(league, cost_cache) VALUES (?, ?);");

    return query;
}

QSqlQuery Database::deleteExchangeCache(Game game)
{
    QSqlQuery query;
    query.prepare("DELETE FROM " % exchange_cache.forGame(game) % " WHERE id = ?;");
    return query;
}

bool Database::deleteExchangeCostCache(Game game, const QString& league)
{
    QSqlQuery query;
    query.prepare("DELETE FROM " % exchange_cost_cache.forGame(game) % " WHERE league = ?;");
    query.addBindValue(league);
    return query.exec();
}

bool Database::clearExchangeCache(Game game)
{
    QSqlQuery query;
    query.prepare("DELETE FROM " % exchange_cache.forGame(game) % ";");
    return query.exec();
}

bool Database::clearExchangeCostCache(Game game)
{
    QSqlQuery query;
    query.prepare("DELETE FROM " % exchange_cost_cache.forGame(game) % ";");

    return query.exec();
}

std::pair<QString, ExchangeData> planner::Database::exchangeCacheFromQuery(const QSqlQuery& query,
                                                                           Game game)
{
    std::pair<QString, ExchangeData> result;
    int i = -1;
    result.first = query.value(++i).toString();

    result.second.name = query.value(++i).toString();
    result.second.details_id = query.value(++i).toString();
    result.second.type = query.value(++i).toString();
    auto default_time = query.value(++i);
    if (!default_time.isNull())
        result.second.default_time = ItemTime(default_time.toDouble());
    if (!result.first.isEmpty())
        result.second.icon = QIcon{ExchangeRequestCache::iconFileName(game, result.first)};

    result.second.is_changed = false;

    return result;
}

std::pair<QString, ExchangeCostData> Database::exchangeCostCacheFromQuery(
    const QSqlQuery& query, const ExchangeRequestCache& cache)
{
    std::pair<QString, ExchangeCostData> result;
    int i = -1;
    result.first = query.value(++i).toString();

    auto json{QJsonDocument::fromJson(query.value(++i).toByteArray())};
    result.second = ExchangeCostData::fromJson(json.object(), cache);

    return result;
}

bool planner::Database::insertExchangeCache(QSqlQuery& query,
                                            const QString& id,
                                            const ExchangeData& data)
{
    query.addBindValue(id);
    query.addBindValue(data.name);
    query.addBindValue(data.details_id);
    query.addBindValue(data.type);
    if (data.default_time)
        query.addBindValue(data.default_time->count());
    else
        query.addBindValue(QVariant{QMetaType::fromType<double>()});
    return query.exec();
}

bool Database::insertExchangeCostCache(QSqlQuery& query,
                                       const QString& league,
                                       const ExchangeCostData& data)
{
    query.addBindValue(league);

    QJsonDocument doc{data.toJson()};
    query.addBindValue(doc.toJson(QJsonDocument::Compact));

    return query.exec();
}

bool Database::createTradeCacheTable(Game game)
{
    QSqlQuery query;
    return query.exec(
        "CREATE TABLE IF NOT EXISTS " % trade_cache.forGame(game)
        % "(id TEXT NOT NULL, domain INTEGER, name TEXT, query TEXT, regex TEXT, description "
          "TEXT, default_time REAL, PRIMARY KEY (id, domain));");
}

bool Database::createTradeCostCacheTable(Game game)
{
    QSqlQuery query;
    return query.exec("CREATE TABLE IF NOT EXISTS " % trade_cost_cache.forGame(game)
                      % "(league TEXT PRIMARY KEY NOT NULL, cost_cache TEXT);");
}

QSqlQuery Database::selectTradeCache(Game game)
{
    QSqlQuery query;
    query.prepare("SELECT id, domain, name, query, regex, description, default_time FROM "
                  % trade_cache.forGame(game) % ";");

    return query;
}

QSqlQuery Database::selectTradeCostCache(Game game)
{
    QSqlQuery query;
    query.prepare("SELECT league, cost_cache FROM " % trade_cost_cache.forGame(game)
                  % " ORDER BY league;");

    return query;
}

QSqlQuery Database::insertTradeCache(Game game)
{
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO " % trade_cache.forGame(game)
                  % "(id, domain, name, query, regex, description, default_time) VALUES (?, ?, ?, "
                    "?, ?, ?, ?);");

    return query;
}

QSqlQuery planner::Database::insertTradeCostCache(Game game)
{
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO " % trade_cost_cache.forGame(game)
                  % "(league, cost_cache) VALUES (?, ?);");

    return query;
}

bool Database::deleteTradeCache(Game game, const TradeRequestKey& request)
{
    QSqlQuery query;
    query.prepare("DELETE FROM " % trade_cache.forGame(game) % " WHERE id = ? AND domain = ?;");
    query.addBindValue(request.request_id);
    query.addBindValue(static_cast<std::underlying_type_t<Domain>>((request.domain)));
    return query.exec();
}

bool Database::deleteTradeCostCache(Game game, const QString& league)
{
    QSqlQuery query;
    query.prepare("DELETE FROM " % trade_cost_cache.forGame(game) % " WHERE league = ?;");
    query.addBindValue(league);
    return query.exec();
}

bool Database::clearTradeCache(Game game)
{
    QSqlQuery query;
    query.prepare("DELETE FROM " % trade_cache.forGame(game) % ";");
    return query.exec();
}

bool Database::clearTradeCostCache(Game game)
{
    QSqlQuery query;
    query.prepare("DELETE FROM " % trade_cost_cache.forGame(game) % ";");
    return query.exec();
}

std::pair<TradeRequestKey, TradeRequestData> Database::tradeCacheFromQuery(const QSqlQuery& query)
{
    std::pair<TradeRequestKey, TradeRequestData> result;
    int i = -1;
    result.first.request_id = query.value(++i).toString();
    result.first.domain = static_cast<Domain>(query.value(++i).toInt());

    result.second.name_ = query.value(++i).toString();
    result.second.query_ = QJsonDocument::fromJson(query.value(++i).toByteArray());
    result.second.regex_ = query.value(++i).toString();
    result.second.description_ = query.value(++i).toString();
    auto default_time = query.value(++i);
    if (!default_time.isNull())
        result.second.default_time = ItemTime(default_time.toDouble());

    result.second.is_changed = false;

    return result;
}

std::pair<QString, TradeCostData> Database::tradeCostCacheFromQuery(
    const QSqlQuery& query, const ExchangeRequestCache& cache)
{
    std::pair<QString, TradeCostData> result;
    int i = -1;
    result.first = query.value(++i).toString();

    auto json{QJsonDocument::fromJson(query.value(++i).toByteArray())};
    result.second = TradeCostData::fromJson(json.object(), cache);

    return result;
}

bool Database::insertTradeCache(QSqlQuery& query,
                                const TradeRequestKey& key,
                                const TradeRequestData& data)
{
    query.addBindValue(key.request_id);
    query.addBindValue(static_cast<std::underlying_type_t<Domain>>(key.domain));
    query.addBindValue(data.name_);
    query.addBindValue(data.query_.toJson(QJsonDocument::Compact));
    query.addBindValue(data.regex_);
    query.addBindValue(data.description_);
    if (data.default_time)
        query.addBindValue(data.default_time->count());
    else
        query.addBindValue(QVariant{QMetaType::fromType<double>()});

    return query.exec();
}

bool Database::insertTradeCostCache(QSqlQuery& query,
                                    const QString& league,
                                    const TradeCostData& data)
{
    query.addBindValue(league);

    QJsonDocument doc{data.toJson()};
    query.addBindValue(doc.toJson(QJsonDocument::Compact));

    return query.exec();
}

} // namespace planner
