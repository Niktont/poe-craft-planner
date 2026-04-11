#ifndef QUERYPARSER_H
#define QUERYPARSER_H

#include "HashFunctions.h"
#include <boost/unordered/unordered_flat_map.hpp>
#include <QString>

class QJsonDocument;
class QJsonObject;
class QJsonArray;

namespace planner {

class QueryParser
{
public:
    QString parseQuery(const QJsonDocument& query_json) const;

    void parseFilter(QString& result, QString key, const QJsonObject& filter_o) const;

    void parseStatGroup(QString& result, const QJsonObject& stat_group_o, bool first) const;
    void parseStat(QString& result,
                   const QString& type,
                   const QString& id,
                   const QJsonObject& value_o) const;

    QString parseMinMax(const QJsonObject& obj) const;

    using QueryTexts = boost::unordered::unordered_flat_map<QString, QString>;
    QueryTexts filters;
    QueryTexts filter_options;
    QueryTexts stats;
    QueryTexts stat_types;
    QueryTexts stat_groups;
};

} // namespace planner

#endif // QUERYPARSER_H
