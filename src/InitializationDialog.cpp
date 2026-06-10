#include "InitializationDialog.h"
#include "AppState.h"
#include "Database.h"
#include "ExchangeRequestCache.h"
#include "ExchangeRequestManager.h"
#include "ExchangeRequester.h"
#include "MainWindow.h"
#include "PlanModel.h"
#include "Settings.h"
#include "SnapshotModel.h"
#include "TradeRequestCache.h"
#include <boost/range/algorithm.hpp>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRestReply>
#include <QStringBuilder>
#include <QTimer>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace planner {

InitializationDialog::InitializationDialog(MainWindow& mw)
    : QDialog{}
    , mw{&mw}
{
    auto main_layout = new QVBoxLayout{};
    main_layout->setVerticalSizeConstraint(QLayout::SetFixedSize);
    setLayout(main_layout);
    setMinimumWidth(300);

    progress_label = new QLabel{this};
    progress_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    progress_label->setWordWrap(true);
    layout()->addWidget(progress_label);

    auto buttons_layout = new QHBoxLayout{};
    main_layout->addLayout(buttons_layout);

    offline_button = new QPushButton{tr("Offline mode"), this};
    offline_button->setEnabled(false);
    connect(offline_button, &QPushButton::clicked, this, [this] {
        Settings::offline_mode = true;
        finishInitialization();
    });
    buttons_layout->addWidget(offline_button);

    continue_button = new QPushButton{tr("Continue"), this};
    continue_button->setEnabled(false);
    connect(continue_button, &QPushButton::clicked, this, &InitializationDialog::updateCacheData);
    buttons_layout->addWidget(continue_button);

    select_league_widget = new QWidget{this};
    select_league_widget->setLayout(new QVBoxLayout{});
    layout()->addWidget(select_league_widget);
    select_league_widget->hide();

    select_league_widget->layout()->addWidget(new QLabel{tr("Select league for PoE 1:")});
    league_combo_poe1 = new QComboBox{select_league_widget};
    league_combo_poe1->setEnabled(false);
    select_league_widget->layout()->addWidget(league_combo_poe1);

    select_league_widget->layout()->addWidget(new QLabel{tr("Select league for PoE 2:")});
    league_combo_poe2 = new QComboBox{select_league_widget};
    league_combo_poe2->setEnabled(false);
    select_league_widget->layout()->addWidget(league_combo_poe2);

    main_layout->addStretch(1);

    continue_button->setDefault(true);
    continue_button->setAutoDefault(true);

    progress_label->setText(tr("Initializing database..."));
    QTimer::singleShot(0, this, &InitializationDialog::initDatabase);
}

void InitializationDialog::closeEvent(QCloseEvent* event)
{
    event->accept();

    if (result() == QDialog::Rejected) {
        QCoreApplication::quit();
    }
}

void InitializationDialog::initDatabase()
{
    bool result = Database::initConnection();
    result = result && Database::initAddConnection();

    result = result && Database::createInfoTable();
    result = result && Database::createExchangeCacheTable(Game::Poe1);
    result = result && Database::createExchangeCacheTable(Game::Poe2);
    result = result && Database::createExchangeCostCacheTable(Game::Poe1);
    result = result && Database::createExchangeCostCacheTable(Game::Poe2);
    result = result && Database::createTradeCacheTable(Game::Poe1);
    result = result && Database::createTradeCacheTable(Game::Poe2);
    result = result && Database::createTradeCostCacheTable(Game::Poe1);
    result = result && Database::createTradeCostCacheTable(Game::Poe2);
    result = result && Database::createPlansTable(Game::Poe1);
    result = result && Database::createPlansTable(Game::Poe2);
    result = result && Database::createSnapshotTable(Game::Poe1);
    result = result && Database::createSnapshotTable(Game::Poe2);

    auto db_version = Database::selectInfo(Database::db_version_key).toInt();
    if (db_version != Database::db_version) {
        result = result && Database::updateInfo(Database::db_version_key, Database::db_version);
    }

    if (!result) {
        progress_label->setText(tr("Database initialization failed."));
        return;
    }

    progress_label->setText(tr("Reading data..."));
    QTimer::singleShot(0, this, &InitializationDialog::readDatabase);
}

void InitializationDialog::readDatabase()
{
    bool result = AppState::state.exchange_cache_poe1->readCurrencyTypes();
    result = result && AppState::state.exchange_cache_poe2->readCurrencyTypes();

    result = result && AppState::state.exchange_cache_poe1->readDatabase();
    result = result && AppState::state.exchange_cache_poe2->readDatabase();
    result = result && AppState::state.trade_cache_poe1->readDatabase();
    result = result && AppState::state.trade_cache_poe2->readDatabase();

    result = result && AppState::state.plan_model_poe1->readDatabase();
    result = result && AppState::state.plan_model_poe2->readDatabase();

    result = result && AppState::state.snapshots_poe1->readDatabase();
    result = result && AppState::state.snapshots_poe2->readDatabase();

    if (!result) {
        progress_label->setText(tr("Reading of database failed."));
        return;
    }

    if (Settings::offline_mode) {
        finishInitialization();
        return;
    }
    offline_button->setEnabled(true);

    progress_label->setText(tr("Requesting leagues from poe.ninja..."));
    requestLeagues();
}

void InitializationDialog::requestLeagues()
{
    auto reply = AppState::state.exchange_manager->getLeagues(Game::Poe1);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        parseLeagues(Game::Poe1, reply);
    });

    reply = AppState::state.exchange_manager->getLeagues(Game::Poe2);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        parseLeagues(Game::Poe2, reply);
    });
}

void InitializationDialog::parseLeagues(Game game, QNetworkReply* reply)
{
    QRestReply rest(reply);
    if (!rest.isSuccess()) {
        progress_label->setText(tr("Failed to request leagues."));
        return;
    }

    auto json = rest.readJson();
    QStringList names;
    QStringList urls;
    if (!json || !ExchangeRequestManager::parseLeagues(json->object(), names, urls)) {
        progress_label->setText(tr("Failed to parse leagues."));
        return;
    }

    if (game == Game::Poe1) {
        request_finished_poe1 = true;
        league_urls_poe1 = std::move(urls);
    } else {
        request_finished_poe2 = true;
        league_urls_poe2 = std::move(urls);
    }

    auto& leagues = game == Game::Poe1 ? leagues_poe1 : leagues_poe2;
    leagues = std::move(names);

    auto cache = AppState::exchangeCache(game);
    bool is_leagues_changed = (std::ranges::find_if(leagues,
                                                    [&](const auto& l) {
                                                        return !cache->cost_cache.contains(l);
                                                    })
                               != leagues.end())
                              || !leagues.contains(Settings::currentLeague(game));

    auto combo = game == Game::Poe1 ? league_combo_poe1 : league_combo_poe2;
    if (is_leagues_changed) {
        combo->clear();
        combo->addItems(leagues);
        combo->setEnabled(true);
    }

    if (!request_finished_poe1 || !request_finished_poe2)
        return;

    request_finished_poe1 = false;
    request_finished_poe2 = false;

    is_leagues_changed = league_combo_poe1->isEnabled() || league_combo_poe2->isEnabled();

    is_data_needed_poe1 = Settings::get<settings::poe1_init_needed>()
                          || AppState::state.exchange_cache_poe1->cache.empty();
    is_data_needed_poe2 = Settings::get<settings::poe2_init_needed>()
                          || AppState::state.exchange_cache_poe2->cache.empty();
    bool is_data_needed = is_data_needed_poe1 || is_data_needed_poe2;

    if (!is_leagues_changed && !is_data_needed) {
        finishInitialization();
        return;
    }

    if (is_leagues_changed) {
        if (is_data_needed)
            progress_label->setText(tr("Select leagues to continue."));
        else
            progress_label->setText(tr("Active leagues were changed."));
        select_league_widget->show();
        continue_button->setEnabled(true);
    } else
        updateCacheData();
}

void InitializationDialog::updateCacheData()
{
    continue_button->setEnabled(false);

    if (league_combo_poe1->isEnabled()) {
        Settings::set<Settings::poe1_league>(league_combo_poe1->currentText());

        AppState::state.exchange_cache_poe1->updateLeagues(leagues_poe1, league_urls_poe1);
        AppState::state.trade_cache_poe1->updateLeagues(leagues_poe1);

        league_combo_poe1->setEnabled(false);
    }
    if (league_combo_poe2->isEnabled()) {
        Settings::set<Settings::poe2_league>(league_combo_poe2->currentText());

        AppState::state.exchange_cache_poe2->updateLeagues(leagues_poe2, league_urls_poe2);
        AppState::state.trade_cache_poe2->updateLeagues(leagues_poe2);

        league_combo_poe2->setEnabled(false);
    }

    if (!is_data_needed_poe1 && !is_data_needed_poe2) {
        finishInitialization();
        return;
    }

    requestData();
}

void InitializationDialog::requestData()
{
    requester = new ExchangeRequester{this};

    if (is_data_needed_poe1) {
        for (auto& type : AppState::state.exchange_cache_poe1->currency_types)
            requester->requests.emplace(type.first, Game::Poe1);
    }
    if (is_data_needed_poe2) {
        for (auto& type : AppState::state.exchange_cache_poe2->currency_types)
            requester->requests.emplace(type.first, Game::Poe2);
    }
    connect(requester, &ExchangeRequester::requestFailed, this, [this] {
        progress_label->setText(tr("Failed to request currency data."));
        requester->cancelRequests();
    });
    connect(requester, &ExchangeRequester::parseFailed, this, [this] {
        progress_label->setText(tr("Failed to parse currency data."));
        requester->cancelRequests();
    });

    connect(requester, &ExchangeRequester::requestFinished, this, [this](size_t requests_left) {
        if (requests_left > 0)
            progress_label->setText(tr("%1 currency types left to load.").arg(requests_left));
        else {
            QTimer::singleShot(3000, this, &InitializationDialog::finishInitialization);

            if (is_data_needed_poe1) {
                auto div_card_link_poe1 = u"/image/Art/2DItems/Divination/InventoryIcon.png"_s;
                auto div_card_file_poe1 = AppState::state.exchange_cache_poe1->iconFileName(
                    AppState::state.exchange_cache_poe1->div_card_icon_id);
                if (!QFile::exists(div_card_file_poe1))
                    AppState::state.exchange_manager->downloadIcon(div_card_link_poe1,
                                                                   div_card_file_poe1);

                Settings::set<Settings::poe1_init_needed>(false);
            }

            if (is_data_needed_poe2) {
                Settings::set<Settings::poe2_init_needed>(false);
            }
        }
    });

    QDir::current().mkpath("currency_icons/poe1");
    QDir::current().mkpath("currency_icons/poe2");

    progress_label->setText(tr("Requesting currency data from poe.ninja..."));

    requester->startRequests(true);
}

void InitializationDialog::finishInitialization()
{
    if (is_data_needed_poe1) {
        AppState::state.exchange_cache_poe1->saveCache();
        AppState::state.exchange_cache_poe1->saveCostCache();
        AppState::state.exchange_cache_poe1->initExchangeData();
    }
    if (is_data_needed_poe2) {
        AppState::state.exchange_cache_poe2->saveCache();
        AppState::state.exchange_cache_poe2->saveCostCache();
        AppState::state.exchange_cache_poe2->initExchangeData();
    }

    accept();

    mw->restoreSession();
    mw->show();

    deleteLater();
}

} // namespace planner
