#ifndef TRADEREQUESTDATA_H
#define TRADEREQUESTDATA_H

#include "ItemTime.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace planner {
enum class DescriptionType {
    Text,
};

class RequestDescription
{
public:
    RequestDescription() = default;
    RequestDescription(QString text)
        : text{text}
    {}
    RequestDescription(const QJsonObject& description_o)
        : type{static_cast<DescriptionType>(description_o["type"].toInt())}
        , text{description_o["text"].toString()}
    {}

    DescriptionType type{DescriptionType::Text};
    QString text;

    QJsonObject toJson() const
    {
        QJsonObject description_o;
        description_o["type"] = static_cast<std::underlying_type_t<DescriptionType>>(type);
        description_o["text"] = text;
        return description_o;
    }

    friend auto operator<=>(const RequestDescription& l, const RequestDescription& r)
    {
        return std::tie(l.type, l.text) <=> std::tie(r.type, r.text);
    }
    friend bool operator==(const RequestDescription& lhs, const RequestDescription& rhs)
    {
        return (lhs <=> rhs) == 0;
    }
    friend bool operator!=(const RequestDescription& lhs, const RequestDescription& rhs)
    {
        return (lhs <=> rhs) != 0;
    }
};

class TradeRequestData
{
public:
    TradeRequestData() = default;
    TradeRequestData(QString name,
                     QJsonDocument query,
                     QString regex,
                     RequestDescription description)
        : name_{std::move(name)}
        , query_{std::move(query)}
        , regex_{std::move(regex)}
        , description_{std::move(description)}
    {}
    TradeRequestData(const QJsonObject& request_o)
        : name_{request_o["name"].toString()}
        , query_{request_o["query"].toObject()}
        , regex_{request_o["regex"].toString()}
        , description_{request_o["description"].toObject()}
    {
        if (auto time_v = request_o["time"]; !time_v.isUndefined())
            default_time = ItemTime{time_v.toDouble()};
    }
    void exportJson(QJsonObject& request_o) const
    {
        request_o["name"] = name_;
        request_o["query"] = query_.object();
        request_o["regex"] = regex_;
        request_o["description"] = description_.toJson();
        if (default_time)
            request_o["time"] = default_time->count();
    }

    const QString& name() const { return name_; }
    const QJsonDocument& query() const { return query_; }
    const QString& regex() const { return regex_; }
    const RequestDescription& description() const { return description_; }
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
    void setDescription(const RequestDescription& description)
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
    RequestDescription description_;

    std::optional<ItemTime> default_time;
};

} // namespace planner

#endif // TRADEREQUESTDATA_H
