#ifndef SETTINGS_H
#define SETTINGS_H

#include "Game.h"
#include "ItemTime.h"
#include "SettingsKey.h"
#include <QSettings>

namespace planner {

class Settings
{
public:
    using enum settings::SettingsKey;

    static inline bool offline_mode{false};
    static inline bool stays_on_top{false};

    static QSettings get();

    static void initCache();

    template<settings::SettingsKey key>
    static settings::KeyType<key> get()
    {
        return cache[key].value<settings::KeyType<key>>();
    }

    template<settings::SettingsKey key>
    static void set(const settings::KeyType<key>& value, QSettings& settings)
    {
        cache[key] = value;
        settings.setValue(settings::KeyTraits<key>::key, value);
    }
    template<settings::SettingsKey key>
    static void set(const settings::KeyType<key>& value)
    {
        cache[key] = value;
        get().setValue(settings::KeyTraits<key>::key, value);
    }

    template<settings::SettingsKey key>
    static void read(const QSettings& settings)
    {
        cache[key] = settings.value(settings::KeyTraits<key>::key);
    }
    template<settings::SettingsKey key>
    static void read(const QSettings& settings, settings::KeyType<key>&& default_value)
    {
        cache[key] = settings.value(settings::KeyTraits<key>::key, std::move(default_value));
    }

    template<settings::SettingsKey key>
    static void setCache(const settings::KeyType<key>& value)
    {
        cache[key] = value;
    }
    template<settings::SettingsKey key>
    static void save(QSettings& settings)
    {
        settings.setValue(settings::KeyTraits<key>::key, cache[key]);
    }
    template<settings::SettingsKey key>
    static void save()
    {
        get().setValue(settings::KeyTraits<key>::key, cache[key]);
    }

    static QString currentLeague(Game game)
    {
        return game == Game::Poe1 ? get<poe1_league>() : get<poe2_league>();
    }

    static ItemTime defaultTradeTime() { return ItemTime{get<step_items_default_trade_time>()}; }
    static ItemTime defaultExchangeTime()
    {
        return ItemTime{get<step_items_default_exchange_time>()};
    }

    static std::chrono::milliseconds tradeCostExpirationTime();
    static std::chrono::milliseconds exchangeCostExpirationTime();
    static std::chrono::milliseconds exchangeRequestDelay();

    static const QLatin1StringView windows_main_geometry;
    static const QLatin1StringView windows_main_state;
    static const QLatin1StringView windows_main_last_plan;
    static const QLatin1StringView windows_main_snapshot_poe1;
    static const QLatin1StringView windows_main_snapshot_poe2;
    static const QLatin1StringView windows_web_view_dialog_geometry;
    static const QLatin1StringView windows_searches_dialog_geometry;
    static const QLatin1StringView windows_searches_view_columns;
    static const QLatin1StringView windows_snapshots_dialog_size;
    static const QLatin1StringView windows_snapshots_view_columns;
    static const QLatin1StringView windows_request_edit_dialog_size;
    static const QLatin1StringView windows_shopping_dialog_geometry;
    static const QLatin1StringView windows_plan_search_dialog_size;

private:
    static std::array<QVariant, last + 1> cache;
};

} // namespace planner

#endif // SETTINGS_H
