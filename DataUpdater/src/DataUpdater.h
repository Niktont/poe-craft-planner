#ifndef DATAUPDATER_H
#define DATAUPDATER_H

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

    static bool initAddConnection();
    static QSqlQuery insertName(Game game, QStringView lang);
    static QSqlQuery insertImageAndName(Game game, QStringView lang);

public slots:
    void getData();
    void parseData(QRestReply& reply);
};

} // namespace planner

#endif // DATAUPDATER_H
