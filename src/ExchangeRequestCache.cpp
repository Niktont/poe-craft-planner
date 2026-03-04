#include "ExchangeRequestCache.h"
#include "Database.h"
#include "ExchangeItemData.h"
#include "Settings.h"
#include <QCompleter>
#include <QFile>
#include <QTextStream>

using namespace Qt::StringLiterals;

namespace planner {

ExchangeRequestCache::ExchangeRequestCache(Game game, QObject* parent)
    : QAbstractTableModel{parent}
    , game{game}
    , div_card_icon{iconFileName(u"div_card"_s)}
{
    completer = new QCompleter{this, this};

    completer->setCompletionColumn(static_cast<int>(ExchangeRequestColumn::Name));
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCompletionRole(Qt::DisplayRole);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
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

QVariant ExchangeRequestCache::data(const QModelIndex& index, int role) const
{
    auto row = index.row();
    auto col = static_cast<ExchangeRequestColumn>(index.column());
    switch (col) {
    case ExchangeRequestColumn::Name:
        switch (role) {
        case Qt::DisplayRole:
            return cache.nth(row)->second.name;
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

CurrencyCost ExchangeRequestCache::convertToPrimary(const Currency& currency) const
{
    CurrencyCost result;
    auto league_it = currentLeagueData();
    if (league_it == cost_cache.end())
        return result;

    auto primary_value = league_it->second.primaryValue(currency.id);
    if (!primary_value) {
        auto cost_it = league_it->second.costData(currency.id);
        if (cost_it == league_it->second.costs.end())
            return result;

        primary_value = league_it->second.primaryValue(cost_it->second.popular.currency.id);
        if (!primary_value)
            return result;

        result.value = *primary_value * cost_it->second.popular.value;
        result.currency = league_it->second.primaryCurrency();
        return result;
    } else {
        result.value = *primary_value;
        result.currency = league_it->second.primaryCurrency();
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

bool ExchangeRequestCache::isOutdated(const Currency& currency, QDateTime now) const
{
    if (currency.id.isEmpty())
        return false;

    auto it = currentLeagueData();
    if (it == cost_cache.end())
        return false;
    auto cost_it = it->second.costData(currency.id);
    if (cost_it == it->second.costs.end())
        return true;

    return (cost_it->second.updated + Settings::exchangeCostExpirationTime()) <= now;
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

double ExchangeRequestCache::cost(const Currency& currency) const
{
    auto it = currentLeagueData();
    if (it == cost_cache.end())
        return 0.0;

    auto cost_it = it->second.costData(currency.id);
    return cost_it != it->second.costs.end() ? cost_it->second.popular.value : 0.0;
}

std::pair<double, ExchangeRequestCache::Cache::const_iterator> ExchangeRequestCache::costData(
    const Currency& currency) const
{
    std::pair<double, ExchangeRequestCache::Cache::const_iterator> result{0.0, cache.end()};

    auto it = currentLeagueData();
    if (it == cost_cache.end())
        return result;
    auto cost_it = it->second.costData(currency.id);
    if (cost_it == it->second.costs.end())
        return result;

    auto& cost = cost_it->second.popular;
    return {cost.value, currencyData(cost.currency)};
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

    return result;
}

bool ExchangeRequestCache::readAdditionalData()
{
    auto select = Database::selectAdditionalData(game);
    bool result = select.exec();
    if (!result)
        return result;
    while (select.next()) {
        auto id = select.value(0).toString();
        auto fee = select.value(1).toDouble();
        if (auto it = cache.find(id); it != cache.end())
            it->second.gold_fee = fee;
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
