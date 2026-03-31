#ifndef TRADEREQUESTCACHE_H
#define TRADEREQUESTCACHE_H

#include "Game.h"
#include "ItemTime.h"
#include "TradeCostData.h"
#include "TradeRequestData.h"
#include "TradeRequestKey.h"
#include <boost/container/flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <QAbstractTableModel>
#include <QJsonArray>

class QCompleter;
class QSortFilterProxyModel;

namespace planner {
enum class TradeRequestColumn {
    Name,
    Link,
    Query,
    Regex,
    Description,
    Time,

    last = Time,
};

class TradeItemData;
class ExchangeRequestCache;
class ExchangeData;

class TradeRequestCache : public QAbstractTableModel
{
    Q_OBJECT
public:
    TradeRequestCache(Game game, ExchangeRequestCache& exchange_cache, QObject* parent = nullptr);
    ~TradeRequestCache() { saveCostCache(); }

    bool readDatabase();

    using Cache = boost::container::flat_map<TradeRequestKey, TradeRequestData>;
    Cache cache;

    using CostCache = boost::container::flat_map<QString, TradeCostData>;
    CostCache cost_cache;

    QCompleter* completer;
    QSortFilterProxyModel* proxy_model;
    const Game game;

    Cache::iterator requestData(const TradeRequestKey& request) { return cache.find(request); };
    Cache::const_iterator requestData(const TradeRequestKey& request) const
    {
        return const_cast<TradeRequestCache*>(this)->requestData(request);
    }
    CostCache::iterator currentLeagueData();
    CostCache::const_iterator currentLeagueData() const
    {
        return const_cast<TradeRequestCache*>(this)->currentLeagueData();
    };

    void saveRequest(const TradeRequestKey& request,
                     const QString& name,
                     const QJsonDocument& query,
                     const QString& regex,
                     const RequestDescription& description);

    void deleteRequests(int top, int bottom);
    void deleteRequest(const TradeRequestKey& request) { deleteRequest(requestData(request)); }
    void deleteRequest(Cache::const_iterator it);

    void updateCost(const TradeRequestKey& request, TradeCostData::Data cost_data);
    void setDefaultTime(const TradeRequestKey& request, std::optional<ItemTime> time);

    bool saveCostCache() const;

    bool isOutdated(const TradeRequestKey& request, QDateTime now) const;

    QString name(const TradeItemData& trade_item) const;
    double costValue(const TradeRequestKey& request) const;
    const TradeCostData::Data* costData(const TradeRequestKey& request) const;
    const ExchangeData* costCurrency(const TradeRequestKey& request) const;
    double goldFee(const TradeRequestKey& request) const;
    ItemTime time(const TradeItemData& trade_item) const;

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;

    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    boost::unordered::unordered_flat_set<TradeRequestKey> export_requests;
    QJsonArray exportRequests();

    static Cache requestsFromJson(const QJsonArray& requests_a);
    void mergeImportRequests(Cache&& import_requests);

signals:
    void defaultTimeChanged(const planner::TradeRequestKey& request);

private:
    ExchangeRequestCache* exchange_cache;
};

} // namespace planner

#endif // TRADEREQUESTCACHE_H
