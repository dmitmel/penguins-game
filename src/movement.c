#include "movement.h"
#include "board.h"
#include "game.h"
#include "utils.h"
#include <assert.h>
#include <stddef.h> // IWYU pragma: keep

extern int count_obstructed_directions(const Game* game, Coords penguin);
extern PossibleSteps calculate_penguin_possible_moves(const Game* game, Coords start);

void movement_begin(Game* game) {
  assert(game->phase >= GAME_PHASE_SETUP_DONE);
  game_set_current_player(game, -1);
  game_set_phase(game, GAME_PHASE_MOVEMENT);
}

void movement_end(Game* game) {
  assert(game->phase == GAME_PHASE_MOVEMENT);
  game_set_phase(game, GAME_PHASE_MOVEMENT_DONE);
}

int movement_switch_player(Game* game) {
  assert(game->phase == GAME_PHASE_MOVEMENT);
  int index = game->current_player_index;
  int checked_players = 0;
  while (checked_players < game->players_count) {
    index = (index + 1) % game->players_count;
    if (any_valid_player_move_exists(game, index)) {
      game_set_current_player(game, index);
      return index;
    }
    checked_players++;
  }
  return -1;
}

bool any_valid_player_move_exists(const Game* game, int player_idx) {
  Player* player = game_get_player(game, player_idx);
  for (int i = 0; i < player->penguins_count; i++) {
    if (count_obstructed_directions(game, player->penguins[i]) < DIRECTION_MAX) {
      return true;
    }
  }
  return false;
}

bool validate_movement_start(const Game* game, Coords start) {
  if (!is_tile_in_bounds(game, start)) return false;
  int tile = get_tile(game, start);
  if (get_tile_player_id(tile) != game_get_current_player(game)->id) return false;
  return count_obstructed_directions(game, start) < DIRECTION_MAX;
}

MovementError validate_movement(const Game* game, Coords start, Coords target, Coords* fail) {
  int start_tile = is_tile_in_bounds(game, start) ? get_tile(game, start) : WATER_TILE;
  int target_tile = is_tile_in_bounds(game, target) ? get_tile(game, target) : WATER_TILE;
  if (!is_tile_in_bounds(game, target)) {
    return OUT_OF_BOUNDS_MOVEMENT;
  } else if (get_tile_player_id(start_tile) != game_get_current_player(game)->id) {
    return NOT_YOUR_PENGUIN;
  } else if (target.x == start.x && target.y == start.y) {
    return CURRENT_LOCATION;
  } else if (target.x != start.x && target.y != start.y) {
    return DIAGONAL_MOVE;
  } else if (is_water_tile(target_tile)) {
    return MOVE_ONTO_EMPTY_TILE;
  } else if (is_penguin_tile(target_tile)) {
    return MOVE_ONTO_PENGUIN;
  }

  Coords coords = start;
  int dx = target.x > start.x ? 1 : target.x < start.x ? -1 : 0;
  int dy = target.y > start.y ? 1 : target.y < start.y ? -1 : 0;
  while (coords.x != target.x || coords.y != target.y) {
    coords.x += dx, coords.y += dy;
    if (fail) {
      *fail = coords;
    }
    int tile = get_tile(game, coords);
    if (is_water_tile(tile)) {
      return MOVE_OVER_EMPTY_TILE;
    } else if (is_penguin_tile(tile)) {
      return MOVE_OVER_PENGUIN;
    }
  }

  return VALID_INPUT;
}

void move_penguin(Game* game, Coords start, Coords target) {
  assert(game->phase == GAME_PHASE_MOVEMENT);
  assert(validate_movement(game, start, target, NULL) == VALID_INPUT);
  Player* player = game_get_current_player(game);
  int target_tile = get_tile(game, target);
  assert(is_fish_tile(target_tile));

  GameLogMovement* entry = &game_push_log_entry(game, GAME_LOG_ENTRY_MOVEMENT)->data.movement;
  entry->penguin = start;
  entry->target = target;
  entry->undo_tile = target_tile;

  *game_find_player_penguin(game, game->current_player_index, start) = target;
  set_tile(game, target, PENGUIN_TILE(player->id));
  set_tile(game, start, WATER_TILE);
  player->points += get_tile_fish(target_tile);
  player->moves_count += 1;
}

void undo_move_penguin(Game* game) {
  assert(game->phase == GAME_PHASE_MOVEMENT);
  const GameLogMovement* entry = &game_pop_log_entry(game, GAME_LOG_ENTRY_MOVEMENT)->data.movement;

  Player* player = game_get_current_player(game);
  *game_find_player_penguin(game, game->current_player_index, entry->target) = entry->penguin;
  set_tile(game, entry->penguin, PENGUIN_TILE(player->id));
  set_tile(game, entry->target, entry->undo_tile);
  player->points -= get_tile_fish(entry->undo_tile);
  player->moves_count -= 1;
}
