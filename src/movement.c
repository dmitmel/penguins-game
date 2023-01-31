#include "movement.h"
#include "board.h"
#include "game.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>

extern int count_obstructed_directions(const Game* game, Coords penguin);
extern PossibleSteps calculate_penguin_possible_moves(const Game* game, Coords start);

void movement_begin(Game* game) {
  assert(game->phase >= GAME_PHASE_SETUP_DONE);
  game->phase = GAME_PHASE_MOVEMENT;
  game->current_player_index = -1;
}

void movement_end(Game* game) {
  assert(game->phase == GAME_PHASE_MOVEMENT);
  game->phase = GAME_PHASE_MOVEMENT_DONE;
}

int movement_switch_player(Game* game) {
  assert(game->phase == GAME_PHASE_MOVEMENT);
  int index = game->current_player_index;
  int checked_players = 0;
  while (checked_players < game->players_count) {
    index = (index + 1) % game->players_count;
    if (any_valid_player_move_exists(game, index)) {
      game->current_player_index = index;
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

int move_penguin(Game* game, Coords start, Coords target) {
  assert(game->phase == GAME_PHASE_MOVEMENT);
  assert(validate_movement(game, start, target, NULL) == VALID_INPUT);
  Player* player = game_get_current_player(game);
  int tile = get_tile(game, target);
  assert(is_fish_tile(tile));
  *game_find_player_penguin(game, game->current_player_index, start) = target;
  set_tile(game, target, PENGUIN_TILE(player->id));
  set_tile(game, start, WATER_TILE);
  player->points += get_tile_fish(tile);
  player->moves_count += 1;
  return tile;
}

void undo_move_penguin(Game* game, Coords start, Coords target, int prev_target_tile) {
  assert(game->phase == GAME_PHASE_MOVEMENT);
  int start_tile = get_tile(game, start);
  assert(is_water_tile(start_tile));
  Player* player = game_get_current_player(game);
  int target_tile = get_tile(game, target);
  assert(get_tile_player_id(target_tile) == player->id);
  *game_find_player_penguin(game, game->current_player_index, target) = start;
  set_tile(game, start, PENGUIN_TILE(player->id));
  set_tile(game, target, prev_target_tile);
  player->points -= get_tile_fish(prev_target_tile);
  player->moves_count -= 1;
}
