#ifndef EXCHANGEREQUESTMANAGER_H
#define EXCHANGEREQUESTMANAGER_H

#include "Game.h"
#include <QObject>

class QRestAccessManager;
class QNetworkReply;
class QWebEngineView;
class QNetworkReply;
class QJsonObject;

namespace planner {
class MainWindow;
class ExchangeRequestCache;
class ExchangeRequestManager : public QObject
{
    Q_OBJECT
public:
    explicit ExchangeRequestManager(QRestAccessManager& manager,
                                    QWebEngineView& web_view,
                                    MainWindow& parent);

    QNetworkReply* getLeagues(Game game);
    QNetworkReply* getOverview(Game game, const QString& type);

    static bool parseLeagues(const QJsonObject& obj, QStringList& names, QStringList& urls);

    bool parseOverviewItems(const QJsonObject& overview,
                            const QString& type,
                            ExchangeRequestCache& cache);
    static bool parseOverviewCosts(const QJsonObject& overview, ExchangeRequestCache& cache);
    static bool parseCore(const QJsonObject& core, ExchangeRequestCache& cache);

public slots:
    void downloadIcon(const QString& link_path, const QString& file_path);

private:
    QRestAccessManager* manager;
    QWebEngineView* web_view;
    QString userAgent();
};

} // namespace planner

#endif // EXCHANGEREQUESTMANAGER_H
