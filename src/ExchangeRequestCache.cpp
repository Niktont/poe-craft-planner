#include "ExchangeRequestCache.h"
#include "Database.h"
#include "ExchangeItemData.h"
#include "Settings.h"
#include "Snapshot.h"
#include <QCompleter>
#include <QFile>
#include <QTextStream>

using namespace Qt::StringLiterals;

namespace planner {

ExchangeRequestCache::ExchangeRequestCache(Game game, QObject* parent)
    : QAbstractTableModel{parent}
    , completer{new QCompleter{{}, this}}
    , game{game}
    , div_card_icon{iconFileName(div_card_icon_id)}
{
    completer->setCompletionColumn(static_cast<int>(ExchangeRequestColumn::Name));
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCompletionRole(Qt::DisplayRole);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setModel(this);
}

QVariant ExchangeRequestCache::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation == Qt::Vertical)
        return {};
    auto col = static_cast<ExchangeRequestColumn>(section);
    switch (col) {
    case ExchangeRequestColumn::Name:
        return tr("Name");
    }

    return {};
}

int ExchangeRequestCache::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return cache.size();
}

int ExchangeRequestCache::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(ExchangeRequestColumn::last) + 1;
}

QVariant ExchangeRequestCache::data(const QModelIndex& index, int role) const
{
    auto row = index.row();
    auto col = static_cast<ExchangeRequestColumn>(index.column());
    switch (col) {
    case ExchangeRequestColumn::Name:
        switch (role) {
        case Qt::DisplayRole:
            return name(cache.nth(row));
        case Qt::DecorationRole: {
            return icon(cache.nth(row));
        }
        }
        break;
    }
    return {};
}

Qt::ItemFlags ExchangeRequestCache::flags(const QModelIndex& index) const
{
    return QAbstractTableModel::flags(index);
}

const QString ExchangeRequestCache::div_card_icon_id{u"div_card"_s};

std::pair<const ExchangeCostData*, const ExchangeCostData::Data*> ExchangeRequestCache::costData(
    const Currency& currency) const
{
    if (snapshot) {
        auto data = &snapshot->exchange;
        auto it = data->costs.find(currency.id);
        if (it != data->costs.end())
            return {data, &it->second};
        else if (!Settings::get<settings::snapshots_use_current_if_missing>())
            return {nullptr, nullptr};
    }

    auto league_it = currentLeagueData();
    if (league_it == cost_cache.end())
        return {nullptr, nullptr};

    auto cost_it = league_it->second.costData(currency.id);
    if (cost_it == league_it->second.costs.end())
        return {nullptr, nullptr};

    return {&league_it->second, &cost_it->second};
}

const ExchangeCostData::Data* ExchangeRequestCache::currencyCostData(const Currency& currency) const
{
    if (snapshot) {
        auto data = &snapshot->exchange;
        auto it = data->costs.find(currency.id);
        if (it != data->costs.end())
            return &it->second;
        else if (!Settings::get<settings::snapshots_use_current_if_missing>())
            return nullptr;
    }
    auto league_it = currentLeagueData();
    if (league_it == cost_cache.end())
        return nullptr;

    auto cost_it = league_it->second.costs.find(currency.id);
    return cost_it != league_it->second.costs.end() ? &cost_it->second : nullptr;
}

QString ExchangeRequestCache::iconFileName(Cache::const_iterator it) const
{
    if (it->second.type == div_card_type)
        return iconFileName(game, div_card_icon_id);

    return iconFileName(game, it->first);
}

QString ExchangeRequestCache::iconFileName(Game game, const QString& id)
{
    return game == Game::Poe1 ? "currency_icons/poe1/" % id % ".png"
                              : "currency_icons/poe2/" % id % ".png";
}

QIcon ExchangeRequestCache::icon(Cache::const_iterator it) const
{
    if (it->second.type == div_card_type)
        return div_card_icon;
    else
        return it->second.icon;
}

bool ExchangeRequestCache::isCore(const Currency& currency) const
{
    auto league_it = currentLeagueData();
    if (league_it == cost_cache.end())
        return false;

    auto core_it = league_it->second.findCore(currency.id);
    return core_it != league_it->second.core_currencies.end();
}

CurrencyCost ExchangeRequestCache::convertToPrimary(const Currency& currency) const
{
    CurrencyCost result;
    auto [data, currency_data] = costData(currency);
    if (!data)
        return result;

    auto primary_value = data->primaryValue(currency.id);
    if (!primary_value) {
        primary_value = data->primaryValue(currency_data->popular.currency.id);
        if (!primary_value)
            return result;

        result.value = *primary_value * currency_data->popular.value;
        result.currency = data->primaryCurrency();
        return result;
    } else {
        result.value = *primary_value;
        result.currency = data->primaryCurrency();
        return result;
    }
}

ItemTime ExchangeRequestCache::time(const ExchangeItemData& exchange_item) const
{
    if (exchange_item.time)
        return *exchange_item.time;
    if (auto it = currencyData(exchange_item.currency);
        it != cache.end() && it->second.defaultTime())
        return *it->second.defaultTime();
    return Settings::defaultExchangeTime();
}

bool ExchangeRequestCache::saveCache() const
{
    if (!cache_changed)
        return true;

    auto db = QSqlDatabase::database();
    db.transaction();
    auto insert = Database::insertExchangeCache(game);
    auto result = true;
    for (auto& [id, data] : cache) {
        if (!data.is_changed)
            continue;
        if (!Database::insertExchangeCache(insert, id, data))
            result = false;
        else
            data.is_changed = false;
    }
    db.commit();
    if (result)
        cache_changed = false;

    return result;
}

bool ExchangeRequestCache::saveCostCache() const
{
    auto db = QSqlDatabase::database();
    db.transaction();
    auto insert = Database::insertExchangeCostCache(game);
    auto result = true;
    for (auto& [league, data] : cost_cache) {
        if (!data.is_changed)
            continue;
        if (!Database::insertExchangeCostCache(insert, league, data))
            result = false;
        else
            data.is_changed = false;
    }
    db.commit();

    return result;
}

const ExchangeCostData* ExchangeRequestCache::costData() const
{
    if (snapshot)
        return &snapshot->exchange;

    auto league_it = currentLeagueData();
    return league_it != cost_cache.end() ? &league_it->second : nullptr;
}

bool ExchangeRequestCache::isOutdated(const Currency& currency, QDateTime now) const
{
    if (currency.id.isEmpty())
        return false;

    auto league_it = currentLeagueData();
    if (league_it == cost_cache.end())
        return false;
    auto cost_it = league_it->second.costData(currency.id);
    if (cost_it == league_it->second.costs.end())
        return true;

    return (cost_it->second.updated + Settings::exchangeCostExpirationTime()) <= now;
}

QString ExchangeRequestCache::name(const ExchangeData& data)
{
    return !data.translated_name.isEmpty() ? data.translated_name : data.name;
}

bool ExchangeRequestCache::shareCurrencyId(QString& id) const
{
    if (auto it = cache.find(id); it != cache.end()) {
        id = it->first;
        return true;
    }
    return false;
}

bool ExchangeRequestCache::shareCurrencyType(QString& type) const
{
    if (auto it = currency_types.find(type); it != currency_types.end()) {
        type = it->first;
        return true;
    }
    return false;
}

bool ExchangeRequestCache::prepareCurrency(Currency& currency) const
{
    if (auto it = currencyData(currency); it != cache.end()) {
        currency.id = it->first;
        return true;
    }
    return false;
}

bool ExchangeRequestCache::prepareData(ExchangeData& data) const
{
    if (auto it = currency_types.find(data.type); it != currency_types.end()) {
        data.type = it->first;
        return true;
    }
    return false;
}

ExchangeRequestCache::CostCache::iterator ExchangeRequestCache::currentLeagueData()
{
    return cost_cache.find(Settings::currentLeague(game));
}

QString ExchangeRequestCache::link(const Currency& currency) const
{
    auto it = currencyData(currency);
    if (it == cache.end())
        return {};
    auto cost_it = currentLeagueData();
    if (cost_it == cost_cache.end())
        return {};

    auto game_url = game == Game::Poe1 ? u"poe1"_s : u"poe2"_s;
    return u"https://poe.ninja/%1/economy/%2/%3/%4"_s.arg(game_url,
                                                          cost_it->second.league_url,
                                                          currency_types.at(it->second.type),
                                                          it->second.details_id);
}

CurrencyCost ExchangeRequestCache::cost(const Currency& currency) const
{
    auto data = currencyCostData(currency);
    return data ? data->popular : CurrencyCost{};
}

std::pair<double, ExchangeRequestCache::Cache::const_iterator> ExchangeRequestCache::costCurrency(
    const Currency& currency) const
{
    auto data = currencyCostData(currency);
    if (!data)
        return {0.0, cache.end()};

    return {data->popular.value, currencyData(data->popular.currency)};
}

ExchangeRequestCache::~ExchangeRequestCache() noexcept
{
    saveCache();
    saveCostCache();
}

bool ExchangeRequestCache::readCurrencyTypes()
{
    auto select = Database::selectCurrencyType(game);
    bool result = select.exec();
    if (!result)
        return result;
    while (select.next())
        currency_types.try_emplace(select.value(0).toString(), select.value(1).toString());

    if (auto it = currency_types.find(u"DivinationCard"_s); it != currency_types.end())
        div_card_type = it->first;

    return !currency_types.empty();
}

bool ExchangeRequestCache::readDatabase()
{
    auto select = Database::selectExchangeCache(game);
    bool result = select.exec();
    if (!result)
        return result;

    beginResetModel();
    while (select.next()) {
        auto p = Database::exchangeCacheFromQuery(select, game);
        if (p.first.isEmpty()) {
            result = false;
            break;
        }
        shareCurrencyType(p.second.type);
        cache.emplace_hint(cache.end(), std::move(p));
    }
    select = Database::selectExchangeCostCache(game);
    result = result && select.exec();
    while (select.next()) {
        auto p = Database::exchangeCostCacheFromQuery(select, *this);
        if (p.first.isEmpty()) {
            result = false;
            break;
        }
        cost_cache.emplace(std::move(p));
    }
    result = result && readAdditionalData();
    endResetModel();

    return result;
}

bool ExchangeRequestCache::readAdditionalData()
{
    auto select = Database::selectCurrencyData(game,
                                               Settings::get<Settings::language_exchange_items>());
    bool result = select.exec();
    if (!result)
        return result;
    while (select.next()) {
        auto id = select.value(0).toString();
        auto fee = select.value(1).toDouble();
        auto translated_name = select.value(2).toString();
        if (auto it = cache.find(id); it != cache.end()) {
            it->second.gold_fee = fee;
            it->second.translated_name = translated_name;
        }
    }
    return true;
}

void ExchangeRequestCache::setDefaultTime(const Currency& currency, std::optional<ItemTime> time)
{
    auto it = currencyData(currency);
    if (it == cache.end())
        return;

    if (it->second.defaultTime() != time) {
        it->second.setDefaultTime(time);
        cache_changed = true;
        emit defaultTimeChanged(currency);
    }
}

void ExchangeRequestCache::setSnapshot(Snapshot* snapshot)
{
    this->snapshot = snapshot;
}

ExchangeRequestCache::Cache::iterator ExchangeRequestCache::currencyData(const Currency& currency)
{
    if (currency.cache_pos < cache.size()) {
        auto it = cache.nth(currency.cache_pos);
        if (it->first != currency.id) {
            it = cache.find(currency.id);
            if (it == cache.end())
                return it;

            currency.cache_pos = std::distance(cache.begin(), it);
            return it;
        }
        return it;
    }

    auto it = cache.find(currency.id);
    if (it == cache.end())
        return it;

    currency.cache_pos = std::distance(cache.begin(), it);
    return it;
}

} // namespace planner
