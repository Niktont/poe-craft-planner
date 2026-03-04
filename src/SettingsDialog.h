#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QListView;
class QStringListModel;
class QStackedWidget;
class QSettings;
class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;

namespace planner {
class MainWindow;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    SettingsDialog(MainWindow& mw);

signals:
    void tradeTimeChanged();
    void exchangeTimeChanged();

public slots:
    void openSettings();

private slots:
    void save();
    void setRequestsChanged();
    void setLeagueChanged();

private:
    QListView* tab_view;
    QStringListModel* tab_model;
    QStackedWidget* tabs_widget;

    QWidget* requests_tab;
    QSpinBox* trade_min_update_time;
    QDoubleSpinBox* trade_default_time;
    QSpinBox* exchange_min_update_time;
    QDoubleSpinBox* exchange_default_time;
    QSpinBox* exchange_delay;
    QCheckBox* reload_data_poe1;
    QCheckBox* reload_data_poe2;

    QWidget* league_tab;
    QComboBox* league_poe1;
    QComboBox* league_poe2;

    void resetTab(int index);

    void setupRequestsTab();
    void resetRequests();
    void saveRequests(QSettings& settings);

    void setupLeagueTab();
    void resetLeague();
    void saveLeague(QSettings& settings);

    std::array<bool, 2> is_changed{};
    std::array<bool, 2> needs_reset;

    MainWindow* mw() const;
};

} // namespace planner

#endif // SETTINGSDIALOG_H
