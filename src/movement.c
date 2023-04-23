#include "movement.h"
#include "board.h"
#include "game.h"
#include "utils.h"
#include <assert.h>
#include <stddef.h> // IWYU pragma: keep

/// @relatesalso Game
/// @brief Enters the #GAME_PHASE_MOVEMENT phase, can only be called in
/// #GAME_PHASE_SETUP_DONE.
void movement_begin(Game* game) {
  assert(game->phase == GAME_PHASE_SETUP_DONE);
  game_set_current_player(game, -1);
  game_set_phase(game, GAME_PHASE_MOVEMENT);
}

/// @relatesalso Game
/// @brief Exits the #GAME_PHASE_MOVEMENT phase and switches to
/// #GAME_PHASE_SETUP_DONE.
void movement_end(Game* game) {
  assert(game->phase == GAME_PHASE_MOVEMENT);
  game_set_phase(game, GAME_PHASE_SETUP_DONE);
}

/// @relatesalso Game
/// @brief Performs the player switching logic for the movement phase.
///
/// Finds the next player who can make a move, sets #Game::current_player_index
/// to that player and returns their index. If no players can make any moves
/// returns a negative number.
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

/// @relatesalso Game
bool any_valid_player_move_exists(const Game* game, int player_idx) {
  Player* player = game_get_player(game, player_idx);
  for (int i = 0; i < player->penguins_count; i++) {
    if (count_obstructed_directions(game, player->penguins[i]) < DIRECTION_MAX) {
      return true;
    }
  }
  return false;
}

/// @relatesalso Game
MovementError validate_movement_start(const Game* game, Coords start) {
  if (!is_tile_in_bounds(game, start)) {
    return MOVEMENT_NOT_A_PENGUIN;
  }
  short start_tile = get_tile(game, start);
  if (!is_penguin_tile(start_tile)) {
    return MOVEMENT_NOT_A_PENGUIN;
  } else if (get_tile_player_id(start_tile) != game_get_current_player(game)->id) {
    return MOVEMENT_NOT_YOUR_PENGUIN;
  }
  if (count_obstructed_directions(game, start) < DIRECTION_MAX) {
    return MOVEMENT_VALID;
  } else {
    return MOVEMENT_PENGUIN_BLOCKED;
  }
}

/// @relatesalso Game
MovementError validate_movement(const Game* game, Coords start, Coords target, Coords* fail) {
  MovementError result = validate_movement_start(game, start);
  if (result != MOVEMENT_VALID) {
    return result;
  }

  if (!is_tile_in_bounds(game, target)) {
    return MOVEMENT_OUT_OF_BOUNDS;
  }
  if (target.x == start.x && target.y == start.y) {
    return MOVEMENT_CURRENT_LOCATION;
  } else if (target.x != start.x && target.y != start.y) {
    return MOVEMENT_DIAGONAL;
  }
  short target_tile = get_tile(game, target);
  if (is_water_tile(target_tile)) {
    return MOVEMENT_ONTO_EMPTY_TILE;
  } else if (is_penguin_tile(target_tile)) {
    return MOVEMENT_ONTO_PENGUIN;
  }

  Coords coords = start;
  int dx = target.x > start.x ? 1 : target.x < start.x ? -1 : 0;
  int dy = target.y > start.y ? 1 : target.y < start.y ? -1 : 0;
  while (coords.x != target.x || coords.y != target.y) {
    coords.x += dx, coords.y += dy;
    if (fail) {
      *fail = coords;
    }
    short tile = get_tile(game, coords);
    if (is_water_tile(tile)) {
      return MOVEMENT_OVER_EMPTY_TILE;
    } else if (is_penguin_tile(tile)) {
      return MOVEMENT_OVER_PENGUIN;
    }
  }

  return MOVEMENT_VALID;
}

/// @relatesalso Game
/// @brief Creates a #GameLogMovement entry. The requested move must be valid.
void move_penguin(Game* game, Coords start, Coords target) {
  assert(game->phase == GAME_PHASE_MOVEMENT);
  assert(validate_movement(game, start, target, NULL) == MOVEMENT_VALID);
  Player* player = game_get_current_player(game);
  short target_tile = get_tile(game, target);
  assert(is_fish_tile(target_tile));

  GameLogEntry* entry;
  if ((entry = game_push_log_entry(game, GAME_LOG_ENTRY_MOVEMENT)) != NULL) {
    GameLogMovement* entry_data = &entry->data.movement;
    entry_data->penguin = start;
    entry_data->target = target;
    entry_data->undo_tile = target_tile;
  }

  *game_find_player_penguin(game, game->current_player_index, start) = target;
  set_tile(game, target, PENGUIN_TILE(player->id));
  set_tile(game, start, WATER_TILE);
  player->points += get_tile_fish(target_tile);
  player->moves_count += 1;
}

/// @relatesalso Game
/// @brief Removes a #GameLogMovement entry from the log and undoes it.
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

extern int count_obstructed_directions(const Game* game, Coords penguin);
extern PossibleSteps calculate_penguin_possible_moves(const Game* game, Coords start);
