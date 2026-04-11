#include "TradeRequestCache.h"
#include "Database.h"
#include "ExchangeRequestCache.h"
#include "ImportException.h"
#include "QueryParser.h"
#include "Settings.h"
#include "Snapshot.h"
#include "TradeItemData.h"
#include <QCompleter>
#include <QFont>
#include <QSortFilterProxyModel>

namespace planner {

TradeRequestCache::TradeRequestCache(Game game,
                                     ExchangeRequestCache& exchange_cache,
                                     QObject* parent)
    : QAbstractTableModel{parent}
    , completer{new QCompleter{{}, this}}
    , proxy_model{new QSortFilterProxyModel{this}}
    , game{game}
    , exchange_cache{&exchange_cache}
{
    completer->setCompletionColumn(static_cast<int>(TradeRequestColumn::Name));
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCompletionRole(Qt::DisplayRole);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setModel(this);

    proxy_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy_model->setFilterKeyColumn(static_cast<int>(TradeRequestColumn::Name));
    proxy_model->setDynamicSortFilter(false);
    proxy_model->setSourceModel(this);
}

bool TradeRequestCache::readDatabase()
{
    auto select = Database::selectTradeCache(game);
    bool result = select.exec();
    if (!result)
        return result;

    beginResetModel();
    while (select.next()) {
        auto p = Database::tradeCacheFromQuery(select);
        if (!p.first.isValid()) {
            result = false;
            break;
        }
        cache.emplace_hint(cache.end(), std::move(p));
    }
    endResetModel();

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
    result = result && readAdditionalDatabase();

    return result;
}

bool TradeRequestCache::readAdditionalDatabase()
{
    auto lang = Settings::get<Settings::language_trade_query>();
    auto query = Database::selectFilters(game, lang);
    bool result = query.exec();
    if (!result)
        return result;

    while (query.next())
        query_parser.filters.try_emplace(query.value(0).toString(), query.value(1).toString());

    query = Database::selectFilterOptions(game, lang);
    result = result && query.exec();
    while (query.next())
        query_parser.filter_options.try_emplace(query.value(0).toString(),
                                                query.value(1).toString());

    query = Database::selectStatTypes(game, lang);
    result = result && query.exec();
    while (query.next())
        query_parser.stat_types.try_emplace(query.value(0).toString(), query.value(1).toString());

    query = Database::selectStats(game, lang);
    result = result && query.exec();
    while (query.next())
        query_parser.stats.try_emplace(query.value(0).toString(), query.value(1).toString());

    query = Database::selectStatGroups(game, lang);
    result = result && query.exec();
    while (query.next())
        query_parser.stat_groups.try_emplace(query.value(0).toString(), query.value(1).toString());

    return result;
}

void TradeRequestCache::saveRequest(const TradeRequestKey& request,
                                    const QString& name,
                                    const QJsonDocument& query,
                                    const QString& regex,
                                    const RequestDescription& description)
{
    auto it = cache.lower_bound(request);
    auto pos = cache.index_of(it);
    bool is_changed = false;
    if (it == cache.end() || it->first != request) {
        beginInsertRows({}, pos, pos);
        it = cache.try_emplace(it, request, name, query, regex, description);
        endInsertRows();
        is_changed = true;
    } else {
        if (it->second.name() != name) {
            it->second.name_ = name;
            auto idx = index(pos, static_cast<int>(TradeRequestColumn::Name));
            emit dataChanged(idx, idx, {Qt::DisplayRole});
            is_changed = true;
        }
        if (it->second.query() != query) {
            it->second.query_ = query;
            auto idx = index(pos, static_cast<int>(TradeRequestColumn::Query));
            emit dataChanged(idx, idx, {Qt::CheckStateRole});
            is_changed = true;
        }
        if (it->second.regex_ != regex) {
            it->second.regex_ = regex;
            auto idx = index(pos, static_cast<int>(TradeRequestColumn::Regex));
            emit dataChanged(idx, idx, {Qt::DisplayRole});
            is_changed = true;
        }
        if (it->second.description() != description) {
            it->second.description_ = description;
            auto idx = index(pos, static_cast<int>(TradeRequestColumn::Description));
            emit dataChanged(idx, idx, {Qt::DisplayRole});
            is_changed = true;
        }
    }

    if (is_changed) {
        auto insert = Database::insertTradeCache(game);
        Database::insertTradeRequest(insert, it->first, it->second);
    }
}

void TradeRequestCache::deleteRequests(int top, int bottom)
{
    for (int i = top; i <= bottom; ++i)
        Database::deleteTradeCache(game, cache.nth(i)->first);

    beginRemoveRows({}, top, bottom);
    cache.erase(cache.nth(top), cache.nth(bottom + 1));
    endRemoveRows();
}

void TradeRequestCache::deleteRequest(Cache::const_iterator it)
{
    if (it == cache.end())
        return;

    Database::deleteTradeCache(game, it->first);

    auto pos = cache.index_of(it);
    beginRemoveRows({}, pos, pos);
    cache.erase(it);
    endRemoveRows();
}

void TradeRequestCache::setSnapshot(Snapshot* snapshot)
{
    this->snapshot = snapshot;
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

    if (it->second.defaultTime() != time) {
        it->second.default_time = time;
        Database::updateTradeRequestTime(game, it->first, it->second);
        auto idx = index(cache.index_of(it), static_cast<int>(TradeRequestColumn::Time));
        emit dataChanged(idx, idx, {Qt::DisplayRole});
    }
}

TradeRequestCache::CostCache::iterator TradeRequestCache::currentLeagueData()
{
    return cost_cache.find(Settings::currentLeague(game));
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
    auto data = costData(request);
    return data ? data->cost.value : 0.0;
}

const TradeCostData::Data* TradeRequestCache::costData(const TradeRequestKey& request) const
{
    if (snapshot) {
        auto it = snapshot->trade.costs.find(request);
        if (it != snapshot->trade.costs.end())
            return &it->second;
        else if (!Settings::get<settings::snapshots_use_current_if_missing>())
            return nullptr;
    }
    auto league_it = currentLeagueData();
    if (league_it == cost_cache.end())
        return nullptr;

    auto cost_it = league_it->second.costs.find(request);
    return cost_it != league_it->second.costs.end() ? &cost_it->second : nullptr;
}

const ExchangeData* TradeRequestCache::costCurrency(const TradeRequestKey& request) const
{
    auto data = costData(request);
    if (!data)
        return nullptr;

    auto exchange_it = exchange_cache->currencyData(data->cost.currency);
    return exchange_it != exchange_cache->cache.end() ? &(exchange_it->second) : nullptr;
}

double TradeRequestCache::goldFee(const TradeRequestKey& request) const
{
    auto data = costData(request);
    return data ? data->gold_fee : 0.0;
}

ItemTime TradeRequestCache::time(const TradeItemData& trade_item) const
{
    if (trade_item.time)
        return *trade_item.time;
    if (auto it = requestData(trade_item.request_key); it != cache.end() && it->second.defaultTime())
        return *it->second.defaultTime();
    return Settings::defaultTradeTime();
}

QString TradeRequestCache::description(const TradeRequestKey& request) const
{
    if (auto it = requestData(request); it != cache.end())
        return description(it);

    return {};
}

QVariant TradeRequestCache::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation == Qt::Vertical)
        return {};
    auto col = static_cast<TradeRequestColumn>(section);
    switch (col) {
    case TradeRequestColumn::Name:
        return tr("Name");
    case TradeRequestColumn::Link:
        return tr("Link");
    case TradeRequestColumn::Query:
        return tr("Q");
    case TradeRequestColumn::Regex:
        return tr("Regex");
    case TradeRequestColumn::Description:
        return tr("Description");
    case TradeRequestColumn::Time:
        return tr("Time");
    }

    return {};
}

int TradeRequestCache::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return cache.size();
}

int TradeRequestCache::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(TradeRequestColumn::last) + 1;
}

QVariant TradeRequestCache::data(const QModelIndex& index, int role) const
{
    auto request = cache.nth(index.row());
    auto col = static_cast<TradeRequestColumn>(index.column());
    switch (col) {
    case TradeRequestColumn::Name:
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return request->second.name();
        case Qt::ToolTipRole:
            return description(request);
        }
        return {};
    case TradeRequestColumn::Link:
        switch (role) {
        case Qt::DisplayRole:
            return tr("Link");
        case Qt::ToolTipRole:
            return request->first.toUrl(game);
        case Qt::FontRole: {
            QFont font;
            font.setUnderline(true);
            return font;
        }
        case Qt::ForegroundRole:
            if (request->first.isValid())
                return QColor{0x0000EE};
            return {};
        }
        return {};
    case TradeRequestColumn::Query:
        switch (role) {
        case Qt::CheckStateRole:
            return !request->second.query().isEmpty() ? Qt::Checked : Qt::Unchecked;
        case Qt::ToolTipRole:
            return description(request);
        }
        return {};
    case TradeRequestColumn::Regex:
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return request->second.regex();
        }
        return {};
    case TradeRequestColumn::Description:
        switch (role) {
        case Qt::DisplayRole:
        //     switch (request->second.description().type) {
        //     case DescriptionType::Text:
        //         return tr("Text");
        //     }
        //     return {};
        case Qt::EditRole:
            // case Qt::ToolTipRole:
            switch (request->second.description().type) {
            case DescriptionType::Text:
                return request->second.description().text;
            }
        }
        return {};
    case TradeRequestColumn::Time:
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return request->second.defaultTime()
                       ? QString::number(request->second.defaultTime()->count())
                       : QVariant{};
        case Qt::TextAlignmentRole:
            return QVariant{Qt::AlignRight | Qt::AlignVCenter};
        }
        return {};
    }
    return {};
}

bool TradeRequestCache::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    if (index.row() >= std::ssize(cache))
        return false;

    auto request = cache.nth(index.row());
    auto col = static_cast<TradeRequestColumn>(index.column());
    switch (col) {
    case TradeRequestColumn::Name: {
        auto name = value.toString();
        if (!name.isEmpty() && request->second.name() != name) {
            request->second.name_ = name;
            Database::updateTradeRequestName(game, request->first, request->second);
            emit dataChanged(index, index, {Qt::DisplayRole});
            return true;
        }
        return false;
    }
    case TradeRequestColumn::Regex: {
        auto regex = value.toString();
        if (request->second.regex() != regex) {
            request->second.regex_ = regex;
            Database::updateTradeRequestRegex(game, request->first, request->second);
            emit dataChanged(index, index, {Qt::DisplayRole});
            return true;
        }
        return false;
    }
    case TradeRequestColumn::Description: {
        auto description = value.toString();
        if (request->second.description().text != description) {
            request->second.description_.text = description;
            Database::updateTradeRequestDescription(game, request->first, request->second);
            emit dataChanged(index, index, {Qt::DisplayRole});
            return true;
        }
        return false;
    }
    case TradeRequestColumn::Time: {
        auto time_str = value.toString();
        if (time_str.isEmpty()) {
            if (!request->second.default_time.has_value())
                return false;
            request->second.default_time.reset();
            Database::updateTradeRequestTime(game, request->first, request->second);
            emit dataChanged(index, index, {Qt::DisplayRole});
            return true;
        }
        if (auto val = ItemTime{time_str.toDouble()};
            val.count() >= 0.0 && request->second.default_time != val) {
            request->second.default_time = val;
            Database::updateTradeRequestTime(game, request->first, request->second);
            emit dataChanged(index, index, {Qt::DisplayRole});
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

Qt::ItemFlags TradeRequestCache::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return {Qt::NoItemFlags};

    auto default_flags = QAbstractTableModel::flags(index);

    auto col = static_cast<TradeRequestColumn>(index.column());
    if (col == TradeRequestColumn::Link || col == TradeRequestColumn::Query)
        return default_flags;
    return default_flags | Qt::ItemIsEditable;
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
    if (Settings::get<Settings::import_add_prefix_requests>()) {
        for (auto& [request, data] : import_requests) {
            if (!data.name_.startsWith("(I) "))
                data.name_.prepend("(I) ");
        }
    }

    auto db = QSqlDatabase::database();
    db.transaction();
    auto insert = Database::insertTradeCache(game);
    for (auto& [request, data] : import_requests) {
        auto it = cache.lower_bound(request);
        if (it == cache.end() || it->first != request) {
            beginInsertRows({}, cache.index_of(it), cache.index_of(it));
            it = cache.emplace_hint(it, std::move(request), std::move(data));
            endInsertRows();
            Database::insertTradeRequest(insert, it->first, it->second);
        }
    }
    db.commit();
}

QString TradeRequestCache::description(Cache::const_iterator it) const
{
    if (Settings::get<Settings::trade_use_query_as_description>() && !it->second.query().isEmpty()) {
        auto& query = it->second.description().query;
        if (!query)
            query = query_parser.parseQuery(it->second.query());
        return *query;
    }

    return it->second.description().text;
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
