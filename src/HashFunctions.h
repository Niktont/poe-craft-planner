#ifndef HASHFUNCTIONS_H
#define HASHFUNCTIONS_H
#include <boost/container_hash/hash.hpp>
#include <QString>
#include <QUuid>

namespace boost {
template<>
struct hash<QString>
{
    using argument_type = QString;
    using result_type = std::size_t;
    using is_avalanching = std::true_type;

    std::size_t operator()(const QString& val, size_t seed = 0) const { return qHash(val, seed); }
};
template<>
struct hash<QUuid>
{
    using argument_type = QUuid;
    using result_type = std::size_t;
    using is_avalanching = std::true_type;

    std::size_t operator()(const QUuid& val, size_t seed = 0) const { return qHash(val, seed); }
};
} // namespace boost
#endif // HASHFUNCTIONS_H
