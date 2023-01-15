#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "game.h"
#include "utils.h"

typedef enum PlacementError {
  PLACEMENT_VALID = 0,
  PLACEMENT_OUT_OF_BOUNDS,
  PLACEMENT_EMPTY_TILE,
  PLACEMENT_ENEMY_PENGUIN,
  PLACEMENT_OWN_PENGUIN,
  PLACEMENT_MULTIPLE_FISH,
} PlacementError;

typedef enum PlacementSwitchResult {
  PLACEMENT_ALL_PENGUINS_PLACED = -1,
  PLACEMENT_NO_MORE_FREE_TILES = -2,
} PlacementSwitchError;

void placement_begin(Game* game);
void placement_end(Game* game);

int placement_switch_player(Game* game);
bool any_valid_placement_exists(const Game* game);

PlacementError validate_placement(const Game* game, Coords target);
void place_penguin(Game* game, Coords target);

void handle_placement_input(Game* game, Coords* selected);
void interactive_placement(Game* game);

#ifdef __cplusplus
}
#endif
