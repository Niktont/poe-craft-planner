#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "PlanModel.h"
#include <memory>
#include <QMainWindow>
#include <QMessageBox>
#include <QWebEngineProfile>

class QWebEngineView;
class QLineEdit;
class QNetworkAccessManager;
class QDockWidget;
class QTreeView;
class QToolBar;
class QDialog;
class QWebEngineProfile;
class QRestAccessManager;
class QAction;

namespace planner {

class TradeRequestManager;
class TradeRequestCache;
class ExchangeRequestManager;
class ExchangeRequestCache;
class PlanWidget;
class RequestEditDialog;
class PlanTreeView;
class UpdateCostDialog;
class SettingsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    QDockWidget* plans_widget_poe1;
    PlanTreeView* plan_view_poe1;

    QDockWidget* plans_widget_poe2;
    PlanTreeView* plan_view_poe2;

    QDialog* web_view_dialog;
    QWebEngineView* web_view;
    QLineEdit* url_edit;
    std::unique_ptr<QWebEngineProfile> web_profile;

    QNetworkAccessManager* network_manager;
    QRestAccessManager* rest_manager;

    TradeRequestManager* trade_manager;
    TradeRequestCache* trade_cache_poe1;
    TradeRequestCache* trade_cache_poe2;

    ExchangeRequestManager* exchange_manager;
    ExchangeRequestCache* exchange_cache_poe1;
    ExchangeRequestCache* exchange_cache_poe2;

    SettingsDialog* settings_dialog;
    RequestEditDialog* request_edit_dialog;
    UpdateCostDialog* update_cost_dialog;
    QDialog* about_dialog;

    PlanModel* plan_model_poe1;
    PlanModel* plan_model_poe2;

    PlanWidget* planWidget();

    void restoreLastPlan();

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

    QAction* open_web_page_action;
    QAction* always_on_top_action;
    QAction* hide_descriptions_action;

    QAction* add_step_action;

    QAction* manage_searches_action;
    QAction* manage_searches_poe1_action;
    QAction* manage_searches_poe2_action;
    QAction* update_cost_action;

    QAction* import_text_action;
    QAction* import_file_action;
    QAction* settings_action;
    QAction* about_action;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void setAlwaysOnTop(bool checked)
    {
        setWindowFlag(Qt::WindowStaysOnTopHint, checked);
        show();
    }

    void cleanup();
    void openRequestEdit();
    void importItem(bool from_clipboard);

private:
    void setupDockWidgets();
    void setupNetwork();
    void setupWebViewDialog();
    void setupAboutDialog();
    void setupActions();

    bool haveUnsavedPlans();

    bool execSaveMsg();
};

} // namespace planner
#endif // MAINWINDOW_H
