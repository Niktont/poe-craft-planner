#ifndef TRADEREQUESTDATA_H
#define TRADEREQUESTDATA_H

#include "ItemTime.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace planner {

class TradeRequestData
{
public:
    TradeRequestData() = default;
    TradeRequestData(QString name, QJsonDocument query)
        : name_{name}
        , query_{query}
    {}
    TradeRequestData(const QJsonObject& request_o)
        : name_{request_o["name"].toString()}
        , query_{request_o["query"].toObject()}
        , regex_{request_o["regex"].toString()}
        , description_{request_o["description"].toString()}
    {
        if (auto time_v = request_o["time"]; !time_v.isUndefined())
            default_time = ItemTime{time_v.toDouble()};
    }
    void exportJson(QJsonObject& request_o) const
    {
        request_o["name"] = name_;
        request_o["query"] = query_.object();
        request_o["regex"] = regex_;
        request_o["description"] = description_;
        if (default_time)
            request_o["time"] = default_time->count();
    }

    const QString& name() const { return name_; }
    const QJsonDocument& query() const { return query_; }
    const QString& regex() const { return regex_; }
    const QString& description() const { return description_; }
    const std::optional<ItemTime>& defaultTme() const { return default_time; }

    void setName(const QString& name)
    {
        name_ = name;
        is_changed = true;
    }
    void setQuery(const QJsonDocument& query)
    {
        query_ = query;
        is_changed = true;
    }
    void setRegex(const QString& regex)
    {
        regex_ = regex;
        is_changed = true;
    }
    void setDescription(const QString& description)
    {
        description_ = description;
        is_changed = true;
    }
    void setDefaultTime(std::optional<ItemTime> time)
    {
        default_time = time;
        is_changed = true;
    }

    mutable bool is_changed{true};

private:
    friend class Database;

    QString name_;
    QJsonDocument query_;

    QString regex_;
    QString description_;

    std::optional<ItemTime> default_time;
};

} // namespace planner

#endif // TRADEREQUESTDATA_H
