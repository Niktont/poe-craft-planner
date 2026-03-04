#ifndef GAME_H
#define GAME_H

#include <QString>

namespace planner {
enum class Game : unsigned char {
    Poe1,
    Poe2,
    Unknown,
};

QString gameStr(Game game);
Game gamefromStr(QString game_str);
}

#endif // GAME_H
