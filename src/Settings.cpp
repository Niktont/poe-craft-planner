#include "Settings.h"
#include <QApplication>

using namespace std::chrono;

namespace planner {

QSettings Settings::get()
{
    static const QString file = qApp->applicationDirPath() + u"/settings.ini";
    return QSettings{file, QSettings::Format::IniFormat};
}

QString Settings::currentLeague(Game game)
{
    auto settings = get();
    return game == Game::Poe1 ? settings.value(poe1_league).toString()
                              : settings.value(poe2_league).toString();
}

bool Settings::initNeeded(Game game)
{
    auto settings = get();
    return game == Game::Poe1 ? settings.value(poe1_init_needed, true).toBool()
                              : settings.value(poe2_init_needed, true).toBool();
}

ItemTime Settings::defaultTradeTime()
{
    return ItemTime{get().value(step_items_default_trade_time, 7.0).toDouble()};
}

ItemTime Settings::defaultExchangeTime()
{
    return ItemTime{get().value(step_items_default_exchange_time, 2.0).toDouble()};
}

milliseconds Settings::tradeCostExpirationTime()
{
    static constexpr milliseconds min_time{minutes{10}};
    auto time = milliseconds{
        get().value(trade_cost_expiration_time, min_time.count() * 3).toLongLong()};
    return std::max(time, min_time);
}

milliseconds Settings::exchangeCostExpirationTime()
{
    static constexpr milliseconds min_time{minutes{60}};
    auto time = milliseconds{
        get().value(exchange_cost_expiration_time, min_time.count() * 2).toLongLong()};
    return std::max(time, min_time);
}

milliseconds Settings::exchangeRequestDelay()
{
    static constexpr milliseconds min_time{seconds{3}};
    auto time = milliseconds{get().value(exchange_request_delay, 5000).toLongLong()};
    return std::max(time, min_time);
}

const QLatin1StringView Settings::windows_main_geometry{"windows/main_geometry"};
const QLatin1StringView Settings::windows_main_state{"windows/main_state"};
const QLatin1StringView Settings::windows_web_view_dialog_geometry{
    "windows/web_view_dialog_geometry"};
const QLatin1StringView Settings::windows_main_hide_descriptions{"windows/main_hide_descriptions"};
const QLatin1StringView Settings::windows_main_last_plan{"windows/main_last_plan"};

const QLatin1StringView Settings::poe1_realm{"poe1/realm"};
const QLatin1StringView Settings::poe1_league{"poe1/league"};

const QLatin1StringView Settings::poe2_league{"poe2/league"};

const QLatin1StringView Settings::poe1_init_needed{"poe1/init_needed"};
const QLatin1StringView Settings::poe2_init_needed{"poe2/init_needed"};

const QLatin1StringView Settings::trade_cost_expiration_time{"trade/cost_expiration_time"};
const QLatin1StringView Settings::exchange_cost_expiration_time{"exchange/cost_expiration_time"};
const QLatin1StringView Settings::exchange_request_delay{"exchange/request_delay"};

const QLatin1StringView Settings::step_items_default_exchange_time{
    "step_items/default_exchange_time"};
const QLatin1StringView Settings::step_items_default_trade_time{"step_items/default_trade_time"};

} // namespace planner
