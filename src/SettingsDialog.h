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
class HotkeyEdit;

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
    void setImportChanged();
    void setLanguageChanged();
    void setHotkeysChanged();

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
    QCheckBox* snapshot_use_current_data;

    QWidget* import_tab;
    QCheckBox* overwrite_names;
    QCheckBox* add_prefix;
    QCheckBox* add_prefix_requests;

    QWidget* language_tab;
    QComboBox* currency_language;
    QComboBox* trade_query_language;

    QWidget* hotkeys_tab;
    HotkeyEdit* next_item;
    HotkeyEdit* paste_want;
    HotkeyEdit* paste_want_amount;
    HotkeyEdit* paste_have;
    HotkeyEdit* paste_have_amount;
    HotkeyEdit* open_link;

    void resetTab(int index);

    void setupRequestsTab();
    void resetRequests();
    void saveRequests(QSettings& settings);

    void setupLeagueTab();
    void resetLeague();
    void saveLeague(QSettings& settings);

    void setupImportTab();
    void resetImport();
    void saveImport(QSettings& settings);

    void setupLanguageTab();
    void resetLanguage();
    void saveLanguage(QSettings& settings);

    void setupHotkeysTab();
    void resetHotkeys();
    void saveHotkeys(QSettings& settings);

    std::array<bool, 5> is_changed{};
    std::array<bool, 5> needs_reset;

    MainWindow* mw() const;
};

} // namespace planner

#endif // SETTINGSDIALOG_H
