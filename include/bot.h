#pragma once

#include "game.h"
#include "movement.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool bot_make_placement(Game* game);
int bot_rate_placement(const Game* game, Coords penguin);

typedef struct BotMove {
  Coords penguin;
  Coords target;
} BotMove;

bool bot_make_move(Game* game);
int bot_rate_move(const Game* game, BotMove move);

int iterate_possible_moves_struct(Coords penguin, PossibleMoves moves, BotMove* list);
int iterate_possible_moves_steps(Coords penguin, int steps, int dx, int dy, BotMove* list);

#ifdef __cplusplus
}
#endif
