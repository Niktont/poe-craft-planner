#include "MainWindow.h"

#include "ExchangeRequestCache.h"
#include "ExchangeRequestManager.h"
#include "PlanTreeView.h"
#include "PlanWidget.h"
#include "RequestEditDialog.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "TradeRequestCache.h"
#include "TradeRequestManager.h"
#include "UpdateCostDialog.h"
#include "WebPage.h"
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QPushButton>
#include <QRestAccessManager>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QWebEngineProfileBuilder>
#include <QWebEngineSettings>
#include <QtWebEngineWidgets/QWebEngineView>

using namespace Qt::Literals;

namespace planner {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow{parent}
{
    auto settings = Settings::get();
    auto geometry = settings.value(Settings::windows_main_geometry);
    if (!geometry.isValid())
        setGeometry(200, 200, 980, 600);
    else
        restoreGeometry(geometry.toByteArray());

    auto plan_widget = new PlanWidget{*this};
    setCentralWidget(plan_widget);

    exchange_cache_poe1 = new ExchangeRequestCache{Game::Poe1, this};
    exchange_cache_poe2 = new ExchangeRequestCache{Game::Poe2, this};
    trade_cache_poe1 = new TradeRequestCache{Game::Poe1, *exchange_cache_poe1, this};
    trade_cache_poe2 = new TradeRequestCache{Game::Poe2, *exchange_cache_poe2, this};

    setupDockWidgets();

    setupNetwork();

    settings_dialog = new SettingsDialog{*this};
    request_edit_dialog = new RequestEditDialog{*this};
    update_cost_dialog = new UpdateCostDialog{*this};
    connect(update_cost_dialog,
            &UpdateCostDialog::costUpdated,
            plan_widget,
            &PlanWidget::updateCost);
    setupAboutDialog();

    plan_widget->connectSignals();

    setupActions();

    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::cleanup);

    hide_descriptions_action->setChecked(
        settings.value(Settings::windows_main_hide_descriptions, false).toBool());

    auto state = settings.value(Settings::windows_main_state, {});
    if (!state.isValid()) {
        plans_widget_poe1->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        plans_widget_poe1->setDockLocation(Qt::LeftDockWidgetArea);
        plans_widget_poe2->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        plans_widget_poe2->setDockLocation(Qt::LeftDockWidgetArea);

        tabifyDockWidget(plans_widget_poe1, plans_widget_poe2);
        resizeDocks({plans_widget_poe1}, {150}, Qt::Horizontal);
    } else
        restoreState(state.toByteArray());
}

PlanWidget* MainWindow::planWidget()
{
    return static_cast<PlanWidget*>(centralWidget());
}

void MainWindow::restoreLastPlan()
{
    auto settings = Settings::get();
    QUuid id{settings.value(Settings::windows_main_last_plan).toString()};
    if (id.isNull())
        return;

    auto it = plan_model_poe1->plans.find(id);
    if (it == plan_model_poe1->plans.end()) {
        it = plan_model_poe2->plans.find(id);
        if (it == plan_model_poe2->plans.end())
            return;
        plan_view_poe2->setCurrentIndex(it->second.item()->index());
    } else
        plan_view_poe1->setCurrentIndex(it->second.item()->index());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (haveUnsavedPlans() && !execSaveMsg()) {
        event->ignore();
        return;
    }

    auto settings = Settings::get();
    settings.setValue(Settings::windows_main_geometry, saveGeometry());
    settings.setValue(Settings::windows_main_state, saveState());
    settings.setValue(Settings::windows_web_view_dialog_geometry, web_view_dialog->saveGeometry());
    settings.setValue(Settings::windows_main_hide_descriptions,
                      hide_descriptions_action->isChecked());

    if (planWidget()->plan())
        settings.setValue(Settings::windows_main_last_plan, planWidget()->plan()->id().toString());

    event->accept();
}

void MainWindow::cleanup()
{
    web_view->page()->deleteLater();
}

void MainWindow::openRequestEdit()
{
    request_edit_dialog->openGame(planWidget()->game());
}

void MainWindow::importItem(bool from_clipboard)
{
    QJsonDocument json;

    if (from_clipboard) {
        auto text = qApp->clipboard()->text();
        json = {QJsonDocument::fromJson(text.toUtf8())};
    } else {
        auto filename = QFileDialog::getOpenFileName(this,
                                                     tr("Import"),
                                                     {},
                                                     tr("JSON file (*.json)"));
        if (filename.isEmpty())
            return;

        QFile file{filename};
        if (file.open(QFile::ReadOnly))
            json = {QJsonDocument::fromJson(file.readAll())};
        else {
            QMessageBox msg;
            msg.setWindowTitle(tr("Import (File)"));
            msg.setText(tr("Failed to read file \"%1\".").arg(filename));
            msg.exec();
        }
    }

    const auto export_o = json.object();
    auto game = gamefromStr(export_o["game"].toString());
    auto folder_v = export_o["folder"];
    auto plan_v = export_o["plan"];
    auto requests_v = export_o["trade_requests"];
    if (game == Game::Unknown || requests_v.isUndefined()
        || (folder_v.isUndefined() && plan_v.isUndefined())) {
        QMessageBox msg;
        msg.setWindowTitle(tr("Import Failed"));
        msg.setText(tr("Unrecognized format."));
        msg.exec();
        return;
    }

    planModel(game)->importItem(export_o);
}

void MainWindow::setupDockWidgets()
{
    plans_widget_poe1 = new QDockWidget{this};
    plans_widget_poe1->setObjectName("poe1_dock");
    plans_widget_poe1->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    plans_widget_poe1->setWindowTitle(tr("PoE 1"));

    plan_model_poe1 = new PlanModel{Game::Poe1, *this};

    plan_view_poe1 = new PlanTreeView{*plan_model_poe1, this};
    plans_widget_poe1->setWidget(plan_view_poe1);

    plans_widget_poe2 = new QDockWidget{this};
    plans_widget_poe2->setObjectName("poe2_dock");
    plans_widget_poe2->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    plans_widget_poe2->setWindowTitle(tr("PoE 2"));

    plan_model_poe2 = new PlanModel{Game::Poe2, *this};

    plan_view_poe2 = new PlanTreeView{*plan_model_poe2, this};
    plans_widget_poe2->setWidget(plan_view_poe2);

    setDockOptions(ForceTabbedDocks | AnimatedDocks | GroupedDragging);
}

void MainWindow::setupNetwork()
{
    network_manager = new QNetworkAccessManager{this};
    network_manager->setAutoDeleteReplies(true);
    rest_manager = new QRestAccessManager{network_manager, this};

    setupWebViewDialog();

    trade_manager = new TradeRequestManager{*rest_manager, *web_view, this};
    exchange_manager = new ExchangeRequestManager{*rest_manager, *web_view, *this};
}

void MainWindow::setupWebViewDialog()
{
    const QString name = u"profile."_s + QLatin1StringView(qWebEngineChromiumVersion());
    QWebEngineProfileBuilder profileBuilder;
    web_profile.reset(profileBuilder.createProfile(name));
    web_profile->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    web_profile->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    web_profile->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
    web_profile->setHttpUserAgent(
        u"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "_s
        % u"PoeCraftPlanner/"_s % APP_VERSION % u" Chrome/134.0.0.0 Safari/537.36"_s);

    web_view = new QWebEngineView{};
    web_view->setPage(new WebPage{web_profile.get(), web_view});
    web_view->setSizePolicy(
        QSizePolicy{QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding});

    url_edit = new QLineEdit{};

    connect(url_edit, &QLineEdit::returnPressed, this, [this] {
        web_view->setUrl(QUrl::fromUserInput(url_edit->text()));
    });
    connect(web_view, &QWebEngineView::urlChanged, this, [this](const QUrl& url) {
        url_edit->setText(url.toDisplayString());
    });

    auto button = new QPushButton{tr("Clear cookies")};
    connect(button, &QPushButton::clicked, this, [this] {
        web_profile->cookieStore()->deleteAllCookies();
    });

    web_view_dialog = new QDialog{this};
    web_view_dialog->setWindowTitle(tr("Web Page"));
    auto settings = Settings::get();
    auto geometry = settings.value(Settings::windows_web_view_dialog_geometry);
    if (!geometry.isValid())
        web_view_dialog->resize(900, 800);
    else
        web_view_dialog->restoreGeometry(geometry.toByteArray());

    auto dialog_layout = new QVBoxLayout{};
    web_view_dialog->setLayout(dialog_layout);

    connect(web_view->page(),
            &QWebEnginePage::titleChanged,
            web_view_dialog,
            &QDialog::setWindowTitle);

    auto url_layout = new QHBoxLayout{};
    url_layout->addWidget(url_edit, 1);
    url_layout->addWidget(button);

    dialog_layout->addLayout(url_layout);
    dialog_layout->addWidget(web_view);

    button->setAutoDefault(false);

    connect(web_profile->cookieStore(),
            &QWebEngineCookieStore::cookieAdded,
            network_manager,
            [this](const QNetworkCookie& cookie) {
                if (cookie.name() != "POESESSID")
                    return;

                network_manager->cookieJar()->insertCookie(cookie);
            });
    connect(web_profile->cookieStore(),
            &QWebEngineCookieStore::cookieRemoved,
            network_manager,
            [this](const QNetworkCookie& cookie) {
                if (cookie.name() != "POESESSID")
                    return;
                network_manager->cookieJar()->deleteCookie(cookie);
            });

    web_profile->cookieStore()->loadAllCookies();

    web_view->load({""});
}

void MainWindow::setupAboutDialog()
{
    about_dialog = new QDialog{this};
    about_dialog->setFixedWidth(500);
    about_dialog->setWindowTitle(tr("About"));

    about_dialog->setLayout(new QVBoxLayout{});
    auto label = new QLabel{};
    label->setWordWrap(true);
    label->setOpenExternalLinks(true);
    label->setText(
        R"(
<p>PoE Craft Planner is an open source, free software used to create plans for crafting or farming in PoE 1 and 2, it uses price data from trade website and poe.ninja. This product isn't affiliated with or endorsed by Grinding Gear Games in any way.</p>
<p>It is licensed under the GNU General Public License Version 3 or later. You can modify or redistribute it under the conditions of this license. See <a href="https://www.gnu.org/licenses/gpl.html"><span style=" text-decoration: underline; color:#0000EE;">https://www.gnu.org/licenses/gpl.html</span></a> for details.</p>
<p>You can find source code at <a href="https://github.com/Niktont/poe-craft-planner"><span style=" text-decoration: underline; color:#0000EE;">https://github.com/Niktont/poe-craft-planner</span></a></p>
<p>This software uses the GPL/LGPL Qt Toolkit from <a href="https://qt-project.org"><span style=" text-decoration: underline; color:#0000EE;">https://qt-project.org</span></a>. See <a href="https://doc.qt.io/qt-6/licensing.html"><span style=" text-decoration: underline; color:#0000EE;">https://doc.qt.io/qt-6/licensing.html</span></a> for licensing terms and information.</p>
<p>It also uses the Boost libraries from <a href="https://www.boost.org"><span style=" text-decoration: underline; color:#0000EE;">https://www.boost.org</span></a>.</p>)");

    about_dialog->layout()->addWidget(label);
}

void MainWindow::setupActions()
{
    save_action = new QAction{tr("Save"), this};
    save_action->setShortcut(QKeySequence::Save);
    connect(save_action, &QAction::triggered, this, [this] {
        if (auto plan = planWidget()->plan()) {
            planModel(plan->game)->savePlan(plan->item());
        }
    });

    save_all_action = new QAction{tr("Save All"), this};
    save_all_action->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_S);
    connect(save_all_action, &QAction::triggered, this, [this] {
        plan_model_poe1->saveAllPlans();
        plan_model_poe2->saveAllPlans();
    });

    open_web_page_action = new QAction{tr("Open Web Page"), this};
    connect(open_web_page_action, &QAction::triggered, web_view_dialog, &QDialog::show);

    always_on_top_action = new QAction{tr("Always On Top"), this};
    always_on_top_action->setCheckable(true);
    connect(always_on_top_action, &QAction::toggled, this, &MainWindow::setAlwaysOnTop);

    hide_descriptions_action = new QAction{tr("Hide Descriptions"), this};
    hide_descriptions_action->setShortcut(Qt::ALT | Qt::Key_D);
    hide_descriptions_action->setCheckable(true);
    connect(hide_descriptions_action,
            &QAction::toggled,
            planWidget(),
            &PlanWidget::hideDescriptions);

    add_step_action = new QAction{tr("Add Step"), this};
    connect(add_step_action, &QAction::triggered, planWidget(), &PlanWidget::addStep);

    manage_searches_action = new QAction{tr("Manage Searches"), this};
    connect(manage_searches_action, &QAction::triggered, this, &MainWindow::openRequestEdit);

    manage_searches_poe1_action = new QAction{tr("Manage Searches (PoE 1)"), this};
    connect(manage_searches_poe1_action, &QAction::triggered, this, [this] {
        request_edit_dialog->openGame(Game::Poe1);
    });
    manage_searches_poe2_action = new QAction{tr("Manage Searches (PoE 2)"), this};
    connect(manage_searches_poe2_action, &QAction::triggered, this, [this] {
        request_edit_dialog->openGame(Game::Poe2);
    });

    update_cost_action = new QAction{tr("Update Costs"), this};
    update_cost_action->setShortcut(Qt::Key_F5);
    connect(update_cost_action, &QAction::triggered, this, [this] {
        update_cost_dialog->updatePlan(planWidget()->plan());
    });

    import_text_action = new QAction{tr("Import (Clipboard)"), this};
    connect(import_text_action, &QAction::triggered, this, [this] { importItem(true); });
    import_file_action = new QAction{tr("Import (File)"), this};
    connect(import_file_action, &QAction::triggered, this, [this] { importItem(false); });

    auto file_menu = menuBar()->addMenu(tr("File"));
    file_menu->addAction(save_action);
    file_menu->addAction(save_all_action);
    file_menu->addSeparator();
    file_menu->addAction(import_text_action);
    file_menu->addAction(import_file_action);

    auto edit_menu = menuBar()->addMenu(tr("Edit"));
    edit_menu->addAction(manage_searches_poe1_action);
    edit_menu->addAction(manage_searches_poe2_action);
    edit_menu->addSeparator();
    edit_menu->addAction(update_cost_action);

    auto view_menu = menuBar()->addMenu(tr("View"));
    view_menu->addAction(open_web_page_action);
    view_menu->addAction(always_on_top_action);
    view_menu->addAction(hide_descriptions_action);

    settings_action = menuBar()->addAction(tr("Settings"));
    connect(settings_action, &QAction::triggered, settings_dialog, &SettingsDialog::openSettings);

    about_action = menuBar()->addAction(tr("About"));
    connect(about_action, &QAction::triggered, about_dialog, &QDialog::open);

    auto toolbar = addToolBar(tr("Toolbar"));
    toolbar->setObjectName("toolbar");
    toolbar->addAction(save_action);
    toolbar->addAction(save_all_action);
    toolbar->addSeparator();

    toolbar->addAction(add_step_action);
    toolbar->addAction(manage_searches_action);
    toolbar->addAction(update_cost_action);
}

bool MainWindow::haveUnsavedPlans()
{
    return plan_model_poe1->haveUnsavedPlans() || plan_model_poe2->haveUnsavedPlans();
}

bool MainWindow::execSaveMsg()
{
    int ret = QMessageBox::warning(this,
                                   tr("Save Changes"),
                                   tr("Save changed plans?"),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Save:
        plan_model_poe1->saveAllPlans();
        plan_model_poe2->saveAllPlans();
        return true;
    case QMessageBox::Discard:
        return true;
    case QMessageBox::Cancel:
        return false;
    }
    return false;
}

} // namespace planner
