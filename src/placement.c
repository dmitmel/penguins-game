#include "placement.h"
#include "board.h"
#include "game.h"
#include "utils.h"
#include <assert.h>

void placement_begin(Game* game) {
  assert(game->phase >= GAME_PHASE_SETUP_DONE);
  game_set_current_player(game, -1);
  game_set_phase(game, GAME_PHASE_PLACEMENT);
}

void placement_end(Game* game) {
  assert(game->phase == GAME_PHASE_PLACEMENT);
  game_set_phase(game, GAME_PHASE_PLACEMENT_DONE);
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
    if (game_get_player(game, index)->penguins_count < game->penguins_per_player) {
      game_set_current_player(game, index);
      return index;
    }
    checked_players++;
  }
  return PLACEMENT_ALL_PENGUINS_PLACED;
}

bool any_valid_placement_exists(const Game* game) {
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      if (validate_placement_simple(game, coords)) {
        return true;
      }
    }
  }
  return false;
}

bool validate_placement_simple(const Game* game, Coords target) {
  short tile = get_tile(game, target);
  return get_tile_fish(tile) == 1;
}

PlacementError validate_placement(const Game* game, Coords target) {
  if (!is_tile_in_bounds(game, target)) {
    return PLACEMENT_OUT_OF_BOUNDS;
  }
  short tile = get_tile(game, target);
  if (is_water_tile(tile)) {
    return PLACEMENT_EMPTY_TILE;
  } else if (is_penguin_tile(tile)) {
    if (get_tile_player_id(tile) == game_get_current_player(game)->id) {
      return PLACEMENT_OWN_PENGUIN;
    } else {
      return PLACEMENT_ENEMY_PENGUIN;
    }
  } else if (is_fish_tile(tile)) {
    if (get_tile_fish(tile) > 1) {
      return PLACEMENT_MULTIPLE_FISH;
    }
  }
  return PLACEMENT_VALID;
}

void place_penguin(Game* game, Coords target) {
  assert(game->phase == GAME_PHASE_PLACEMENT);
  assert(validate_placement(game, target) == PLACEMENT_VALID);
  Player* player = game_get_current_player(game);
  short tile = get_tile(game, target);
  assert(is_fish_tile(tile));

  GameLogPlacement* entry = &game_push_log_entry(game, GAME_LOG_ENTRY_PLACEMENT)->data.placement;
  entry->target = target;
  entry->undo_tile = tile;

  game_add_player_penguin(game, game->current_player_index, target);
  set_tile(game, target, PENGUIN_TILE(player->id));
  player->points += get_tile_fish(tile);
  player->moves_count += 1;
}

void undo_place_penguin(Game* game) {
  assert(game->phase == GAME_PHASE_PLACEMENT);
  const GameLogPlacement* entry =
    &game_pop_log_entry(game, GAME_LOG_ENTRY_PLACEMENT)->data.placement;

  Player* player = game_get_current_player(game);
  game_remove_player_penguin(game, game->current_player_index, entry->target);
  set_tile(game, entry->target, entry->undo_tile);
  player->points -= get_tile_fish(entry->undo_tile);
  player->moves_count -= 1;
}
