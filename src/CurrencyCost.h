#ifndef CURRENCYCOST_H
#define CURRENCYCOST_H

#include <QJsonObject>
#include <QString>

namespace planner {

class ExchangeRequestCache;

class Currency
{
public:
    QString id;
    mutable size_t cache_pos{invalid_pos};

    bool isValid() const { return !id.isEmpty(); }
    QJsonValue toJson() const { return id; }
    static Currency fromJson(const QJsonValue& id, const ExchangeRequestCache& cache);

    static constexpr size_t invalid_pos{static_cast<size_t>(-1)};
};

class CoreCurrency
{
public:
    double ratio_to_primary{};
    Currency currency;
    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["ratio"] = ratio_to_primary;
        obj["currency"] = currency.toJson();
        return obj;
    }
    static CoreCurrency fromJson(const QJsonObject& core, const ExchangeRequestCache& cache);
};

class CurrencyCost
{
public:
    double value{};
    Currency currency;

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["value"] = value;
        obj["currency"] = currency.toJson();
        return obj;
    }
    static CurrencyCost fromJson(const QJsonObject& cost, const ExchangeRequestCache& cache);

    CurrencyCost& operator+=(const CurrencyCost& r)
    {
        value += r.value;
        if (!currency.isValid())
            currency = r.currency;
        return *this;
    }
    CurrencyCost& operator-=(const CurrencyCost& r)
    {
        value -= r.value;
        if (!currency.isValid())
            currency = r.currency;
        return *this;
    }
    friend CurrencyCost operator+(CurrencyCost l, const CurrencyCost& r)
    {
        l += r;
        return l;
    }
    friend CurrencyCost operator-(CurrencyCost l, const CurrencyCost& r)
    {
        l -= r;
        return l;
    }
    friend bool operator<(const CurrencyCost& l, const CurrencyCost& r)
    {
        return l.value < r.value;
    }
};

} // namespace planner

#endif // CURRENCYCOST_H
