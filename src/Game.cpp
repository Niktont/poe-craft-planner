#include "Game.h"
#include <array>

using namespace Qt::StringLiterals;

namespace planner {

static const std::array games_str{
    u"Poe1"_s,
    u"Poe2"_s,
};

QString gameStr(Game game)
{
    return games_str[static_cast<int>(game)];
}

Game gamefromStr(QString game_str)
{
    if (auto it = std::ranges::find(games_str, game_str); it != games_str.end())
        return static_cast<Game>(std::distance(games_str.begin(), it));
    return Game::Unknown;
}

} // namespace planner
