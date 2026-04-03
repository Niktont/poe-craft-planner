#ifndef SETTINGSKEY_H
#define SETTINGSKEY_H

#include <QKeySequence>
#include <QString>

namespace planner {
namespace settings {
enum SettingsKey {
    poe1_realm,
    poe1_league,

    poe2_league,

    poe1_init_needed,
    poe2_init_needed,

    trade_cost_expiration_time,
    exchange_cost_expiration_time,
    exchange_request_delay,

    step_items_default_trade_time,
    step_items_default_exchange_time,

    import_overwrite_names,
    import_add_prefix,
    import_add_prefix_requests,

    language_exchange_items,

    hotkeys_next_item,
    hotkeys_paste_want,
    hotkeys_paste_want_amount,
    hotkeys_paste_have,
    hotkeys_paste_have_amount,
    hotkeys_open_link,

    last = hotkeys_open_link,
};

template<SettingsKey key>
struct KeyTraits;

template<SettingsKey key>
using KeyType = typename KeyTraits<key>::type;

struct BoolValue
{
    using type = bool;
};
struct DoubleValue
{
    using type = double;
};
struct StringValue
{
    using type = QString;
};
struct LongLongValue
{
    using type = long long;
};
struct KeySequenceValue
{
    using type = QKeySequence;
};

template<>
struct KeyTraits<poe1_realm> : StringValue
{
    static inline const std::string key{"poe1/realm"};
};
template<>
struct KeyTraits<poe1_league> : StringValue
{
    static inline const std::string key{"poe1/league"};
};

template<>
struct KeyTraits<poe2_league> : StringValue
{
    static inline const std::string key{"poe2/league"};
};

template<>
struct KeyTraits<poe1_init_needed> : BoolValue
{
    static inline const std::string key{"poe1/init_needed"};
};
template<>
struct KeyTraits<poe2_init_needed> : BoolValue
{
    static inline const std::string key{"poe2/init_needed"};
};

template<>
struct KeyTraits<trade_cost_expiration_time> : LongLongValue
{
    static inline const std::string key{"trade/cost_expiration_time"};
};
template<>
struct KeyTraits<exchange_cost_expiration_time> : LongLongValue
{
    static inline const std::string key{"exchange/cost_expiration_time"};
};
template<>
struct KeyTraits<exchange_request_delay> : LongLongValue
{
    static inline const std::string key{"exchange/request_delay"};
};

template<>
struct KeyTraits<step_items_default_trade_time> : DoubleValue
{
    static inline const std::string key{"step_items/default_trade_time"};
};
template<>
struct KeyTraits<step_items_default_exchange_time> : DoubleValue
{
    static inline const std::string key{"step_items/default_exchange_time"};
};

template<>
struct KeyTraits<import_overwrite_names> : BoolValue
{
    static inline const std::string key{"import/overwrite_names"};
};
template<>
struct KeyTraits<import_add_prefix> : BoolValue
{
    static inline const std::string key{"import/add_prefix"};
};
template<>
struct KeyTraits<import_add_prefix_requests> : BoolValue
{
    static inline const std::string key{"import/add_prefix_requests"};
};

template<>
struct KeyTraits<language_exchange_items> : StringValue
{
    static inline const std::string key{"language/exchange_items"};
};

template<>
struct KeyTraits<hotkeys_next_item> : KeySequenceValue
{
    static inline const std::string key{"hotkeys/next_item"};
};
template<>
struct KeyTraits<hotkeys_paste_want> : KeySequenceValue
{
    static inline const std::string key{"hotkeys/paste_want"};
};
template<>
struct KeyTraits<hotkeys_paste_want_amount> : KeySequenceValue
{
    static inline const std::string key{"hotkeys/paste_want_amount"};
};
template<>
struct KeyTraits<hotkeys_paste_have> : KeySequenceValue
{
    static inline const std::string key{"hotkeys/paste_have"};
};
template<>
struct KeyTraits<hotkeys_paste_have_amount> : KeySequenceValue
{
    static inline const std::string key{"hotkeys/paste_have_amount"};
};
template<>
struct KeyTraits<hotkeys_open_link> : KeySequenceValue
{
    static inline const std::string key{"hotkeys/open_link"};
};

} // namespace settings
} // namespace planner

#endif // SETTINGSKEY_H
