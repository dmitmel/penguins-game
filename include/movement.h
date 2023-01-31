#pragma once

#include "board.h"
#include "game.h"
#include "utils.h"
#include <assert.h>

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
int move_penguin(Game* game, Coords start, Coords target);
void undo_move_penguin(Game* game, Coords start, Coords target, int prev_target_tile);

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

#ifdef __cplusplus
}
#endif
