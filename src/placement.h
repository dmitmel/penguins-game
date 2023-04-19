#pragma once

/// @file
/// @brief Placement phase functions

#include "game.h"
#include "utils.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PlacementError {
  PLACEMENT_VALID = 0,
  PLACEMENT_OUT_OF_BOUNDS,
  PLACEMENT_EMPTY_TILE,
  PLACEMENT_ENEMY_PENGUIN,
  PLACEMENT_OWN_PENGUIN,
  PLACEMENT_MULTIPLE_FISH,
} PlacementError;

typedef enum PlacementSwitchError {
  PLACEMENT_ALL_PENGUINS_PLACED = -1,
  PLACEMENT_NO_MORE_FREE_TILES = -2,
} PlacementSwitchError;

/// @name Placement phase
/// @{

/// @relatesalso Game
void placement_begin(Game* game);
/// @relatesalso Game
void placement_end(Game* game);

/// @relatesalso Game
int placement_switch_player(Game* game);
/// @relatesalso Game
bool any_valid_placement_exists(const Game* game);

/// @relatesalso Game
bool validate_placement_simple(const Game* game, Coords target);
/// @relatesalso Game
PlacementError validate_placement(const Game* game, Coords target);
/// @relatesalso Game
void place_penguin(Game* game, Coords target);
/// @relatesalso Game
void undo_place_penguin(Game* game);

/// @}

#ifdef __cplusplus
}
#endif
