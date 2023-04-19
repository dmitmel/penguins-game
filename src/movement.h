#pragma once

/// @file
/// @brief Movement phase functions

#include "board.h"
#include "game.h"
#include "utils.h"
#include <assert.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum MovementError {
  MOVEMENT_VALID = 0,
  MOVEMENT_OUT_OF_BOUNDS,
  MOVEMENT_CURRENT_LOCATION,
  MOVEMENT_DIAGONAL,
  MOVEMENT_NOT_A_PENGUIN,
  MOVEMENT_NOT_YOUR_PENGUIN,
  MOVEMENT_ONTO_EMPTY_TILE,
  MOVEMENT_ONTO_PENGUIN,
  MOVEMENT_OVER_EMPTY_TILE,
  MOVEMENT_OVER_PENGUIN,
  MOVEMENT_PENGUIN_BLOCKED,
} MovementError;

typedef struct PossibleSteps {
  int steps[DIRECTION_MAX];
} PossibleSteps;

/// @name Movement phase
/// @{

/// @relatesalso Game
void movement_begin(Game* game);
/// @relatesalso Game
void movement_end(Game* game);

/// @relatesalso Game
int movement_switch_player(Game* game);
/// @relatesalso Game
bool any_valid_player_move_exists(const Game* game, int player_idx);
/// @relatesalso Game
MovementError validate_movement_start(const Game* game, Coords start);
/// @relatesalso Game
MovementError validate_movement(const Game* game, Coords start, Coords target, Coords* fail);
/// @relatesalso Game
void move_penguin(Game* game, Coords start, Coords target);
/// @relatesalso Game
void undo_move_penguin(Game* game);

/// @relatesalso Game
inline int count_obstructed_directions(const Game* game, Coords penguin) {
  assert(is_tile_in_bounds(game, penguin));
  int result = 0;
  for (int dir = 0; dir < DIRECTION_MAX; dir++) {
    Coords neighbor = DIRECTION_TO_COORDS[dir];
    neighbor.x += penguin.x, neighbor.y += penguin.y;
    if (!(is_tile_in_bounds(game, neighbor) && is_fish_tile(get_tile(game, neighbor)))) {
      result += 1;
    }
  }
  return result;
}

/// @relatesalso Game
inline PossibleSteps calculate_penguin_possible_moves(const Game* game, Coords start) {
  assert(is_tile_in_bounds(game, start));
  PossibleSteps moves;
  for (int dir = 0; dir < DIRECTION_MAX; dir++) {
    Coords d = DIRECTION_TO_COORDS[dir];
    Coords target = start;
    int steps = 0;
    while (true) {
      target.x += d.x, target.y += d.y;
      if (!(is_tile_in_bounds(game, target) && is_fish_tile(get_tile(game, target)))) break;
      steps++;
    }
    moves.steps[dir] = steps;
  }
  return moves;
}

/// @}

#ifdef __cplusplus
}
#endif
