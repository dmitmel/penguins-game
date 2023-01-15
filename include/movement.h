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
} PossibleMoves;

void movement_begin(Game* game);
void movement_end(Game* game);

int movement_switch_player(Game* game);
bool any_valid_player_move_exists(const Game* game, int player_id);
MovementError validate_movement(const Game* game, Coords start, Coords target, Coords* fail);
PossibleMoves calculate_all_possible_moves(const Game* game, Coords start);
void move_penguin(Game* game, Coords start, Coords target);

void handle_movement_input(Game* game, Coords* penguin, Coords* target);
void interactive_movement(Game* game);

#ifdef __cplusplus
}
#endif
