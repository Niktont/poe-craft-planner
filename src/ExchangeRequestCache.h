#ifndef EXCHANGEREQUESTCACHE_H
#define EXCHANGEREQUESTCACHE_H

#include "ExchangeCostData.h"
#include "ExchangeData.h"
#include "Game.h"
#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>
#include <QAbstractTableModel>
#include <QIcon>

class QCompleter;

namespace planner {
enum class ExchangeRequestColumn {
    Name,

    last = Name,
};

class ExchangeItemData;

class ExchangeRequestCache : public QAbstractTableModel
{
    Q_OBJECT
public:
    ExchangeRequestCache(Game game, QObject* parent = nullptr);
    ~ExchangeRequestCache() noexcept;

    bool readCurrencyTypes();
    bool readDatabase();
    bool readAdditionalData();

    using Cache = boost::container::flat_map<QString, ExchangeData>;
    Cache cache;
    mutable bool cache_changed{false};

    using CostCache = boost::container::flat_map<QString, ExchangeCostData>;
    CostCache cost_cache;

    boost::container::flat_map<QString, QString> currency_types;

    QCompleter* completer;
    const Game game;

    Cache::iterator currencyData(const Currency& currency);
    Cache::const_iterator currencyData(const Currency& currency) const
    {
        return const_cast<ExchangeRequestCache*>(this)->currencyData(currency);
    }

    CostCache::iterator currentLeagueData();
    CostCache::const_iterator currentLeagueData() const
    {
        return const_cast<ExchangeRequestCache*>(this)->currentLeagueData();
    };

    void setDefaultTime(const Currency& currency, std::optional<ItemTime> time);

    bool saveCache() const;
    bool saveCostCache() const;

    bool isOutdated(const Currency& currency, QDateTime now) const;

    bool haveLink(const Currency& currency) const { return currencyData(currency) != cache.end(); }
    QString link(const Currency& currency) const;
    double cost(const Currency& currency) const;
    std::pair<double, Cache::const_iterator> costData(const Currency& currency) const;
    QIcon icon(Cache::const_iterator it) const;

    CurrencyCost convertToPrimary(const Currency& currency) const;
    ItemTime time(const ExchangeItemData& exchange_item) const;

    bool shareCurrencyId(QString& id) const;
    bool shareCurrencyType(QString& type) const;
    bool prepareCurrency(Currency& currency) const;
    bool prepareData(ExchangeData& data) const;

    QString iconFileName(const QString& id) const { return iconFileName(game, id); }
    static QString iconFileName(Game game, const QString& id);

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex& /*parent*/ = {}) const override { return cache.size(); }
    int columnCount(const QModelIndex& /*parent*/ = {}) const override
    {
        return static_cast<int>(ExchangeRequestColumn::last) + 1;
    }

    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
signals:
    void defaultTimeChanged(const planner::Currency& currency);

private:
    QIcon div_card_icon;
    QString div_card_type;
};
} // namespace planner
#endif // EXCHANGEREQUESTCACHE_H
