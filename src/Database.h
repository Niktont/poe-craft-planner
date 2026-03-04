#ifndef DATABASE2_H
#define DATABASE2_H

#include "ExchangeCostData.h"
#include "ExchangeData.h"
#include "TradeCostData.h"
#include "TradeRequestData.h"
#include "TradeRequestKey.h"
#include "Game.h"
#include <QSqlDatabase>
#include <QSqlQuery>

namespace planner {

class ExchangeRequestCache;

class Database
{
public:
    static bool initConnection();
    static bool initAddConnection();

    static QSqlQuery selectCurrencyType(Game game);
    static QSqlQuery selectAdditionalData(Game game);

    static bool createInfoTable();
    static QVariant selectInfo(const QString& key);
    static bool updateInfo(const QString& key, const QVariant& value);
    static const QString db_version_key;
    static const int db_version;

    static bool createPlansTable(Game game);
    static QSqlQuery selectPlan(Game game);

    static QSqlQuery renamePlan(Game game);
    static QSqlQuery savePlan(Game game);
    static QSqlQuery deletePlan(Game game);

    static bool createExchangeCacheTable(Game game);
    static bool createExchangeCostCacheTable(Game game);

    static QSqlQuery selectExchangeCache(Game game);
    static QSqlQuery selectExchangeCostCache(Game game);

    static QSqlQuery insertExchangeCache(Game game);
    static QSqlQuery insertExchangeCostCache(Game game);

    static QSqlQuery deleteExchangeCache(Game game);
    static bool deleteExchangeCostCache(Game game, const QString& league);

    static bool clearExchangeCache(Game game);
    static bool clearExchangeCostCache(Game game);

    static std::pair<QString, ExchangeData> exchangeCacheFromQuery(const QSqlQuery& query,
                                                                   Game game);
    static std::pair<QString, ExchangeCostData> exchangeCostCacheFromQuery(
        const QSqlQuery& query, const ExchangeRequestCache& cache);

    static bool insertExchangeCache(QSqlQuery& query, const QString& id, const ExchangeData& data);
    static bool insertExchangeCostCache(QSqlQuery& query,
                                        const QString& league,
                                        const ExchangeCostData& data);

    static bool createTradeCacheTable(Game game);
    static bool createTradeCostCacheTable(Game game);

    static QSqlQuery selectTradeCache(Game game);
    static QSqlQuery selectTradeCostCache(Game game);

    static QSqlQuery insertTradeCache(Game game);
    static QSqlQuery insertTradeCostCache(Game game);

    static bool deleteTradeCostCache(Game game, const QString& league);

    static bool clearTradeCache(Game game);
    static bool clearTradeCostCache(Game game);

    static std::pair<TradeRequestKey, TradeRequestData> tradeCacheFromQuery(const QSqlQuery& query);
    static std::pair<QString, TradeCostData> tradeCostCacheFromQuery(
        const QSqlQuery& query, const ExchangeRequestCache& cache);

    static bool insertTradeCache(QSqlQuery& query,
                                 const TradeRequestKey& key,
                                 const TradeRequestData& data);
    static bool insertTradeCostCache(QSqlQuery& query,
                                     const QString& league,
                                     const TradeCostData& data);
};

} // namespace planner

#endif // DATABASE2_H
