#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Game.h"
#include <QMainWindow>
#include <QMessageBox>

class QLineEdit;
class QNetworkAccessManager;
class QDockWidget;
class QDialog;
class QRestAccessManager;
class QAction;

namespace planner {
class PlanWidget;
class PlanTreeView;
class SettingsDialog;
class ShoppingSetupDialog;
class SearchesDialog;
class SnapshotsDialog;
class SnapshotModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    QDockWidget* plans_widget_poe1;
    PlanTreeView* plan_view_poe1;

    QDockWidget* plans_widget_poe2;
    PlanTreeView* plan_view_poe2;

    PlanTreeView* planView(Game game) const
    {
        return game == Game::Poe1 ? plan_view_poe1 : plan_view_poe2;
    }

    QNetworkAccessManager* network_manager;
    QRestAccessManager* rest_manager;

    QLineEdit* snapshot_edit;
    SnapshotModel* current_snapshot_model{};

    SearchesDialog* searches_dialog;
    SnapshotsDialog* snapshots_dialog;
    ShoppingSetupDialog* shopping_setup;
    SettingsDialog* settings_dialog;
    QDialog* about_dialog;

    void restoreSession();

    void raiseDock(Game game);

    QAction* save_action;
    QAction* save_all_action;
    QAction* plan_search_action;

#ifndef PLANNER_NO_BROWSER
    QAction* open_web_page_action;
#endif
    QAction* always_on_top_action;
    QAction* hide_descriptions_action;
    QAction* hide_empty_resources_action;
    QAction* hide_empty_results_action;
    QAction* hide_not_used_items_action;
    QAction* hide_title_currency_name_action;
    QAction* use_query_as_description_action;

    QAction* add_step_action;

    QAction* searches_action;
    QAction* searches_poe1_action;
    QAction* searches_poe2_action;
    QAction* snapshots_poe1_action;
    QAction* snapshots_poe2_action;
    QAction* update_cost_action;
    QAction* shopping_mode_action;

    QAction* import_text_action;
    QAction* import_file_action;
    QAction* settings_action;
    QAction* about_action;

    QAction* back_action;
    QAction* forward_action;

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void setAlwaysOnTop(bool checked);

    void cleanup();
    void importItem(bool from_clipboard);
    bool importItem(const QJsonDocument& json);
    void openShoppingDialog();
    void updateSnapshotName(const QModelIndex& idx);

private:
    void setupDockWidgets();
    void setupNetwork();
    void setupAboutDialog();
    void setupActions();

    PlanWidget* planWidget() const;

    bool haveUnsavedPlans() const;

    bool execSaveMsg();

    bool readFileForImport(const QString& file_path, QJsonDocument& json);
};

} // namespace planner
#endif // MAINWINDOW_H
