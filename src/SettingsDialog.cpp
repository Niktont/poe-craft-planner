#include "SettingsDialog.h"
#include "HotkeyEdit.h"
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
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStringListModel>
#include <QVBoxLayout>

using namespace std::chrono;
using namespace Qt::StringLiterals;

namespace planner {
enum Tab {
    Requests,
    League,
    Import,
    Language,
    Hotkeys,
};

using enum settings::SettingsKey;

static long long msFromMinutes(int val)
{
    return duration_cast<milliseconds>(minutes{val}).count();
}
static int minutesFromMs(milliseconds val)
{
    return duration_cast<minutes>(val).count();
}

static const std::array<QString, 8> exchange_languages{
    u"en"_s,
    u"br"_s,
    u"ru"_s,
    u"th"_s,
    u"de"_s,
    u"fr"_s,
    u"es"_s,
    u"jp"_s,
};

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
    tabs.append(tr("Import"));
    tabs.append(tr("Language"));
    tabs.append(tr("Hotkeys"));
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
    setupImportTab();
    setupLanguageTab();
    setupHotkeysTab();

    tabs_widget = new QStackedWidget{};
    tabs_widget->addWidget(requests_tab);
    tabs_widget->addWidget(league_tab);
    tabs_widget->addWidget(import_tab);
    tabs_widget->addWidget(language_tab);
    tabs_widget->addWidget(hotkeys_tab);
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
    if (is_changed[Import])
        saveImport(settings);
    if (is_changed[Language])
        saveLanguage(settings);
    if (is_changed[Hotkeys])
        saveHotkeys(settings);

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

void SettingsDialog::setImportChanged()
{
    is_changed[Import] = true;
}

void SettingsDialog::setLanguageChanged()
{
    is_changed[Language] = true;
}

void SettingsDialog::setHotkeysChanged()
{
    is_changed[Hotkeys] = true;
}

void SettingsDialog::resetTab(int index)
{
    if (needs_reset[index]) {
        switch (static_cast<Tab>(index)) {
        case Requests:
            resetRequests();
            break;
        case League:
            resetLeague();
            break;
        case Import:
            resetImport();
            break;
        case Language:
            resetLanguage();
            break;
        case Hotkeys:
            resetHotkeys();
            break;
        }
        needs_reset[index] = false;
    }
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

void SettingsDialog::resetRequests()
{
    trade_min_update_time->setValue(minutesFromMs(Settings::tradeCostExpirationTime()));
    trade_default_time->setValue(Settings::get<step_items_default_trade_time>());

    exchange_min_update_time->setValue(minutesFromMs(Settings::exchangeCostExpirationTime()));
    exchange_delay->setValue(Settings::exchangeRequestDelay().count());
    exchange_default_time->setValue(Settings::get<step_items_default_exchange_time>());

    reload_data_poe1->setChecked(Settings::initNeeded(Game::Poe1));
    reload_data_poe2->setChecked(Settings::initNeeded(Game::Poe2));

    is_changed[Requests] = false;
}

void SettingsDialog::saveRequests(QSettings& settings)
{
    Settings::set<trade_cost_expiration_time>(msFromMinutes(trade_min_update_time->value()),
                                              settings);

    auto prev_trade_time = Settings::get<step_items_default_trade_time>();
    auto new_trade_time = trade_default_time->value();
    if (prev_trade_time != new_trade_time) {
        Settings::set<step_items_default_trade_time>(new_trade_time, settings);
        emit tradeTimeChanged();
    }

    Settings::set<exchange_cost_expiration_time>(msFromMinutes(exchange_min_update_time->value()),
                                                 settings);
    Settings::set<exchange_request_delay>(exchange_delay->value(), settings);

    auto prev_exchange_time = Settings::get<step_items_default_exchange_time>();
    auto new_exchange_time = exchange_default_time->value();
    if (prev_exchange_time != new_exchange_time) {
        Settings::set<step_items_default_exchange_time>(new_exchange_time, settings);
        emit exchangeTimeChanged();
    }

    Settings::set<poe1_init_needed>(reload_data_poe1->isChecked(), settings);
    Settings::set<poe2_init_needed>(reload_data_poe2->isChecked(), settings);
}

void SettingsDialog::setupLeagueTab()
{
    league_tab = new QWidget{};
    auto layout = new QFormLayout{};
    league_tab->setLayout(layout);

    league_poe1 = new QComboBox{};
    connect(league_poe1, &QComboBox::currentIndexChanged, this, &SettingsDialog::setLeagueChanged);
    layout->addRow(tr("PoE 1 league:"), league_poe1);

    league_poe2 = new QComboBox{};
    connect(league_poe2, &QComboBox::currentIndexChanged, this, &SettingsDialog::setLeagueChanged);
    layout->addRow(tr("PoE 2 league:"), league_poe2);
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
    Settings::set<poe1_league>(league_poe1->currentText(), settings);
    Settings::set<poe2_league>(league_poe2->currentText(), settings);
}

void SettingsDialog::setupImportTab()
{
    import_tab = new QWidget{};
    auto layout = new QVBoxLayout{};
    import_tab->setLayout(layout);

    overwrite_names = new QCheckBox{tr("Take names from imported plans on overwriting")};
    connect(overwrite_names, &QCheckBox::checkStateChanged, this, &SettingsDialog::setImportChanged);
    layout->addWidget(overwrite_names);

    add_prefix = new QCheckBox{tr("Add prefix to imported plans or folders")};
    connect(add_prefix, &QCheckBox::checkStateChanged, this, &SettingsDialog::setImportChanged);
    layout->addWidget(add_prefix);

    add_prefix_requests = new QCheckBox{tr("Add prefix to imported searches")};
    connect(add_prefix_requests,
            &QCheckBox::checkStateChanged,
            this,
            &SettingsDialog::setImportChanged);
    layout->addWidget(add_prefix_requests);

    layout->addStretch(1);
}

void SettingsDialog::resetImport()
{
    overwrite_names->setChecked(Settings::get<import_overwrite_names>());
    add_prefix->setChecked(Settings::get<import_add_prefix>());
    add_prefix_requests->setChecked(Settings::get<import_add_prefix_requests>());

    is_changed[Import] = false;
}

void SettingsDialog::saveImport(QSettings& settings)
{
    Settings::set<import_overwrite_names>(overwrite_names->isChecked(), settings);
    Settings::set<import_add_prefix>(add_prefix->isChecked(), settings);
    Settings::set<import_add_prefix_requests>(add_prefix_requests->isChecked(), settings);
}

void SettingsDialog::setupLanguageTab()
{
    language_tab = new QWidget{};
    auto layout = new QFormLayout{};
    language_tab->setLayout(layout);

    exchange_language = new QComboBox{};
    exchange_language->addItem("English");
    exchange_language->addItem("Português (Brasil)");
    exchange_language->addItem("Русский");
    exchange_language->addItem("ไทย");
    exchange_language->addItem("Deutsch");
    exchange_language->addItem("Français");
    exchange_language->addItem("Español");
    exchange_language->addItem("日本語");

    connect(exchange_language,
            &QComboBox::currentIndexChanged,
            this,
            &SettingsDialog::setLanguageChanged);
    layout->addRow(tr("Language for names of Exchange items:"), exchange_language);
}

void SettingsDialog::resetLanguage()
{
    auto it = std::ranges::find(exchange_languages, Settings::get<language_exchange_items>());
    auto pos = it != exchange_languages.end() ? std::distance(exchange_languages.begin(), it) : 0;
    exchange_language->setCurrentIndex(pos);

    is_changed[Language] = false;
}

void SettingsDialog::saveLanguage(QSettings& settings)
{
    auto prev_lang = Settings::get<language_exchange_items>();
    auto lang = exchange_languages.at(exchange_language->currentIndex());
    if (prev_lang != lang) {
        Settings::set<language_exchange_items>(lang, settings);
        QMessageBox::information(this,
                                 tr("Language changed"),
                                 tr("The language change will take effect after restart."));
    }
}

void SettingsDialog::setupHotkeysTab()
{
    hotkeys_tab = new QWidget{};
    auto layout = new QFormLayout{};
    hotkeys_tab->setLayout(layout);

    next_item = new HotkeyEdit{false};
    layout->addRow(tr("Next item:"), next_item);

    paste_want = new HotkeyEdit{true};
    layout->addRow(tr("Paste Regex/I Want currency:"), paste_want);

    paste_want_amount = new HotkeyEdit{true};
    layout->addRow(tr("Paste I Want amount:"), paste_want_amount);

    paste_have = new HotkeyEdit{true};
    layout->addRow(tr("Paste I Have currency:"), paste_have);

    paste_have_amount = new HotkeyEdit{true};
    layout->addRow(tr("Paste I Have amount:"), paste_have_amount);

    open_link = new HotkeyEdit{false};
    layout->addRow(tr("Open link:"), open_link);

    auto finish_keys = next_item->finishingKeyCombinations();
    finish_keys << Qt::Key_Escape;

    auto edits = hotkeys_tab->findChildren<QKeySequenceEdit*>();
    for (auto edit : std::as_const(edits)) {
        edit->setFinishingKeyCombinations(finish_keys);
        connect(edit,
                &QKeySequenceEdit::keySequenceChanged,
                this,
                &SettingsDialog::setHotkeysChanged);
    }
}

void SettingsDialog::resetHotkeys()
{
    next_item->setKeySequence(Settings::get<hotkeys_next_item>());
    paste_want->setKeySequence(Settings::get<hotkeys_paste_want>());
    paste_want_amount->setKeySequence(Settings::get<hotkeys_paste_want_amount>());
    paste_have->setKeySequence(Settings::get<hotkeys_paste_have>());
    paste_have_amount->setKeySequence(Settings::get<hotkeys_paste_have_amount>());
    open_link->setKeySequence(Settings::get<hotkeys_open_link>());

    is_changed[Hotkeys] = false;
}

void SettingsDialog::saveHotkeys(QSettings& settings)
{
    Settings::set<hotkeys_next_item>(next_item->keySequence(), settings);
    Settings::set<hotkeys_paste_want>(paste_want->keySequence(), settings);
    Settings::set<hotkeys_paste_want_amount>(paste_want_amount->keySequence(), settings);
    Settings::set<hotkeys_paste_have>(paste_have->keySequence(), settings);
    Settings::set<hotkeys_paste_have_amount>(paste_have_amount->keySequence(), settings);
    Settings::set<hotkeys_open_link>(open_link->keySequence(), settings);
}

MainWindow* SettingsDialog::mw() const
{
    return static_cast<MainWindow*>(parent());
}

} // namespace planner
