#ifndef DATAUPDATER_H
#define DATAUPDATER_H

#include <QJsonDocument>
#include <QObject>
#include <QSqlQuery>

class QNetworkAccessManager;
class QRestAccessManager;
class QRestReply;

namespace planner {

class DataUpdater : public QObject
{
    Q_OBJECT
public:
    explicit DataUpdater(QObject* parent = nullptr);

    QNetworkAccessManager* network_manager;
    QRestAccessManager* rest_manager;

    enum Game {
        Poe1,
        Poe2,
    };
    Game game{Poe1};
    size_t lang{0};

    enum DataType {
        Static,
        Filters,
        Stats,
    };
    DataType data_type{Static};

    static bool initAddConnection();

    static QSqlQuery insertText(QStringView table, QStringView lang);

    static QSqlQuery insertName(Game game, QStringView lang);
    static QSqlQuery insertImageAndName(Game game, QStringView lang);

    static QSqlQuery insertFilter(Game game, QStringView lang);

    static QSqlQuery insertFilterOption(Game game, QStringView lang);

    static QSqlQuery insertStat(Game game, QStringView lang);
    static QSqlQuery insertStatType(Game game, QStringView lang);

    static const QString user_agent;

    std::optional<QJsonDocument> readJson(QRestReply& reply);
    void nextLang();

public slots:
    void getData();
    void parseStaticData(QRestReply& reply);
    void parseFiltersData(QRestReply& reply);
    void parseStatsData(QRestReply& reply);
};

} // namespace planner

#endif // DATAUPDATER_H
