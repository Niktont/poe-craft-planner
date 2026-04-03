#include "Settings.h"
#include <QApplication>

using namespace std::chrono;
using namespace Qt::StringLiterals;

namespace planner {
using namespace settings;

static constexpr milliseconds min_trade_expiration_time{minutes{10}};
static constexpr milliseconds min_exchange_expiration_time{minutes{60}};
static constexpr milliseconds min_exchange_delay{seconds{3}};

bool Settings::offline_mode{false};

QSettings Settings::get()
{
    static const QString file = qApp->applicationDirPath() + u"/settings.ini";
    return QSettings{file, QSettings::Format::IniFormat};
}

void Settings::initCache()
{
    auto settings = get();

    cache[poe1_realm] = settings.value(KeyTraits<poe1_realm>::key);

    cache[poe1_league] = settings.value(KeyTraits<poe1_league>::key);
    cache[poe2_league] = settings.value(KeyTraits<poe2_league>::key);

    cache[poe1_init_needed] = settings.value(KeyTraits<poe1_init_needed>::key, true);
    cache[poe2_init_needed] = settings.value(KeyTraits<poe2_init_needed>::key, true);

    cache[trade_cost_expiration_time] = settings.value(KeyTraits<trade_cost_expiration_time>::key,
                                                       min_trade_expiration_time.count() * 3);
    cache[exchange_cost_expiration_time] = settings
                                               .value(KeyTraits<exchange_cost_expiration_time>::key,
                                                      min_exchange_expiration_time.count() * 2);
    cache[exchange_request_delay] = settings.value(KeyTraits<exchange_request_delay>::key, 5000);

    cache[step_items_default_trade_time] = settings.value(
        KeyTraits<step_items_default_trade_time>::key);
    cache[step_items_default_exchange_time] = settings.value(
        KeyTraits<step_items_default_exchange_time>::key);

    cache[import_overwrite_names] = settings.value(KeyTraits<import_overwrite_names>::key, true);
    cache[import_add_prefix] = settings.value(KeyTraits<import_add_prefix>::key, true);
    cache[import_add_prefix_requests] = settings.value(KeyTraits<import_add_prefix_requests>::key,
                                                       true);

    cache[language_exchange_items] = settings.value(KeyTraits<language_exchange_items>::key,
                                                    u"en"_s);

    cache[hotkeys_next_item] = settings.value(KeyTraits<hotkeys_next_item>::key);
    cache[hotkeys_paste_want] = settings.value(KeyTraits<hotkeys_paste_want>::key);
    cache[hotkeys_paste_want_amount] = settings.value(KeyTraits<hotkeys_paste_want_amount>::key);
    cache[hotkeys_paste_have] = settings.value(KeyTraits<hotkeys_paste_have>::key);
    cache[hotkeys_paste_have_amount] = settings.value(KeyTraits<hotkeys_paste_have_amount>::key);
    cache[hotkeys_open_link] = settings.value(KeyTraits<hotkeys_open_link>::key);
}

milliseconds Settings::tradeCostExpirationTime()
{
    auto time = milliseconds{get<trade_cost_expiration_time>()};
    return std::max(time, min_trade_expiration_time);
}

milliseconds Settings::exchangeCostExpirationTime()
{
    auto time = milliseconds{get<exchange_cost_expiration_time>()};
    return std::max(time, min_exchange_expiration_time);
}

milliseconds Settings::exchangeRequestDelay()
{
    auto time = milliseconds{get<exchange_request_delay>()};
    return std::max(time, min_exchange_delay);
}

const QLatin1StringView Settings::windows_main_geometry{"windows/main_geometry"};
const QLatin1StringView Settings::windows_main_state{"windows/main_state"};
const QLatin1StringView Settings::windows_web_view_dialog_geometry{
    "windows/web_view_dialog_geometry"};
const QLatin1StringView Settings::windows_searches_dialog_geometry{
    "windows/searches_dialog_geometry"};
const QLatin1StringView Settings::windows_searches_view_columns{"windows/searches_view_columns"};
const QLatin1StringView Settings::windows_request_edit_dialog_size{
    "windows/request_edit_dialog_size"};
const QLatin1StringView Settings::windows_shopping_dialog_geometry{
    "windows/shopping_dialog_geometry"};
const QLatin1StringView Settings::windows_plan_search_dialog_size{
    "windows/plan_search_dialog_size"};
const QLatin1StringView Settings::windows_main_hide_descriptions{"windows/main_hide_descriptions"};
const QLatin1StringView Settings::windows_main_hide_empty_resources{
    "windows/main_hide_empty_resources"};
const QLatin1StringView Settings::windows_main_hide_empty_results{
    "windows/main_hide_empty_results"};
const QLatin1StringView Settings::windows_main_hide_not_used_items{
    "windows/main_hide_not_used_items"};
const QLatin1StringView Settings::windows_main_last_plan{"windows/main_last_plan"};

std::array<QVariant, Settings::last + 1> Settings::cache{};

} // namespace planner
