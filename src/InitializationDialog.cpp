#include "InitializationDialog.h"

#include "Database.h"
#include "ExchangeRequestCache.h"
#include "ExchangeRequestManager.h"
#include "MainWindow.h"
#include "Settings.h"
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
#include <QStackedWidget>
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
    setLayout(main_layout);
    setMinimumWidth(300);

    progress_label = new QLabel{};
    progress_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    progress_label->setWordWrap(true);
    layout()->addWidget(progress_label);

    continue_button = new QPushButton{tr("Continue")};
    layout()->addWidget(continue_button);
    continue_button->setEnabled(false);
    connect(continue_button, &QPushButton::clicked, this, &InitializationDialog::updateCacheData);

    stacked_widget = new QStackedWidget{};
    layout()->addWidget(stacked_widget);
    stacked_widget->hide();

    select_league_widget = new QWidget{};
    select_league_widget->setLayout(new QVBoxLayout{});
    stacked_widget->addWidget(select_league_widget);

    select_league_widget->layout()->addWidget(new QLabel{tr("Select league for PoE 1:")});
    league_combo_poe1 = new QComboBox{};
    league_combo_poe1->setEnabled(false);
    select_league_widget->layout()->addWidget(league_combo_poe1);

    select_league_widget->layout()->addWidget(new QLabel{tr("Select league for PoE 2:")});
    league_combo_poe2 = new QComboBox{};
    league_combo_poe2->setEnabled(false);
    select_league_widget->layout()->addWidget(league_combo_poe2);

    main_layout->addStretch(1);

    continue_button->setAutoDefault(true);

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
    progress_label->setText(tr("Initializing database..."));
    qApp->processEvents();

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

    result = result && Database::updateInfo(Database::db_version_key, Database::db_version);

    if (!result) {
        progress_label->setText(tr("Database initialization failed."));
        return;
    }

    progress_label->setText(tr("Reading data..."));
    QTimer::singleShot(1, this, &InitializationDialog::readDatabase);
}

void InitializationDialog::readDatabase()
{
    bool result = mw->exchange_cache_poe1->readCurrencyTypes();
    result = result && mw->exchange_cache_poe2->readCurrencyTypes();

    result = result && mw->exchange_cache_poe1->readDatabase();
    result = result && mw->exchange_cache_poe2->readDatabase();
    result = result && mw->trade_cache_poe1->readDatabase();
    result = result && mw->trade_cache_poe2->readDatabase();

    result = result && mw->plan_model_poe1->readDatabase();
    result = result && mw->plan_model_poe2->readDatabase();

    result = result && mw->exchange_cache_poe1->readAdditionalData();
    result = result && mw->exchange_cache_poe2->readAdditionalData();

    if (!result) {
        progress_label->setText(tr("Reading of database failed."));
        return;
    }

    progress_label->setText(tr("Requesting leagues from poe.ninja..."));
    requestLeagues();
}

void InitializationDialog::requestLeagues()
{
    auto reply = mw->exchange_manager->getLeagues(Game::Poe1);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        parseLeagues(Game::Poe1, reply);
    });

    reply = mw->exchange_manager->getLeagues(Game::Poe2);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        parseLeagues(Game::Poe2, reply);
    });
}

void InitializationDialog::parseLeagues(Game game, QNetworkReply* reply)
{
    if (game == Game::Poe1)
        request_finished_poe1 = true;
    else
        request_finished_poe2 = true;

    QRestReply rest(reply);
    if (!rest.isSuccess()) {
        requestFailed(tr("leagues"));
        return;
    }

    auto json = rest.readJson();
    QStringList names;
    QStringList urls;
    if (!json || !ExchangeRequestManager::parseLeagues(json->object(), names, urls)) {
        parseFailed(tr("leagues"));
        return;
    }
    auto& league_urls = game == Game::Poe1 ? league_urls_poe1 : league_urls_poe2;
    league_urls = urls;

    auto cache = mw->exchangeCache(game);
    bool is_leagues_changed{false};
    for (auto& league : std::as_const(names)) {
        if (!cache->cost_cache.contains(league)) {
            is_leagues_changed = true;
            break;
        }
    }
    if (!is_leagues_changed) {
        auto currentLeague = Settings::currentLeague(game);
        if (!names.contains(currentLeague))
            is_leagues_changed = true;
    }

    auto combo = game == Game::Poe1 ? league_combo_poe1 : league_combo_poe2;
    if (is_leagues_changed) {
        combo->clear();
        combo->addItems(names);
        combo->setEnabled(true);
    }

    if (!request_finished_poe1 || !request_finished_poe2)
        return;

    request_finished_poe1 = false;
    request_finished_poe2 = false;

    is_leagues_changed = league_combo_poe1->isEnabled() || league_combo_poe2->isEnabled();
    bool is_data_needed = isDataNeeded(Game::Poe1) || isDataNeeded(Game::Poe2);
    if (!is_leagues_changed && !is_data_needed) {
        finishInitialization();
        return;
    }

    if (is_leagues_changed) {
        if (is_data_needed)
            progress_label->setText(tr("Select leagues to continue."));
        else
            progress_label->setText(tr("Active leagues were changed."));
        stacked_widget->show();
        continue_button->setEnabled(true);
    } else
        updateCacheData();
}

void InitializationDialog::updateCacheData()
{
    continue_button->setEnabled(false);

    auto settings = Settings::get();
    if (league_combo_poe1->isEnabled()) {
        settings.setValue(Settings::poe1_league, league_combo_poe1->currentText());

        QStringList leagues;
        for (int i = 0; i < league_combo_poe1->count(); ++i)
            leagues.push_back(league_combo_poe1->itemText(i));

        auto& cost_cache = mw->exchange_cache_poe1->cost_cache;
        auto& trade_cost_cache = mw->trade_cache_poe1->cost_cache;
        boost::container::erase_if(cost_cache, [&](const auto& it) {
            if (!leagues.contains(it.first)) {
                Database::deleteExchangeCostCache(Game::Poe1, it.first);
                return true;
            }
            return false;
        });
        boost::container::erase_if(trade_cost_cache, [&](const auto& it) {
            if (!leagues.contains(it.first)) {
                Database::deleteTradeCostCache(Game::Poe1, it.first);
                return true;
            }
            return false;
        });

        for (int i = 0; i < leagues.size(); ++i) {
            if (auto res = cost_cache.try_emplace(leagues[i]); res.second) {
                res.first->second.league_url = league_urls_poe1[i];
                res.first->second.is_changed = true;
            }
            if (auto res = trade_cost_cache.try_emplace(leagues[i]); res.second)
                res.first->second.is_changed = true;
        }
        mw->exchange_cache_poe1->saveCostCache();
        mw->trade_cache_poe1->saveCostCache();

        league_combo_poe1->setEnabled(false);
    }
    if (league_combo_poe2->isEnabled()) {
        settings.setValue(Settings::poe2_league, league_combo_poe2->currentText());

        QStringList leagues;
        for (int i = 0; i < league_combo_poe2->count(); ++i)
            leagues.push_back(league_combo_poe2->itemText(i));

        auto& cost_cache = mw->exchange_cache_poe2->cost_cache;
        auto& trade_cost_cache = mw->trade_cache_poe2->cost_cache;
        boost::container::erase_if(cost_cache, [&](const auto& it) {
            if (!leagues.contains(it.first)) {
                Database::deleteExchangeCostCache(Game::Poe2, it.first);
                return true;
            }
            return false;
        });
        boost::container::erase_if(trade_cost_cache, [&](const auto& it) {
            if (!leagues.contains(it.first)) {
                Database::deleteTradeCostCache(Game::Poe2, it.first);
                return true;
            }
            return false;
        });

        for (int i = 0; i < leagues.size(); ++i) {
            if (auto res = cost_cache.try_emplace(leagues[i]); res.second) {
                res.first->second.league_url = league_urls_poe2[i];
                res.first->second.is_changed = true;
            }
            if (auto res = trade_cost_cache.try_emplace(leagues[i]); res.second)
                res.first->second.is_changed = true;
        }
        mw->exchange_cache_poe2->saveCostCache();
        mw->trade_cache_poe2->saveCostCache();

        league_combo_poe2->setEnabled(false);
    }

    bool is_data_needed_poe1 = isDataNeeded(Game::Poe1);
    bool is_data_needed_poe2 = isDataNeeded(Game::Poe2);
    if (!is_data_needed_poe1 && !is_data_needed_poe2) {
        finishInitialization();
        return;
    }

    if (is_data_needed_poe1) {
        for (auto& type : mw->exchange_cache_poe1->currency_types)
            currency_types_poe1.push(type.first);
    }
    if (is_data_needed_poe2) {
        for (auto& type : mw->exchange_cache_poe2->currency_types)
            currency_types_poe2.push(type.first);
    }

    QDir::current().mkpath("currency_icons/poe1");
    QDir::current().mkpath("currency_icons/poe2");

    progress_label->setText(tr("Requesting currency data from poe.ninja..."));

    requestData();
}

void InitializationDialog::requestData()
{
    if (!currency_types_poe1.empty()) {
        auto& type = currency_types_poe1.front();
        auto reply = mw->exchange_manager->getOverview(Game::Poe1, type);
        connect(reply, &QNetworkReply::finished, this, [this, type, reply] {
            parseOverviewData(Game::Poe1, type, reply);
        });

        currency_types_poe1.pop();
    } else if (!currency_types_poe2.empty()) {
        auto& type = currency_types_poe2.front();
        auto reply = mw->exchange_manager->getOverview(Game::Poe2, type);
        connect(reply, &QNetworkReply::finished, this, [this, type, reply] {
            parseOverviewData(Game::Poe2, type, reply);
        });
        currency_types_poe2.pop();
    }
}

void InitializationDialog::parseOverviewData(Game game, QString type, QNetworkReply* reply)
{
    QRestReply rest(reply);
    if (!rest.isSuccess()) {
        requestFailed(tr("currency data"));
        return;
    }

    auto cache = mw->exchangeCache(game);
    auto json = rest.readJson();
    if (!json) {
        parseFailed(tr("currency data"));
        return;
    }
    const auto obj = json->object();
    if (!mw->exchange_manager->parseOverviewItems(obj, type, *cache)
        || !ExchangeRequestManager::parseOverviewCosts(obj, *cache)
        || !ExchangeRequestManager::parseCore(obj["core"].toObject(), *cache)) {
        parseFailed(tr("currency data"));
        return;
    }

    if (!currency_types_poe1.empty() || !currency_types_poe2.empty()) {
        progress_label->setText(tr("%1 currency types left to load.")
                                    .arg(currency_types_poe1.size() + currency_types_poe2.size()));
        QTimer::singleShot(Settings::exchangeRequestDelay(),
                           this,
                           &InitializationDialog::requestData);
    } else {
        QTimer::singleShot(3000, this, &InitializationDialog::finishInitialization);

        auto div_card_link_poe1 = u"/image/Art/2DItems/Divination/InventoryIcon.png"_s;
        auto div_card_file_poe1 = mw->exchange_cache_poe1->iconFileName(u"div_card"_s);
        if (!QFile::exists(div_card_file_poe1))
            mw->exchange_manager->downloadIcon(div_card_link_poe1, div_card_file_poe1);

        mw->exchange_cache_poe1->saveCache();
        mw->exchange_cache_poe1->saveCostCache();
        mw->exchange_cache_poe2->saveCache();
        mw->exchange_cache_poe2->saveCostCache();
    }
}

void InitializationDialog::finishInitialization()
{
    auto settings = Settings::get();
    settings.setValue(Settings::poe1_init_needed, false);
    settings.setValue(Settings::poe2_init_needed, false);

    for (auto& [id, data] : mw->exchange_cache_poe1->cache) {
        if (data.icon.isNull())
            data.icon = QIcon{ExchangeRequestCache::iconFileName(Game::Poe1, id)};
    }
    for (auto& [id, data] : mw->exchange_cache_poe2->cache) {
        if (data.icon.isNull())
            data.icon = QIcon{ExchangeRequestCache::iconFileName(Game::Poe2, id)};
    }

    accept();
    mw->show();
    deleteLater();
}

void InitializationDialog::requestFailed(QString type)
{
    progress_label->setText(tr("Failed to request %1.").arg(type));
}

void InitializationDialog::parseFailed(QString type)
{
    progress_label->setText(tr("Failed to parse %1.").arg(type));
}

bool InitializationDialog::isDataNeeded(Game game)
{
    return Settings::initNeeded(game)
           || (game == Game::Poe1 ? mw->exchange_cache_poe1->cache.empty()
                                  : mw->exchange_cache_poe2->cache.empty());
}

} // namespace planner
