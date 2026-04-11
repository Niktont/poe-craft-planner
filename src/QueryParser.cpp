#include "QueryParser.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Qt::StringLiterals;

namespace planner {
static const QString min_max_format{u" %1/%2"_s};
static const QString min_format{u" %1"_s};

QString QueryParser::parseQuery(const QJsonDocument& query_json) const
{
    QString result;
    if (query_json.isEmpty())
        return result;

    const auto json_o = query_json.object();
    const auto query_o = json_o["query"].toObject();

    if (auto type_v = query_o["type"]; !type_v.isUndefined()) {
        if (auto name_v = query_o["name"]; !name_v.isUndefined())
            result.append(name_v.toString() % u' ');
        result.append(type_v.toString() % u'\n');
    }

    const auto filters_o = query_o["filters"].toObject();
    for (auto group_v : filters_o) {
        auto group_o = group_v.toObject();
        if (group_o["disabled"].toBool())
            continue;

        const auto group_filters_o = group_o["filters"].toObject();
        for (auto [key, value] : group_filters_o.asKeyValueRange())
            parseFilter(result, key.toString(), value.toObject());
    }

    const auto stats_a = query_o["stats"].toArray();
    bool first = true;
    for (auto stat_group_v : stats_a) {
        const auto stat_group_o = stat_group_v.toObject();
        if (stat_group_o["disabled"].toBool())
            continue;

        parseStatGroup(result, stat_group_o, first);
        first = false;
    }

    if (result.endsWith(u'\n'))
        result.removeLast();

    return result;
}

void QueryParser::parseFilter(QString& result, QString key, const QJsonObject& filter_o) const
{
    auto it = filters.find(key);
    if (it == filters.end())
        return;

    result.append(it->second);

    auto option_v = filter_o["option"];
    if (!option_v.isUndefined()) {
        if (auto option_it = filter_options.find(option_v.toString());
            option_it != filter_options.end())
            result.append(u": " % option_it->second);
        else
            result.append(u':');
    } else
        result.append(u':');

    result.append(parseMinMax(filter_o) % u'\n');
}

void QueryParser::parseStatGroup(QString& result, const QJsonObject& stat_group_o, bool first) const
{
    const auto stat_filters_a = stat_group_o["filters"].toArray();
    if (stat_filters_a.empty())
        return;

    auto type = stat_group_o["type"].toString();
    auto type_it = stat_groups.find(type);
    if (!(first && type == u"and")) {
        result.append(type_it != stat_groups.end() ? type_it->second : type);
        auto group_value_o = stat_group_o["value"].toObject();
        result.append(parseMinMax(group_value_o) % u'\n');
    }

    for (auto stat_v : stat_filters_a) {
        const auto stat_o = stat_v.toObject();
        if (stat_o["disabled"].toBool())
            continue;

        auto parts = stat_o["id"].toString().split(u'.');
        if (parts.size() != 2)
            continue;

        const auto value_o = stat_o["value"].toObject();
        parseStat(result, parts[0], parts[1], value_o);
    }
}

void QueryParser::parseStat(QString& result,
                            const QString& type,
                            const QString& id,
                            const QJsonObject& value_o) const
{
    auto type_it = stat_types.find(type);
    if (type_it == stat_types.end())
        return;

    auto id_it = stats.find(id);
    if (id_it == stats.end())
        return;

    auto min_max_str = parseMinMax(value_o);
    if (!min_max_str.isEmpty())
        result.append(u' ' % min_max_str % u" | " % id_it->second);
    else
        result.append(u"  " % id_it->second);

    if (type != u"explicit")
        result.append(u" (" % type_it->second % u')');

    if (auto weight_v = value_o["weight"]; !weight_v.isUndefined())
        result.append(u": w" % QString::number(weight_v.toDouble()));

    result.append(u'\n');
}

QString QueryParser::parseMinMax(const QJsonObject& obj) const
{
    auto min_v = obj["min"];
    auto max_v = obj["max"];
    if (min_v.isUndefined() && max_v.isUndefined())
        return {};

    auto min_str = !min_v.isUndefined() ? QString::number(min_v.toDouble()) : u"-"_s;
    if (max_v.isUndefined() || max_v.isNull()) {
        return min_format.arg(min_str);
    }

    auto max_str = QString::number(max_v.toDouble());
    return min_max_format.arg(min_str, max_str);
}

} // namespace planner
