#ifndef QUERYPARSER_H
#define QUERYPARSER_H

#include "HashFunctions.h"
#include <boost/unordered/unordered_flat_map.hpp>
#include <map>
#include <QString>

class QJsonDocument;
class QJsonObject;
class QJsonArray;

namespace planner {
class QueryParser;

class ParsedQuery
{
public:
    struct Filter
    {
        QString translated_name;
        std::optional<QString> translated_option;
        std::optional<double> min;
        std::optional<double> max;

        QString toString() const;
    };
    struct Stat
    {
        QString type;
        QString translated_id;
        std::optional<double> weight;
        std::optional<double> min;
        std::optional<double> max;

        QString toString(const QueryParser& parser) const;
    };
    struct StatGroup
    {
        QString type;
        std::optional<double> min;
        std::optional<double> max;
        std::vector<Stat> stats;
    };

    QString name;
    QString type;
    std::map<QString, std::vector<Filter>> filter_groups;
    std::vector<StatGroup> stat_groups;

    QString toString(const QueryParser& parser) const;
};

class QueryParser
{
public:
    QString printQuery(const QJsonDocument& query_json) const;

    ParsedQuery parseQuery(const QJsonDocument& query_json) const;

    using QueryTexts = boost::unordered::unordered_flat_map<QString, QString>;
    QueryTexts filters;
    QueryTexts filter_options;
    QueryTexts stats;
    QueryTexts stat_types;
    QueryTexts stat_groups;

private:
    void parseFilterGroup(ParsedQuery& result, QString group_type, const QJsonObject& group_o) const;
    void parseStatGroup(ParsedQuery& result, const QJsonObject& stat_group_o) const;
    void parseStat(std::vector<ParsedQuery::Stat>& group_stats,
                   const QString& type,
                   const QString& id,
                   const QJsonObject& value_o) const;
};

} // namespace planner

#endif // QUERYPARSER_H
