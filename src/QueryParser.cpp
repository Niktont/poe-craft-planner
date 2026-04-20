#include "QueryParser.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Qt::StringLiterals;

namespace planner {
static const QString min_max_format{u" %1/%2"_s};
static const QString min_format{u" %1"_s};

static const QString printMinMax(const std::optional<double>& min, const std::optional<double>& max)
{
    if (!min && !max)
        return {};

    auto min_str = min ? QString::number(*min) : u"-"_s;
    if (!max)
        return min_format.arg(min_str);

    return min_max_format.arg(min_str, QString::number(*max));
}

QString QueryParser::printQuery(const QJsonDocument& query_json) const
{
    if (query_json.isEmpty())
        return {};

    return parseQuery(query_json).toString(*this);
}

ParsedQuery QueryParser::parseQuery(const QJsonDocument& query_json) const
{
    ParsedQuery result;
    if (query_json.isEmpty())
        return result;

    const auto json_o = query_json.object();
    const auto query_o = json_o["query"].toObject();

    if (auto type_v = query_o["type"]; !type_v.isUndefined()) {
        if (auto name_v = query_o["name"]; !name_v.isUndefined())
            result.name = name_v.toString();
        result.type = type_v.toString();
    }

    const auto filters_o = query_o["filters"].toObject();
    for (auto [key, value] : filters_o.asKeyValueRange()) {
        auto group_o = value.toObject();
        if (group_o["disabled"].toBool())
            continue;

        parseFilterGroup(result, key.toString(), group_o);
    }

    const auto stats_a = query_o["stats"].toArray();
    for (auto stat_group_v : stats_a) {
        const auto stat_group_o = stat_group_v.toObject();
        if (stat_group_o["disabled"].toBool())
            continue;

        parseStatGroup(result, stat_group_o);
    }

    return result;
}

void QueryParser::parseFilterGroup(ParsedQuery& result,
                                   QString group_type,
                                   const QJsonObject& group_o) const
{
    auto& group_filters = result.filter_groups.try_emplace(group_type).first->second;

    const auto group_filters_o = group_o["filters"].toObject();
    for (auto [key, value] : group_filters_o.asKeyValueRange()) {
        auto it = filters.find(key.toString());
        if (it == filters.end())
            continue;

        const auto filter_o = value.toObject();
        auto& filter = group_filters.emplace_back();
        filter.translated_name = it->second;

        if (auto option_v = filter_o["option"]; !option_v.isUndefined()) {
            if (auto option_it = filter_options.find(option_v.toString());
                option_it != filter_options.end())
                filter.translated_option = option_it->second;
        } else if (auto input_v = filter_o["input"]; !input_v.isUndefined())
            filter.translated_option = input_v.toString();

        if (auto min_v = filter_o["min"]; !min_v.isUndefined())
            filter.min = min_v.toDouble();
        if (auto max_v = filter_o["max"]; !max_v.isUndefined() && !max_v.isNull())
            filter.min = max_v.toDouble();
    }
}

void QueryParser::parseStatGroup(ParsedQuery& result, const QJsonObject& stat_group_o) const
{
    const auto stat_filters_a = stat_group_o["filters"].toArray();
    if (stat_filters_a.empty())
        return;

    auto& stat_group = result.stat_groups.emplace_back();
    stat_group.type = stat_group_o["type"].toString();

    const auto group_value_o = stat_group_o["value"].toObject();
    if (auto min_v = group_value_o["min"]; !min_v.isUndefined())
        stat_group.min = min_v.toDouble();
    if (auto max_v = group_value_o["max"]; !max_v.isUndefined())
        stat_group.max = max_v.toDouble();

    for (auto stat_v : stat_filters_a) {
        const auto stat_o = stat_v.toObject();
        if (stat_o["disabled"].toBool())
            continue;

        auto parts = stat_o["id"].toString().split(u'.');
        if (parts.size() != 2)
            continue;

        const auto value_o = stat_o["value"].toObject();
        parseStat(stat_group.stats, parts[0], parts[1], value_o);
    }

    if (stat_group.stats.empty())
        result.stat_groups.pop_back();
}

void QueryParser::parseStat(std::vector<ParsedQuery::Stat>& group_stats,
                            const QString& type,
                            const QString& id,
                            const QJsonObject& value_o) const
{
    auto id_it = stats.find(id);
    if (id_it == stats.end())
        return;

    auto& stat = group_stats.emplace_back();
    stat.type = type;
    stat.translated_id = id_it->second;

    if (auto weight_v = value_o["weight"]; !weight_v.isUndefined())
        stat.weight = weight_v.toDouble();

    if (auto min_v = value_o["min"]; !min_v.isUndefined())
        stat.min = min_v.toDouble();
    if (auto max_v = value_o["max"]; !max_v.isUndefined())
        stat.min = max_v.toDouble();
}

QString ParsedQuery::toString(const QueryParser& parser) const
{
    QString result;
    if (!type.isEmpty()) {
        if (!name.isEmpty())
            result.append(name % u' ');
        result.append(type % u'\n');
    }

    for (auto& [group_name, filters] : filter_groups) {
        for (auto& filter : filters)
            result.append(filter.toString());
    }
    bool first_and = true;
    for (auto& group : stat_groups) {
        if (!(first_and && group.type == "and")) {
            auto type_it = parser.stat_groups.find(group.type);
            result.append(type_it != parser.stat_groups.end() ? type_it->second : group.type);

            result.append(printMinMax(group.min, group.max) % u'\n');
            first_and = false;
        }
        for (auto& stat : group.stats)
            result.append(stat.toString(parser));
    }

    if (result.endsWith(u'\n'))
        result.removeLast();

    return result;
}

QString ParsedQuery::Filter::toString() const
{
    QString result;
    result.append(translated_name);

    if (translated_option)
        result.append(u": " % *translated_option);
    else
        result.append(u':');

    result.append(printMinMax(min, max) % u'\n');
    return result;
}

QString ParsedQuery::Stat::toString(const QueryParser& parser) const
{
    QString result;
    auto min_max_str = printMinMax(min, max);
    if (!min_max_str.isEmpty())
        result.append(u' ' % min_max_str % u" | " % translated_id);
    else
        result.append(u"  " % translated_id);

    if (type != u"explicit") {
        auto type_it = parser.stat_types.find(type);
        result.append(u" (" % (type_it != parser.stat_types.end() ? type_it->second : type) % u')');
    }
    if (weight)
        result.append(u" | w" % QString::number(*weight));

    result.append(u'\n');
    return result;
}

} // namespace planner
