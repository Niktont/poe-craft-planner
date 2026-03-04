#include "TradeRequestCache.h"
#include "Database.h"
#include "ExchangeRequestCache.h"
#include "Settings.h"
#include "TradeItemData.h"
#include "ImportException.h"
#include <QCompleter>

namespace planner {

TradeRequestCache::TradeRequestCache(Game game,
                                     ExchangeRequestCache& exchange_cache,
                                     QObject* parent)
    : QAbstractTableModel{parent}
    , game{game}
    , exchange_cache{&exchange_cache}
{
    completer = new QCompleter{this, this};

    completer->setCompletionColumn(static_cast<int>(ExchangeRequestColumn::Name));
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCompletionRole(Qt::DisplayRole);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
}

TradeRequestCache::~TradeRequestCache()
{
    saveCache();
    saveCostCache();
}

bool TradeRequestCache::readDatabase()
{
    auto select = Database::selectTradeCache(game);
    bool result = select.exec();
    if (!result)
        return result;
    while (select.next()) {
        auto p = Database::tradeCacheFromQuery(select);
        if (!p.first.isValid()) {
            result = false;
            break;
        }
        cache.emplace_hint(cache.end(), std::move(p));
    }
    select = Database::selectTradeCostCache(game);
    result = result && select.exec();
    while (select.next()) {
        auto p = Database::tradeCostCacheFromQuery(select, *exchange_cache);
        if (p.first.isEmpty()) {
            result = false;
            break;
        }
        cost_cache.emplace(std::move(p));
    }

    return result;
}

void TradeRequestCache::saveRequest(const TradeRequestKey& request, TradeRequestData data)
{
    auto it = cache.lower_bound(request);
    if (it == cache.end() || it->first != request) {
        beginInsertRows({}, cache.index_of(it), cache.index_of(it));
        it = cache.emplace_hint(it, request, std::move(data));
        endInsertRows();
    } else {
        if (it->second.name() != data.name()) {
            it->second.setName(data.name());
            auto idx = index(cache.index_of(it), static_cast<int>(TradeRequestColumn::Name));
            emit dataChanged(idx, idx, {Qt::DisplayRole});
        }
        if (it->second.query() != data.query())
            it->second.setQuery(data.query());
    }

    if (it->second.is_changed) {
        auto insert = Database::insertTradeCache(game);
        Database::insertTradeCache(insert, it->first, it->second);
        it->second.is_changed = false;
    }
}

void planner::TradeRequestCache::updateCost(const TradeRequestKey& request,
                                            TradeCostData::Data cost_data)
{
    auto it = currentLeagueData();
    if (it == cost_cache.end())
        return;

    auto cost_it = it->second.costs.try_emplace(request).first;
    cost_it->second = std::move(cost_data);
    it->second.is_changed = true;
}

void TradeRequestCache::setDefaultTime(const TradeRequestKey& request, std::optional<ItemTime> time)
{
    auto it = requestData(request);
    if (it == cache.end())
        return;

    if (it->second.defaultTme() != time) {
        it->second.setDefaultTime(time);
        cache_changed = true;
        emit defaultTimeChanged(request);
    }
}

TradeRequestCache::CostCache::iterator TradeRequestCache::currentLeagueData()
{
    return cost_cache.find(Settings::currentLeague(game));
}

bool TradeRequestCache::saveCache() const
{
    if (!cache_changed)
        return true;

    auto db = QSqlDatabase::database();
    db.transaction();
    auto insert = Database::insertTradeCache(game);
    auto result = true;
    for (auto& [id, data] : cache) {
        if (!data.is_changed)
            continue;
        if (!Database::insertTradeCache(insert, id, data))
            result = false;
        else
            data.is_changed = false;
    }
    db.commit();
    if (result)
        cache_changed = false;

    return result;
}

bool TradeRequestCache::saveCostCache() const
{
    auto db = QSqlDatabase::database();
    db.transaction();
    auto insert = Database::insertTradeCostCache(game);
    auto result = true;
    for (auto& [league, data] : cost_cache) {
        if (!data.is_changed)
            continue;
        if (!Database::insertTradeCostCache(insert, league, data))
            result = false;
        else
            data.is_changed = false;
    }
    db.commit();

    return result;
}

bool TradeRequestCache::isOutdated(const TradeRequestKey& request, QDateTime now) const
{
    if (!request.isValid())
        return false;

    auto it = currentLeagueData();
    if (it == cost_cache.end())
        return false;
    auto cost_it = it->second.costs.find(request);
    if (cost_it == it->second.costs.end())
        return true;

    return (cost_it->second.updated + Settings::tradeCostExpirationTime()) <= now;
}

QString TradeRequestCache::name(const TradeItemData& trade_item) const
{
    if (!trade_item.name.isEmpty())
        return trade_item.name;
    if (auto it = requestData(trade_item.request_key); it != cache.end())
        return it->second.name();
    return {};
}

double TradeRequestCache::costValue(const TradeRequestKey& request) const
{
    auto it = currentLeagueData();
    if (it == cost_cache.end())
        return 0.0;
    auto cost_it = it->second.costs.find(request);
    return cost_it != it->second.costs.end() ? cost_it->second.cost.value : 0.0;
}

const TradeCostData::Data* TradeRequestCache::costData(const TradeRequestKey& request) const
{
    auto it = currentLeagueData();
    if (it == cost_cache.end())
        return nullptr;
    auto cost_it = it->second.costs.find(request);
    if (cost_it == it->second.costs.end())
        return nullptr;
    return &cost_it->second;
}

const ExchangeData* TradeRequestCache::costCurrency(const TradeRequestKey& request) const
{
    auto it = currentLeagueData();
    if (it == cost_cache.end())
        return nullptr;
    auto cost_it = it->second.costs.find(request);
    if (cost_it == it->second.costs.end())
        return nullptr;

    auto exchange_it = exchange_cache->currencyData(cost_it->second.cost.currency);
    return exchange_it != exchange_cache->cache.end() ? &(exchange_it->second) : nullptr;
}

double TradeRequestCache::goldFee(const TradeRequestKey& request) const
{
    auto it = currentLeagueData();
    if (it == cost_cache.end())
        return 0.0;
    auto cost_it = it->second.costs.find(request);
    return cost_it != it->second.costs.end() ? cost_it->second.gold_fee : 0.0;
}

ItemTime TradeRequestCache::time(const TradeItemData& trade_item) const
{
    if (trade_item.time)
        return *trade_item.time;
    if (auto it = requestData(trade_item.request_key); it != cache.end() && it->second.defaultTme())
        return *it->second.defaultTme();
    return Settings::defaultTradeTime();
}

QVariant TradeRequestCache::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation == Qt::Vertical)
        return {};
    auto col = static_cast<TradeRequestColumn>(section);
    switch (col) {
    case TradeRequestColumn::Name:
        return tr("Name");
    case TradeRequestColumn::Time:
        return tr("Time");
    }

    return {};
}

QVariant TradeRequestCache::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    auto row = index.row();
    auto request = cache.nth(row);
    auto col = static_cast<TradeRequestColumn>(index.column());
    switch (col) {
    case TradeRequestColumn::Name:
        return request->second.name();
    case TradeRequestColumn::Time:
        return request->second.defaultTme() ? request->second.defaultTme()->count() : QVariant{};
    }
    return {};
}

Qt::ItemFlags TradeRequestCache::flags(const QModelIndex& index) const
{
    return QAbstractTableModel::flags(index);
}

QJsonArray TradeRequestCache::exportRequests()
{
    QJsonArray requests_a;
    for (auto& request : export_requests) {
        if (auto it = requestData(request); it != cache.end()) {
            QJsonObject request_o;
            request_o["request"] = request.toJson();
            it->second.exportJson(request_o);

            requests_a.push_back(request_o);
        }
    }
    export_requests.clear();

    return requests_a;
}

void TradeRequestCache::mergeImportRequests(Cache&& import_requests)
{
    for (auto& p : import_requests) {
        auto name = p.second.name();
        if (!name.startsWith("(I) "))
            p.second.setName(name.prepend("(I) "));
    }
    beginResetModel();
    cache.merge(import_requests);
    endResetModel();
    cache_changed = true;
}

TradeRequestCache::Cache TradeRequestCache::requestsFromJson(const QJsonArray& requests_a)
{
    TradeRequestCache::Cache cache;
    for (auto& request_v : requests_a) {
        auto request_o = request_v.toObject();
        auto key = TradeRequestKey::fromJson(request_o["request"].toObject());
        if (!key.isValid())
            throw ImportException{ImportError::InvalidTradeRequest};

        auto& data = cache.try_emplace(key, request_o).first->second;
        if (data.name().isEmpty())
            throw ImportException{ImportError::InvalidTradeRequest};
    }
    return cache;
}

} // namespace planner
