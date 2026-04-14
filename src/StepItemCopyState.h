#ifndef STEPITEMCOPYSTATE_H
#define STEPITEMCOPYSTATE_H

#include "Game.h"
#include "StepItem.h"

namespace planner {

class StepItemCopyState
{
public:
    Game game{Game::Unknown};
    StepItem item;

    static bool haveCopy(Game game) { return state.game != Game::Unknown && game == state.game; }
    static StepItemCopyState state;
};

} // namespace planner

#endif // STEPITEMCOPYSTATE_H
