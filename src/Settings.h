#ifndef SETTINGS_H
#define SETTINGS_H

#include "Game.h"
#include "ItemTime.h"
#include <QSettings>

namespace planner {

class Settings
{
public:
    static QSettings get();

    static QString currentLeague(Game game);
    static bool initNeeded(Game game);

    static ItemTime defaultTradeTime();
    static ItemTime defaultExchangeTime();

    static std::chrono::milliseconds tradeCostExpirationTime();
    static std::chrono::milliseconds exchangeCostExpirationTime();
    static std::chrono::milliseconds exchangeRequestDelay();

    static const QLatin1StringView windows_main_geometry;
    static const QLatin1StringView windows_main_state;
    static const QLatin1StringView windows_main_hide_descriptions;
    static const QLatin1StringView windows_web_view_dialog_geometry;

    static const QLatin1StringView poe1_realm;
    static const QLatin1StringView poe1_league;

    static const QLatin1StringView poe2_league;

    static const QLatin1StringView poe1_init_needed;
    static const QLatin1StringView poe2_init_needed;

    static const QLatin1StringView trade_cost_expiration_time;
    static const QLatin1StringView exchange_cost_expiration_time;
    static const QLatin1StringView exchange_request_delay;

    static const QLatin1StringView step_items_default_trade_time;
    static const QLatin1StringView step_items_default_exchange_time;
};

} // namespace planner

#endif // SETTINGS_H
