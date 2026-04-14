#ifndef STEPCOPYSTATE_H
#define STEPCOPYSTATE_H

#include "Game.h"
#include <QUuid>

namespace planner {
class StepCopyState
{
public:
    Game game{Game::Unknown};
    QUuid plan_id;
    QUuid step_id;

    static bool haveCopy(Game game) { return state.game != Game::Unknown && game == state.game; }
    static StepCopyState state;
};

} // namespace planner
#endif // STEPCOPYSTATE_H
