#ifndef INITIALIZATIONDIALOG_H
#define INITIALIZATIONDIALOG_H

#include "Game.h"
#include <QDialog>

class QLabel;
class QPushButton;
class QLineEdit;
class QNetworkReply;
class QComboBox;
class QTimer;

namespace planner {
class ExchangeRequester;
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

    void finishInitialization();

private:
    MainWindow* mw;
    QLabel* progress_label;
    QPushButton* offline_button;
    QPushButton* continue_button;

    QWidget* select_league_widget;
    QComboBox* league_combo_poe1;
    QComboBox* league_combo_poe2;

    QStringList leagues_poe1;
    QStringList leagues_poe2;

    QStringList league_urls_poe1;
    QStringList league_urls_poe2;

    ExchangeRequester* requester;

    bool request_finished_poe1{false};
    bool request_finished_poe2{false};
    bool is_data_needed_poe1{false};
    bool is_data_needed_poe2{false};

    void requestData();
};

} // namespace planner

#endif // INITIALIZATIONDIALOG_H
