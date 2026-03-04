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

    const std::optional<ItemTime> defaultTime() const { return default_time; }
    void setDefaultTime(const std::optional<ItemTime>& time)
    {
        default_time = time;
        is_changed = true;
    }

    mutable bool is_changed{true};

    QIcon icon;

private:
    std::optional<ItemTime> default_time;
    friend class Database;
};

} // namespace planner

#endif // EXCHANGEDATA_H
