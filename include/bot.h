#pragma once

#include "arguments.h"
#include "game.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool bot_make_placement(const BotParameters* params, Game* game);
int bot_rate_placement(const BotParameters* params, const Game* game, Coords penguin);

typedef struct BotMove {
  Coords penguin;
  Coords target;
} BotMove;

bool bot_make_move(const BotParameters* params, Game* game);
int bot_rate_move(const BotParameters* params, const Game* game, BotMove move);

BotMove* generate_all_possible_moves_list(
  const BotParameters* params, Game* game, int player_idx, int* moves_count
);

#ifdef __cplusplus
}
#endif
