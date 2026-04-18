#include "Settings.h"
#include <QApplication>

using namespace std::chrono;
using namespace Qt::StringLiterals;

namespace planner {
using namespace settings;

static constexpr milliseconds min_trade_expiration_time{minutes{10}};
static constexpr milliseconds min_exchange_expiration_time{minutes{60}};
static constexpr milliseconds min_exchange_delay{seconds{3}};

QSettings Settings::get()
{
    static const QString file = qApp->applicationDirPath() + u"/settings.ini";
    return QSettings{file, QSettings::Format::IniFormat};
}

void Settings::initCache()
{
    auto settings = get();

    read<windows_main_hide_descriptions>(settings);
    read<windows_main_hide_empty_resources>(settings);
    read<windows_main_hide_empty_results>(settings);
    read<windows_main_hide_not_used_items>(settings);
    read<windows_main_hide_title_currency_name>(settings);

    read<poe1_realm>(settings);
    read<poe1_league>(settings);

    read<poe2_league>(settings);

    read<snapshots_use_current_if_missing>(settings);

    read<poe1_init_needed>(settings, true);
    read<poe2_init_needed>(settings, true);

    read<trade_use_query_as_description>(settings);
    read<trade_cost_expiration_time>(settings, min_trade_expiration_time.count() * 3);
    read<exchange_cost_expiration_time>(settings, min_exchange_expiration_time.count() * 2);
    read<exchange_request_delay>(settings, 5000);

    read<step_items_default_trade_time>(settings);
    read<step_items_default_exchange_time>(settings);

    read<import_overwrite_names>(settings, true);
    read<import_add_prefix>(settings, true);
    read<import_add_prefix_requests>(settings, true);
    read<export_with_dependencies>(settings, true);
    read<export_with_requests>(settings, true);

    read<language_exchange_items>(settings, u"en"_s);
    read<language_trade_query>(settings, u"en"_s);

    read<hotkeys_next_item>(settings);
    read<hotkeys_paste_want>(settings);
    read<hotkeys_paste_want_amount>(settings);
    read<hotkeys_paste_have>(settings);
    read<hotkeys_paste_have_amount>(settings);
    read<hotkeys_open_link>(settings);
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
const QLatin1StringView Settings::windows_snapshots_dialog_size{"windows/snapshots_dialog_size"};
const QLatin1StringView Settings::windows_snapshots_view_columns{"windows/snapshots_view_columns"};
const QLatin1StringView Settings::windows_request_edit_dialog_size{
    "windows/request_edit_dialog_size"};
const QLatin1StringView Settings::windows_shopping_dialog_geometry{
    "windows/shopping_dialog_geometry"};
const QLatin1StringView Settings::windows_plan_search_dialog_size{
    "windows/plan_search_dialog_size"};
const QLatin1StringView Settings::windows_main_last_plan{"windows/main_last_plan"};
const QLatin1StringView Settings::windows_main_snapshot_poe1{"windows/main_snapshot_poe1"};
const QLatin1StringView Settings::windows_main_snapshot_poe2{"windows/main_snapshot_poe2"};

std::array<QVariant, Settings::last + 1> Settings::cache{};

} // namespace planner
