#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "PlanModel.h"

#include <QMainWindow>
#include <QMessageBox>

class QLineEdit;
class QNetworkAccessManager;
class QDockWidget;
class QTreeView;
class QToolBar;
class QDialog;
class QRestAccessManager;
class QAction;

namespace planner {
class CustomEditDialog;
class TradeRequestManager;
class TradeRequestCache;
class ExchangeRequestManager;
class ExchangeRequestCache;
class PlanWidget;
class RequestEditDialog;
class PlanTreeView;
class UpdateCostDialog;
class SettingsDialog;
class ShoppingDialog;
class ShoppingSetupDialog;
class SearchesDialog;
class PlanSearchDialog;
#ifndef PLANNER_NO_BROWSER
class WebViewDialog;
#endif

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    QDockWidget* plans_widget_poe1;
    PlanTreeView* plan_view_poe1;

    QDockWidget* plans_widget_poe2;
    PlanTreeView* plan_view_poe2;

#ifndef PLANNER_NO_BROWSER
    WebViewDialog* web_view_dialog;
#endif

    QNetworkAccessManager* network_manager;
    QRestAccessManager* rest_manager;

    TradeRequestManager* trade_manager;
    TradeRequestCache* trade_cache_poe1;
    TradeRequestCache* trade_cache_poe2;

    ExchangeRequestManager* exchange_manager;
    ExchangeRequestCache* exchange_cache_poe1;
    ExchangeRequestCache* exchange_cache_poe2;

    SettingsDialog* settings_dialog;
    SearchesDialog* searches_dialog;
    RequestEditDialog* request_edit_dialog;
    UpdateCostDialog* update_cost_dialog;
    ShoppingDialog* shopping_dialog;
    ShoppingSetupDialog* shopping_setup;

    CustomEditDialog* custom_edit_dialog;
    PlanSearchDialog* plan_search_dialog;

    QDialog* about_dialog;

    PlanModel* plan_model_poe1;
    PlanModel* plan_model_poe2;

    PlanWidget* planWidget();

    void restoreLastPlan();

    PlanTreeView* planView(Game game) const
    {
        return game == Game::Poe1 ? plan_view_poe1 : plan_view_poe2;
    }
    PlanModel* planModel(Game game) const
    {
        return game == Game::Poe1 ? plan_model_poe1 : plan_model_poe2;
    }
    TradeRequestCache* tradeCache(Game game) const
    {
        return game == Game::Poe1 ? trade_cache_poe1 : trade_cache_poe2;
    }
    ExchangeRequestCache* exchangeCache(Game game) const
    {
        return game == Game::Poe1 ? exchange_cache_poe1 : exchange_cache_poe2;
    }

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

    QAction* add_step_action;

    QAction* searches_action;
    QAction* searches_poe1_action;
    QAction* searches_poe2_action;
    QAction* update_cost_action;
    QAction* shopping_mode_action;

    QAction* import_text_action;
    QAction* import_file_action;
    QAction* settings_action;
    QAction* about_action;

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

private:
    void setupDockWidgets();
    void setupNetwork();
    void setupAboutDialog();
    void setupActions();

    bool haveUnsavedPlans();

    bool execSaveMsg();

    bool readFileForImport(const QString& file_path, QJsonDocument& json);
};

} // namespace planner
#endif // MAINWINDOW_H
