#ifndef INITIALIZATIONDIALOG_H
#define INITIALIZATIONDIALOG_H

#include "Game.h"
#include <queue>
#include <QDialog>

class QLabel;
class QPushButton;
class QLineEdit;
class QStackedWidget;
class QNetworkReply;
class QComboBox;
class QTimer;

namespace planner {

class MainWindow;

class InitializationDialog : public QDialog
{
    Q_OBJECT
public:
    InitializationDialog(MainWindow& parent);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void initDatabase();
    void readDatabase();

    void requestLeagues();
    void parseLeagues(planner::Game game, QNetworkReply* reply);

    void updateCacheData();

    void requestData();
    void parseOverviewData(planner::Game game, QString type, QNetworkReply* reply);

    void finishInitialization();

private:
    MainWindow* mw;
    QLabel* progress_label;
    QPushButton* continue_button;

    QWidget* select_league_widget;
    QStackedWidget* stacked_widget;
    QComboBox* league_combo_poe1;
    QComboBox* league_combo_poe2;

    bool request_finished_poe1{false};
    bool request_finished_poe2{false};

    QStringList league_urls_poe1;
    QStringList league_urls_poe2;

    std::queue<QString> currency_types_poe1;
    std::queue<QString> currency_types_poe2;

    void requestFailed(QString type);
    void parseFailed(QString type);
    bool isDataNeeded(Game game);
};

} // namespace planner

#endif // INITIALIZATIONDIALOG_H
