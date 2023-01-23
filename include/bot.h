#pragma once

#include "arguments.h"
#include "game.h"
#include "movement.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BotMove {
  Coords penguin;
  Coords target;
} BotMove;

typedef struct BotState {
  const BotParameters* params;
  Game* game;

  size_t tile_coords_cap;
  Coords* tile_coords;
  size_t tile_scores_cap;
  int* tile_scores;

  size_t possible_steps_cap;
  PossibleSteps* possible_steps;
  size_t all_moves_cap;
  BotMove* all_moves;
  size_t move_scores_cap;
  int* move_scores;

  struct BotState* sub_state;
  int depth;
} BotState;

bool bot_make_placement(BotState* self);
int bot_rate_placement(BotState* self, Coords penguin);

BotState* bot_state_new(const BotParameters* params, Game* game);
void bot_state_free(BotState* self);
BotState* bot_enter_sub_state(BotState* self);

bool bot_make_move(BotState* self);
BotMove* bot_generate_all_moves_list(
  BotState* self, int penguins_count, Coords* penguins, int* moves_count
);
int* bot_rate_moves_list(BotState* self, int moves_count, BotMove* moves_list);
int bot_rate_move(BotState* self, BotMove move);

#ifdef __cplusplus
}
#endif
