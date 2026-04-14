#include "MainWindow.h"

#include "AppState.h"
#include "CustomEditDialog.h"
#include "ExchangeRequestCache.h"
#include "ExchangeRequestManager.h"
#include "Plan.h"
#include "PlanItem.h"
#include "PlanModel.h"
#include "PlanSearchDialog.h"
#include "PlanSearchView.h"
#include "PlanTreeView.h"
#include "PlanWidget.h"
#include "RequestEditDialog.h"
#include "SearchesDialog.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "ShoppingDialog.h"
#include "ShoppingSetupDialog.h"
#include "SnapshotModel.h"
#include "SnapshotsDialog.h"
#include "TradeRequestCache.h"
#include "TradeRequestManager.h"
#include "UpdateCostDialog.h"
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDialog>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QRestAccessManager>
#include <QToolBar>
#include <QVBoxLayout>
#ifndef PLANNER_NO_BROWSER
#include "WebViewDialog.h"
#endif

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

    AppState::state.mw = this;

    setAcceptDrops(true);

    auto plan_widget = new PlanWidget{*this};
    setCentralWidget(plan_widget);

    AppState::state.exchange_cache_poe1 = new ExchangeRequestCache{Game::Poe1, this};
    AppState::state.exchange_cache_poe2 = new ExchangeRequestCache{Game::Poe2, this};
    AppState::state.trade_cache_poe1 = new TradeRequestCache{Game::Poe1,
                                                             *AppState::state.exchange_cache_poe1,
                                                             this};
    AppState::state.trade_cache_poe2 = new TradeRequestCache{Game::Poe2,
                                                             *AppState::state.exchange_cache_poe2,
                                                             this};

    AppState::state.snapshots_poe1 = new SnapshotModel{*AppState::state.exchange_cache_poe1,
                                                       *AppState::state.trade_cache_poe1,
                                                       this};
    AppState::state.snapshots_poe2 = new SnapshotModel{*AppState::state.exchange_cache_poe2,
                                                       *AppState::state.trade_cache_poe2,
                                                       this};

    setupDockWidgets();

    setupNetwork();

    settings_dialog = new SettingsDialog{this};
    searches_dialog = new SearchesDialog{this};
    snapshots_dialog = new SnapshotsDialog{this};
    AppState::state.request_edit_dialog = new RequestEditDialog{this};
    AppState::state.update_cost_dialog = new UpdateCostDialog{this};
    connect(AppState::state.update_cost_dialog,
            &UpdateCostDialog::costUpdated,
            plan_widget,
            &PlanWidget::updateCost);
    AppState::state.shopping_dialog = new ShoppingDialog{this};
    shopping_setup = new ShoppingSetupDialog{this};
    AppState::state.custom_edit_dialog = new CustomEditDialog{this};
    AppState::state.plan_search_dialog = new PlanSearchDialog{this};
    connect(AppState::state.plan_search_dialog->view,
            &PlanSearchView::planClicked,
            plan_widget,
            &PlanWidget::openPlan);

    snapshot_edit = new QLineEdit{};
    snapshot_edit->setPlaceholderText(tr("Snapshot"));
    snapshot_edit->setFixedWidth(80);
    connect(plan_widget, &PlanWidget::gameChanged, this, [this](Game game) {
        current_snapshot_model = AppState::snapshots(game);
        snapshot_edit->setText(current_snapshot_model->currentName());
        snapshot_edit->setCompleter(current_snapshot_model->completer);
    });
    connect(AppState::state.snapshots_poe1,
            &SnapshotModel::dataChanged,
            this,
            &MainWindow::updateSnapshotName);
    connect(AppState::state.snapshots_poe2,
            &SnapshotModel::dataChanged,
            this,
            &MainWindow::updateSnapshotName);

    connect(snapshot_edit, &QLineEdit::editingFinished, this, [this] {
        if (!current_snapshot_model)
            return;

        if (snapshot_edit->text().isEmpty())
            current_snapshot_model->clearCurrent();
        else
            snapshot_edit->setText(current_snapshot_model->currentName());
    });
    connect(snapshot_edit, &QLineEdit::textChanged, this, [this](const QString& text) {
        auto fm = snapshot_edit->fontMetrics();
        auto width = fm.horizontalAdvance(text) + 15;
        snapshot_edit->setFixedWidth(std::max(80, width));
    });

    setupAboutDialog();

    plan_widget->connectSignals();

    setupActions();

    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::cleanup);

    hide_descriptions_action->setChecked(Settings::get<Settings::windows_main_hide_descriptions>());
    hide_empty_resources_action->setChecked(
        Settings::get<Settings::windows_main_hide_empty_resources>());
    hide_empty_results_action->setChecked(
        Settings::get<Settings::windows_main_hide_empty_results>());
    hide_not_used_items_action->setChecked(
        Settings::get<Settings::windows_main_hide_not_used_items>());
    hide_title_currency_name_action->setChecked(
        Settings::get<settings::windows_main_hide_title_currency_name>());
    use_query_as_description_action->setChecked(
        Settings::get<settings::trade_use_query_as_description>());

    auto state = settings.value(Settings::windows_main_state, {});
    if (!state.isValid()) {
        plans_widget_poe1->setDockLocation(Qt::LeftDockWidgetArea);
        plans_widget_poe2->setDockLocation(Qt::LeftDockWidgetArea);

        tabifyDockWidget(plans_widget_poe1, plans_widget_poe2);
        resizeDocks({plans_widget_poe1}, {150}, Qt::Horizontal);
    } else
        restoreState(state.toByteArray());
}

PlanWidget* MainWindow::planWidget() const
{
    return static_cast<PlanWidget*>(centralWidget());
}

void MainWindow::restoreSession()
{
    auto settings = Settings::get();
    AppState::state.snapshots_poe1->setCurrent(
        QUuid::fromString(settings.value(Settings::windows_main_snapshot_poe1).toString()));
    AppState::state.snapshots_poe2->setCurrent(
        QUuid::fromString(settings.value(Settings::windows_main_snapshot_poe2).toString()));

    QUuid id{settings.value(Settings::windows_main_last_plan).toString()};
    if (id.isNull()) {
        plan_view_poe1->setCurrentIndex({});
        plan_view_poe2->setCurrentIndex({});
        return;
    }

    auto it = AppState::state.plan_model_poe1->plans.find(id);
    if (it == AppState::state.plan_model_poe1->plans.end()) {
        it = AppState::state.plan_model_poe2->plans.find(id);
        if (it == AppState::state.plan_model_poe2->plans.end()) {
            plan_view_poe1->setCurrentIndex({});
            plan_view_poe2->setCurrentIndex({});
            return;
        }
        plan_view_poe2->setCurrentIndex(it->second.item()->index());
        plans_widget_poe2->raise();
    } else {
        plan_view_poe1->setCurrentIndex(it->second.item()->index());
        plans_widget_poe1->raise();
    }
}

void MainWindow::raiseDock(Game game)
{
    game == Game::Poe1 ? plans_widget_poe1->raise() : plans_widget_poe2->raise();
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
#ifndef PLANNER_NO_BROWSER
    settings.setValue(Settings::windows_web_view_dialog_geometry,
                      AppState::state.web_view_dialog->saveGeometry());
#endif
    settings.setValue(Settings::windows_shopping_dialog_geometry,
                      AppState::state.shopping_dialog->saveGeometry());
    searches_dialog->saveState(settings);
    snapshots_dialog->saveState(settings);
    settings.setValue(Settings::windows_request_edit_dialog_size,
                      AppState::state.request_edit_dialog->size());
    settings.setValue(Settings::windows_plan_search_dialog_size,
                      AppState::state.plan_search_dialog->size());

    settings.setValue(Settings::windows_main_snapshot_poe1,
                      AppState::state.snapshots_poe1->current
                          ? AppState::state.snapshots_poe1->current->id.toString()
                          : QString{});
    settings.setValue(Settings::windows_main_snapshot_poe2,
                      AppState::state.snapshots_poe2->current
                          ? AppState::state.snapshots_poe2->current->id.toString()
                          : QString{});

    Settings::save<settings::windows_main_hide_descriptions>(settings);
    Settings::save<settings::windows_main_hide_empty_resources>(settings);
    Settings::save<settings::windows_main_hide_empty_results>(settings);
    Settings::save<settings::windows_main_hide_not_used_items>(settings);
    Settings::save<settings::windows_main_hide_title_currency_name>(settings);
    Settings::save<settings::trade_use_query_as_description>(settings);

    settings.setValue(Settings::windows_main_last_plan,
                      planWidget()->plan() ? planWidget()->plan()->id().toString() : QString{});

    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    auto urls = event->mimeData()->urls();
    if (urls.empty())
        return;

    auto& url = urls.front();
    if (!url.isLocalFile() || !url.fileName().endsWith(".json"))
        return;

    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    auto urls = event->mimeData()->urls();
    auto& url = urls.front();
    QJsonDocument json;
    if (!readFileForImport(url.toLocalFile(), json)) {
        event->ignore();
        return;
    }

    if (!importItem(json)) {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
}

void MainWindow::setAlwaysOnTop(bool checked)
{
    auto setStaysOnTop = [](bool checked, QWidget* window) {
        bool shown = !window->isHidden();
        window->setWindowFlag(Qt::WindowStaysOnTopHint, checked);
        if (shown)
            window->show();
    };
    setStaysOnTop(checked, this);
    setStaysOnTop(checked, AppState::state.shopping_dialog);
}

void MainWindow::cleanup()
{
#ifndef PLANNER_NO_BROWSER
    AppState::state.web_view_dialog->cleanup();
#endif
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
        if (filename.isEmpty() || !readFileForImport(filename, json))
            return;
    }

    importItem(json);
}

bool MainWindow::importItem(const QJsonDocument& json)
{
    const auto export_o = json.object();
    auto game = gamefromStr(export_o["game"].toString());
    auto folder_v = export_o["folder"];
    auto plan_v = export_o["plan"];
    auto requests_v = export_o["trade_requests"];
    if (game == Game::Unknown || requests_v.isUndefined()
        || (folder_v.isUndefined() && plan_v.isUndefined())) {
        auto msg = new QMessageBox{this};
        msg->setAttribute(Qt::WA_DeleteOnClose);
        msg->setWindowTitle(tr("Import Failed"));
        msg->setText(tr("Unrecognized format."));
        msg->open();
        return false;
    }

    return AppState::planModel(game)->importItem(export_o);
}

void MainWindow::openShoppingDialog()
{
    auto plan = planWidget()->plan();
    if (!plan || plan->steps.empty())
        return;

    bool have_resources = false;
    for (auto& step : plan->steps) {
        if (!step.resources.empty()) {
            have_resources = true;
            break;
        }
    }
    if (!have_resources)
        return;

    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
        shopping_setup->openPlan(*plan);
    else
        AppState::state.shopping_dialog->openPlan(*plan);
}

void MainWindow::updateSnapshotName(const QModelIndex& idx)
{
    if (!current_snapshot_model)
        return;
    auto changed_model = qobject_cast<const SnapshotModel*>(idx.model());
    if (changed_model != current_snapshot_model)
        return;

    auto changed_it = changed_model->snapshots.nth(idx.row());
    if (changed_model->current && changed_model->current->id == changed_it->first)
        snapshot_edit->setText(changed_it->second.name);
}

void MainWindow::setupDockWidgets()
{
    plans_widget_poe1 = new QDockWidget{this};
    plans_widget_poe1->setObjectName("poe1_dock");
    plans_widget_poe1->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    plans_widget_poe1->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    plans_widget_poe1->setWindowTitle(tr("PoE 1"));

    AppState::state.plan_model_poe1 = new PlanModel{Game::Poe1, this};
    plan_view_poe1 = new PlanTreeView{*AppState::state.plan_model_poe1, this};
    plans_widget_poe1->setWidget(plan_view_poe1);

    plans_widget_poe2 = new QDockWidget{this};
    plans_widget_poe2->setObjectName("poe2_dock");
    plans_widget_poe2->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    plans_widget_poe2->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    plans_widget_poe2->setWindowTitle(tr("PoE 2"));

    AppState::state.plan_model_poe2 = new PlanModel{Game::Poe2, this};
    plan_view_poe2 = new PlanTreeView{*AppState::state.plan_model_poe2, this};
    plans_widget_poe2->setWidget(plan_view_poe2);

    setDockOptions(ForceTabbedDocks | AnimatedDocks | GroupedDragging);
}

void MainWindow::setupNetwork()
{
    QString user_agent{
        u"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "_s
        % u"PoeCraftPlanner/"_s % APP_VERSION % u" Chrome/134.0.0.0 Safari/537.36"_s};

    network_manager = new QNetworkAccessManager{this};
    network_manager->setAutoDeleteReplies(true);
    rest_manager = new QRestAccessManager{network_manager, this};

#ifndef PLANNER_NO_BROWSER
    AppState::state.web_view_dialog = new WebViewDialog{user_agent, this};
#endif

    AppState::state.trade_manager = new TradeRequestManager{*rest_manager, user_agent, this};
    AppState::state.exchange_manager = new ExchangeRequestManager{*rest_manager, user_agent, this};
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
<p>You can find source code at <a href="https://github.com/Niktont/poe-craft-planner"><span style=" text-decoration: underline; color:#0000EE;">https://github.com/Niktont/poe-craft-planner</span></a>.</p>
<p>This software uses the GPL/LGPL Qt Toolkit from <a href="https://qt-project.org"><span style=" text-decoration: underline; color:#0000EE;">https://qt-project.org</span></a>. See <a href="https://doc.qt.io/qt-6/licensing.html"><span style=" text-decoration: underline; color:#0000EE;">https://doc.qt.io/qt-6/licensing.html</span></a> for licensing terms and information.</p>
<p>It also uses the Boost libraries from <a href="https://www.boost.org"><span style=" text-decoration: underline; color:#0000EE;">https://www.boost.org</span></a>, keyboard-auto-type from <a href="https://github.com/antelle/keyboard-auto-type"><span style=" text-decoration: underline; color:#0000EE;">https://github.com/antelle/keyboard-auto-type</span></a> and QHotkey from <a href="https://github.com/Skycoder42/QHotkey"><span style=" text-decoration: underline; color:#0000EE;">https://github.com/Skycoder42/QHotkey</span></a>.</p>)");

    about_dialog->layout()->addWidget(label);
}

void MainWindow::setupActions()
{
    save_action = new QAction{tr("Save"), this};
    save_action->setShortcut(QKeySequence::Save);
    connect(save_action, &QAction::triggered, this, [this] {
        if (auto plan = planWidget()->plan()) {
            AppState::planModel(plan->game)->savePlan(*plan->item());
        }
    });

    save_all_action = new QAction{tr("Save All"), this};
    save_all_action->setShortcut(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_S);
    connect(save_all_action, &QAction::triggered, this, [] {
        AppState::state.plan_model_poe1->saveAllPlans();
        AppState::state.plan_model_poe2->saveAllPlans();
    });

    plan_search_action = new QAction{tr("Find Plan")};
    plan_search_action->setShortcut(Qt::ControlModifier | Qt::Key_F);
    connect(plan_search_action, &QAction::triggered, this, [this] {
        AppState::state.plan_search_dialog->openGame(planWidget()->game());
    });

#ifndef PLANNER_NO_BROWSER
    open_web_page_action = new QAction{tr("Open Web Page"), this};
    connect(open_web_page_action,
            &QAction::triggered,
            AppState::state.web_view_dialog,
            &QDialog::show);
#endif

    always_on_top_action = new QAction{tr("Always On Top"), this};
    always_on_top_action->setCheckable(true);
    connect(always_on_top_action, &QAction::toggled, this, &MainWindow::setAlwaysOnTop);

    hide_descriptions_action = new QAction{tr("Hide Descriptions"), this};
    hide_descriptions_action->setShortcut(Qt::AltModifier | Qt::Key_D);
    hide_descriptions_action->setCheckable(true);
    connect(hide_descriptions_action,
            &QAction::toggled,
            planWidget(),
            &PlanWidget::hideDescriptions);

    hide_empty_resources_action = new QAction{tr("Hide Empty Resources"), this};
    hide_empty_resources_action->setCheckable(true);
    connect(hide_empty_resources_action,
            &QAction::toggled,
            planWidget(),
            &PlanWidget::hideEmptyResources);

    hide_empty_results_action = new QAction{tr("Hide Empty Results"), this};
    hide_empty_results_action->setCheckable(true);
    connect(hide_empty_results_action,
            &QAction::toggled,
            planWidget(),
            &PlanWidget::hideEmptyResults);

    hide_not_used_items_action = new QAction{tr("Hide Unused Items"), this};
    hide_not_used_items_action->setShortcut(Qt::AltModifier | Qt::Key_I);
    hide_not_used_items_action->setCheckable(true);
    connect(hide_not_used_items_action,
            &QAction::toggled,
            planWidget(),
            &PlanWidget::hideNotUsedItems);

    use_query_as_description_action = new QAction{tr("Use Search Query As Description"), this};
    use_query_as_description_action->setToolTip(
        tr("Parsed search query will be shown in the tooltip of Name cell for Trade items"));
    use_query_as_description_action->setCheckable(true);
    connect(use_query_as_description_action, &QAction::triggered, this, [](bool checked) {
        Settings::setCache<Settings::trade_use_query_as_description>(checked);
    });

    back_action = new QAction{style()->standardIcon(QStyle::SP_ArrowLeft), tr("Go Back"), this};
    back_action->setShortcut(Qt::AltModifier | Qt::Key_Left);
    connect(back_action, &QAction::triggered, planWidget(), &PlanWidget::goBack);

    forward_action = new QAction{style()->standardIcon(QStyle::SP_ArrowRight),
                                 tr("Go Forward"),
                                 this};
    forward_action->setShortcut(Qt::AltModifier | Qt::Key_Right);
    connect(forward_action, &QAction::triggered, planWidget(), &PlanWidget::goForward);

    hide_title_currency_name_action = new QAction{tr("Hide Currency Name In Titles"), this};
    hide_title_currency_name_action->setCheckable(true);
    connect(hide_title_currency_name_action,
            &QAction::toggled,
            planWidget(),
            &PlanWidget::hideTitleCurrencyName);

    add_step_action = new QAction{tr("Add Step"), this};
    connect(add_step_action, &QAction::triggered, planWidget(), &PlanWidget::addStep);

    searches_action = new QAction{tr("Searches"), this};
    connect(searches_action, &QAction::triggered, this, [this] {
        searches_dialog->openGame(planWidget()->game());
    });

    searches_poe1_action = new QAction{tr("Searches (PoE 1)"), this};
    connect(searches_poe1_action, &QAction::triggered, this, [this] {
        searches_dialog->openGame(Game::Poe1);
    });
    searches_poe2_action = new QAction{tr("Searches (PoE 2)"), this};
    connect(searches_poe2_action, &QAction::triggered, this, [this] {
        searches_dialog->openGame(Game::Poe2);
    });

    snapshots_poe1_action = new QAction{tr("Snapshots (PoE 1)"), this};
    connect(snapshots_poe1_action, &QAction::triggered, this, [this] {
        snapshots_dialog->openGame(Game::Poe1);
    });
    snapshots_poe2_action = new QAction{tr("Snapshots (PoE 2)"), this};
    connect(snapshots_poe2_action, &QAction::triggered, this, [this] {
        snapshots_dialog->openGame(Game::Poe2);
    });

    update_cost_action = new QAction{tr("Update Costs"), this};
    update_cost_action->setShortcuts({Qt::Key_F5, Qt::ShiftModifier | Qt::Key_F5});
    connect(update_cost_action, &QAction::triggered, this, [this] {
        auto modifiers = QGuiApplication::keyboardModifiers();
        bool send_requests = !Settings::offline_mode && !modifiers.testFlag(Qt::ShiftModifier);
        AppState::state.update_cost_dialog->updatePlan(planWidget()->plan(), send_requests);
    });

    shopping_mode_action = new QAction{tr("Shopping Mode"), this};
    connect(shopping_mode_action, &QAction::triggered, this, &MainWindow::openShoppingDialog);

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
    edit_menu->addAction(searches_poe1_action);
    edit_menu->addAction(searches_poe2_action);
    edit_menu->addSeparator();
    edit_menu->addAction(snapshots_poe1_action);
    edit_menu->addAction(snapshots_poe2_action);
    edit_menu->addAction(update_cost_action);
    edit_menu->addSeparator();
    edit_menu->addAction(plan_search_action);

    auto view_menu = menuBar()->addMenu(tr("View"));
#ifndef PLANNER_NO_BROWSER
    view_menu->addAction(open_web_page_action);
#endif
    view_menu->addAction(always_on_top_action);
    view_menu->addSeparator();
    view_menu->addAction(hide_descriptions_action);
    view_menu->addAction(hide_empty_resources_action);
    view_menu->addAction(hide_empty_results_action);
    view_menu->addAction(hide_not_used_items_action);
    view_menu->addAction(hide_title_currency_name_action);
    view_menu->addSeparator();
    view_menu->addAction(use_query_as_description_action);

    settings_action = menuBar()->addAction(tr("Settings"));
    connect(settings_action, &QAction::triggered, settings_dialog, &SettingsDialog::openSettings);

    about_action = menuBar()->addAction(tr("About"));
    connect(about_action, &QAction::triggered, about_dialog, &QDialog::open);

    auto toolbar = addToolBar(tr("Toolbar"));
    toolbar->setIconSize({16, 16});

    toolbar->setObjectName("toolbar");
    toolbar->addAction(back_action);
    toolbar->addAction(forward_action);
    toolbar->addSeparator();
    toolbar->addAction(save_action);
    toolbar->addAction(save_all_action);
    toolbar->addSeparator();
    toolbar->addAction(add_step_action);
    toolbar->addAction(searches_action);
    toolbar->addSeparator();
    toolbar->addAction(update_cost_action);
    toolbar->addAction(shopping_mode_action);
    toolbar->addWidget(snapshot_edit);
}

bool MainWindow::haveUnsavedPlans() const
{
    return AppState::state.plan_model_poe1->haveUnsavedPlans()
           || AppState::state.plan_model_poe2->haveUnsavedPlans();
}

bool MainWindow::execSaveMsg()
{
    int ret = QMessageBox::warning(this,
                                   tr("Save Changes"),
                                   tr("Save changed plans?"),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Save:
        AppState::state.plan_model_poe1->saveAllPlans();
        AppState::state.plan_model_poe2->saveAllPlans();
        return true;
    case QMessageBox::Discard:
        return true;
    case QMessageBox::Cancel:
        return false;
    }
    return false;
}

bool MainWindow::readFileForImport(const QString& file_path, QJsonDocument& json)
{
    QFile file{file_path};
    if (file.open(QFile::ReadOnly)) {
        json = {QJsonDocument::fromJson(file.readAll())};
        return true;
    }

    auto msg = new QMessageBox{this};
    msg->setAttribute(Qt::WA_DeleteOnClose);
    msg->setWindowTitle(tr("Import (File)"));
    msg->setText(tr("Failed to read file \"%1\".").arg(file_path));
    msg->open();
    return false;
}

} // namespace planner
