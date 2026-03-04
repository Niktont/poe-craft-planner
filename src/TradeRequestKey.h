#ifndef TRADEREQUESTKEY_H
#define TRADEREQUESTKEY_H

#include "Game.h"
#include <boost/container_hash/hash.hpp>
#include <boost/outcome.hpp>
#include <QHashFunctions>
#include <QJsonObject>
#include <QString>
#include <QUrl>

namespace planner {
enum class Domain : unsigned char {
    Main,
    Br,
    Ru,
    Th,
    De,
    Fr,
    Es,
    Jp,
    Daum,
    Unknown,
};

class TradeRequestKey
{
public:
    QString request_id;
    Domain domain{Domain::Unknown};

    enum class ParseError {
        NotValidLink,
        GameMismatch,
    };
    using Result = boost::outcome_v2::result<TradeRequestKey, ParseError>;
    static Result fromUrl(const QString& userUrl, Game game);

    QString toUrl(Game game) const;
    QString toSearchUrl(Game game) const;
    QUrl toFetchUrl(Game game, const std::span<QString>& items) const;

    bool isValid() const { return !request_id.isEmpty() && domain < Domain::Unknown; }

    QJsonObject toJson() const;
    static TradeRequestKey fromJson(const QJsonObject& key_o);

    static Domain domainId(const QString& domain);
    static QString domainUrl(Domain domain_id);
    static QString gameUrl(Game game_id);
};

inline bool operator==(const TradeRequestKey& l, const TradeRequestKey& r)
{
    return l.request_id == r.request_id && l.domain == r.domain;
}
inline bool operator<(const TradeRequestKey& l, const TradeRequestKey& r)
{
    return std::tie(l.request_id, l.domain) < std::tie(r.request_id, r.domain);
}
} // namespace planner

namespace std {
template<>
struct hash<planner::TradeRequestKey>
{
    std::size_t operator()(const planner::TradeRequestKey& key, size_t seed = 0) const noexcept
    {
        return qHashMulti(seed, key.request_id, key.domain);
    }
};
} // namespace std

namespace boost {
template<>
struct hash<planner::TradeRequestKey>
{
    typedef planner::TradeRequestKey argument_type;
    typedef std::size_t result_type;

    std::size_t operator()(const planner::TradeRequestKey& key, size_t seed = 0) const noexcept
    {
        return qHashMulti(seed, key.request_id, key.domain);
    }
};
} // namespace boost

#endif // TRADEREQUESTKEY_H
