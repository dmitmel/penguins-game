#include "placement.h"
#include "board.h"
#include "game.h"
#include "utils.h"
#include <assert.h>

void placement_begin(Game* game) {
  assert(game->phase >= GAME_PHASE_SETUP_DONE);
  game->phase = GAME_PHASE_PLACEMENT;
  game->current_player_index = -1;
}

void placement_end(Game* game) {
  assert(game->phase == GAME_PHASE_PLACEMENT);
  game->phase = GAME_PHASE_PLACEMENT_DONE;
}

int placement_switch_player(Game* game) {
  assert(game->phase == GAME_PHASE_PLACEMENT);
  if (!any_valid_placement_exists(game)) {
    return PLACEMENT_NO_MORE_FREE_TILES;
  }
  int index = game->current_player_index;
  int checked_players = 0;
  while (checked_players < game->players_count) {
    index = (index + 1) % game->players_count;
    if (game_get_player(game, index)->penguins < game->penguins_per_player) {
      game->current_player_index = index;
      return index;
    }
    checked_players++;
  }
  return PLACEMENT_ALL_PENGUINS_PLACED;
}

bool any_valid_placement_exists(const Game* game) {
  assert(game->phase == GAME_PHASE_PLACEMENT);
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      if (get_tile(game, coords) == 1) {
        return true;
      }
    }
  }
  return false;
}

PlacementError validate_placement(const Game* game, Coords target) {
  assert(game->phase == GAME_PHASE_PLACEMENT);
  if (!is_tile_in_bounds(game, target)) {
    return PLACEMENT_OUT_OF_BOUNDS;
  }
  int tile = get_tile(game, target);
  if (tile == 0) {
    return PLACEMENT_EMPTY_TILE;
  } else if (tile < 0) {
    if (-tile == game_get_current_player_id(game)) {
      return PLACEMENT_OWN_PENGUIN;
    } else {
      return PLACEMENT_ENEMY_PENGUIN;
    }
  } else if (tile > 1) {
    return PLACEMENT_MULTIPLE_FISH;
  }
  return PLACEMENT_VALID;
}

void place_penguin(Game* game, Coords target) {
  assert(game->phase == GAME_PHASE_PLACEMENT);
  assert(validate_placement(game, target) == PLACEMENT_VALID);
  Player* player = game_get_player(game, game->current_player_index);
  int fish = get_tile(game, target);
  assert(fish > 0);
  set_tile(game, target, -player->id);
  player->penguins++;
  player->points += fish;
}
