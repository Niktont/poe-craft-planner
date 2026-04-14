#ifndef EXCHANGEDATA_H
#define EXCHANGEDATA_H

#include "ItemTime.h"
#include <optional>
#include <QIcon>
#include <QString>

namespace planner {

class ExchangeData
{
public:
    QString name;

    QString details_id;
    QString type;

    double gold_fee{};
    QString translated_name;

    const std::optional<ItemTime> defaultTime() const { return default_time; }

    QIcon icon;

private:
    std::optional<ItemTime> default_time;
    friend class Database;
    friend class ExchangeRequestCache;
};

} // namespace planner

#endif // EXCHANGEDATA_H
