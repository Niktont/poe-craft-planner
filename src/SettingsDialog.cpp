#include "SettingsDialog.h"
#include "ExchangeRequestCache.h"
#include "MainWindow.h"
#include "Settings.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QItemSelectionModel>
#include <QListView>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStringListModel>
#include <QVBoxLayout>

using namespace std::chrono;

namespace planner {
enum Tabs {
    Requests,
    League,
};

static long long msFromMinutes(int val)
{
    return duration_cast<milliseconds>(minutes{val}).count();
}
static int minutesFromMs(milliseconds val)
{
    return duration_cast<minutes>(val).count();
}

SettingsDialog::SettingsDialog(MainWindow& mw)
    : QDialog{&mw}
{
    needs_reset.fill(true);
    setWindowTitle(tr("Settings"));

    auto main_layout = new QVBoxLayout{};
    setLayout(main_layout);
    auto edit_layout = new QHBoxLayout{};
    main_layout->addLayout(edit_layout);

    tab_model = new QStringListModel{};
    QStringList tabs;
    tabs.append(tr("Requests"));
    tabs.append(tr("League"));
    tab_model->setStringList(tabs);

    tab_view = new QListView{};
    tab_view->setModel(tab_model);
    tab_view->setMaximumWidth(tab_view->sizeHintForColumn(0) + tab_view->lineWidth() * 2
                              + tab_view->verticalScrollBar()->sizeHint().width());

    connect(tab_view->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this,
            [this](const QModelIndex& idx) {
                tabs_widget->setCurrentIndex(idx.row());
                resetTab(idx.row());
            });

    edit_layout->addWidget(tab_view);

    setupRequestsTab();
    setupLeagueTab();

    tabs_widget = new QStackedWidget{};
    tabs_widget->addWidget(requests_tab);
    tabs_widget->addWidget(league_tab);
    edit_layout->addWidget(tabs_widget, 0, Qt::AlignTop | Qt::AlignLeft);

    auto buttons = new QDialogButtonBox{};

    auto button = buttons->addButton(QDialogButtonBox::Ok);
    connect(button, &QPushButton::clicked, this, [this] {
        save();
        accept();
    });
    button = buttons->addButton(QDialogButtonBox::Cancel);
    connect(button, &QPushButton::clicked, this, &QDialog::reject);

    button = buttons->addButton(QDialogButtonBox::Apply);
    connect(button, &QPushButton::clicked, this, &SettingsDialog::save);
    main_layout->addWidget(buttons);

    connect(this, &QDialog::rejected, this, [this] {
        for (size_t i = 0; i < is_changed.size(); ++i)
            needs_reset[i] |= is_changed[i];
        is_changed.fill(false);
    });
}

void SettingsDialog::openSettings()
{
    auto current = tabs_widget->currentIndex();
    resetTab(current);

    open();
}

void SettingsDialog::save()
{
    auto settings = Settings::get();
    if (is_changed[Requests])
        saveRequests(settings);
    if (is_changed[League])
        saveLeague(settings);

    is_changed.fill(false);
}

void SettingsDialog::setRequestsChanged()
{
    is_changed[Requests] = true;
}

void SettingsDialog::setLeagueChanged()
{
    is_changed[League] = true;
}

void SettingsDialog::setupRequestsTab()
{
    requests_tab = new QWidget{};
    requests_tab->setLayout(new QVBoxLayout{});

    auto trade_group = new QGroupBox{tr("Trade")};
    auto trade_layout = new QFormLayout{};
    trade_group->setLayout(trade_layout);

    trade_min_update_time = new QSpinBox{};
    trade_min_update_time->setSuffix(tr(" min"));
    trade_min_update_time->setRange(10, 9999);
    trade_layout->addRow(tr("Minimum time between updates:"), trade_min_update_time);

    trade_default_time = new QDoubleSpinBox{};
    trade_default_time->setSuffix(tr(" s"));
    trade_default_time->setRange(0, 9999.99);
    trade_layout->addRow(tr("Default time for items:"), trade_default_time);

    requests_tab->layout()->addWidget(trade_group);

    auto exchange_group = new QGroupBox{tr("Exchange")};
    auto exchange_layout = new QVBoxLayout{};
    exchange_group->setLayout(exchange_layout);

    auto exchange_form = new QFormLayout{};
    exchange_layout->addLayout(exchange_form);

    exchange_min_update_time = new QSpinBox{};
    exchange_min_update_time->setSuffix(tr(" min"));
    exchange_min_update_time->setRange(60, 9999);
    exchange_form->addRow(tr("Minimum time between updates:"), exchange_min_update_time);

    exchange_delay = new QSpinBox{};
    exchange_delay->setSuffix(tr(" ms"));
    exchange_delay->setRange(3000, 99999);
    exchange_form->addRow(tr("Delay between requests:"), exchange_delay);

    exchange_default_time = new QDoubleSpinBox{};
    exchange_default_time->setSuffix(tr(" s"));
    exchange_default_time->setRange(0, 9999.99);
    exchange_form->addRow(tr("Default time for items:"), exchange_default_time);

    reload_data_poe1 = new QCheckBox{tr("Reload PoE 1 data on next initialization")};
    exchange_layout->addWidget(reload_data_poe1);
    reload_data_poe2 = new QCheckBox{tr("Reload PoE 2 data on next initialization")};
    exchange_layout->addWidget(reload_data_poe2);

    requests_tab->layout()->addWidget(exchange_group);

    auto val_edits = requests_tab->findChildren<QAbstractSpinBox*>();
    for (auto edit : std::as_const(val_edits)) {
        connect(edit, &QAbstractSpinBox::editingFinished, this, &SettingsDialog::setRequestsChanged);
        edit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    }

    auto checkboxes = requests_tab->findChildren<QCheckBox*>();
    for (auto cb : std::as_const(checkboxes))
        connect(cb, &QCheckBox::checkStateChanged, this, &SettingsDialog::setRequestsChanged);
}

void SettingsDialog::resetTab(int index)
{
    if (needs_reset[index]) {
        switch (index) {
        case Requests:
            resetRequests();
            break;
        case League:
            resetLeague();
            break;
        }
        needs_reset[index] = false;
    }
}

void SettingsDialog::resetRequests()
{
    trade_min_update_time->setValue(minutesFromMs(Settings::tradeCostExpirationTime()));
    trade_default_time->setValue(Settings::defaultTradeTime().count());

    exchange_min_update_time->setValue(minutesFromMs(Settings::exchangeCostExpirationTime()));
    exchange_delay->setValue(Settings::exchangeRequestDelay().count());
    exchange_default_time->setValue(Settings::defaultExchangeTime().count());

    reload_data_poe1->setCheckState(Settings::initNeeded(Game::Poe1) ? Qt::Checked : Qt::Unchecked);
    reload_data_poe2->setCheckState(Settings::initNeeded(Game::Poe2) ? Qt::Checked : Qt::Unchecked);
}

void SettingsDialog::saveRequests(QSettings& settings)
{
    settings.setValue(Settings::trade_cost_expiration_time,
                      msFromMinutes(trade_min_update_time->value()));

    auto prev_trade_time = settings.value(Settings::step_items_default_trade_time).toDouble();
    auto new_trade_time = trade_default_time->value();
    if (prev_trade_time != new_trade_time) {
        settings.setValue(Settings::step_items_default_trade_time, new_trade_time);
        emit tradeTimeChanged();
    }

    settings.setValue(Settings::exchange_cost_expiration_time,
                      msFromMinutes(exchange_min_update_time->value()));
    settings.setValue(Settings::exchange_request_delay, exchange_delay->value());

    auto prev_exchange_time = settings.value(Settings::step_items_default_exchange_time).toDouble();
    auto new_exchange_time = exchange_default_time->value();
    if (prev_exchange_time != new_exchange_time) {
        settings.setValue(Settings::step_items_default_exchange_time, new_exchange_time);
        emit exchangeTimeChanged();
    }

    settings.setValue(Settings::poe1_init_needed, reload_data_poe1->checkState() == Qt::Checked);
    settings.setValue(Settings::poe2_init_needed, reload_data_poe2->checkState() == Qt::Checked);
}

void SettingsDialog::setupLeagueTab()
{
    league_tab = new QWidget{};
    auto league_layout = new QFormLayout{};
    league_tab->setLayout(league_layout);

    league_poe1 = new QComboBox{};
    connect(league_poe1, &QComboBox::currentIndexChanged, this, &SettingsDialog::setLeagueChanged);
    league_layout->addRow(tr("PoE 1 league:"), league_poe1);

    league_poe2 = new QComboBox{};
    connect(league_poe2, &QComboBox::currentIndexChanged, this, &SettingsDialog::setLeagueChanged);
    league_layout->addRow(tr("PoE 2 league:"), league_poe2);
}

void SettingsDialog::resetLeague()
{
    if (league_poe1->count() == 0) {
        QStringList leagues;
        for (auto& league : mw()->exchange_cache_poe1->cost_cache)
            leagues.push_back(league.first);
        league_poe1->addItems(leagues);
    }
    league_poe1->setCurrentText(Settings::currentLeague(Game::Poe1));

    if (league_poe2->count() == 0) {
        QStringList leagues;
        for (auto& league : mw()->exchange_cache_poe2->cost_cache)
            leagues.push_back(league.first);
        league_poe2->addItems(leagues);
    }
    league_poe2->setCurrentText(Settings::currentLeague(Game::Poe2));

    is_changed[League] = false;
}

void SettingsDialog::saveLeague(QSettings& settings)
{
    settings.setValue(Settings::poe1_league, league_poe1->currentText());
    settings.setValue(Settings::poe2_league, league_poe2->currentText());
}

MainWindow* SettingsDialog::mw() const
{
    return static_cast<MainWindow*>(parent());
}

} // namespace planner
