#pragma once

#include "game.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum MovementError {
  VALID_INPUT = 0,
  OUT_OF_BOUNDS_MOVEMENT,
  CURRENT_LOCATION,
  DIAGONAL_MOVE,
  NOT_YOUR_PENGUIN,
  MOVE_ONTO_EMPTY_TILE,
  MOVE_ONTO_PENGUIN,
  MOVE_OVER_EMPTY_TILE,
  MOVE_OVER_PENGUIN,
} MovementError;

typedef struct PossibleSteps {
  int steps[DIRECTION_MAX];
} PossibleSteps;

void movement_begin(Game* game);
void movement_end(Game* game);

int movement_switch_player(Game* game);
bool any_valid_player_move_exists(const Game* game, int player_idx);
MovementError validate_movement(const Game* game, Coords start, Coords target, Coords* fail);
int count_obstructed_directions(const Game* game, Coords penguin);
PossibleSteps calculate_penguin_possible_moves(const Game* game, Coords start);
int move_penguin(Game* game, Coords start, Coords target);
void undo_move_penguin(Game* game, Coords start, Coords target, int prev_target_tile);

#ifdef __cplusplus
}
#endif
