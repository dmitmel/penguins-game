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

typedef struct PossibleMoves {
  int steps_up;
  int steps_right;
  int steps_down;
  int steps_left;
  int all_steps;
} PossibleMoves;

void movement_begin(Game* game);
void movement_end(Game* game);

int movement_switch_player(Game* game);
bool any_valid_player_move_exists(const Game* game, int player_idx);
MovementError validate_movement(const Game* game, Coords start, Coords target, Coords* fail);
PossibleMoves calculate_all_possible_moves(const Game* game, Coords start);
void move_penguin(Game* game, Coords start, Coords target);

#ifdef __cplusplus
}
#endif
