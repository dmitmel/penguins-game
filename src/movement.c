#include "movement.h"
#include "board.h"
#include "game.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>

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
    Coords penguin = player->penguins[i];
    if (calculate_all_possible_moves(game, penguin).all_steps != 0) {
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

static int get_possible_steps_in_direction(const Game* game, Coords coords, int dx, int dy) {
  int steps = 0;
  while (true) {
    coords.x += dx, coords.y += dy;
    if (!is_tile_in_bounds(game, coords)) break;
    if (!is_fish_tile(get_tile(game, coords))) break;
    steps++;
  }
  return steps;
}

PossibleMoves calculate_all_possible_moves(const Game* game, Coords start) {
  assert(is_tile_in_bounds(game, start));
  PossibleMoves result = {
    .steps_up = get_possible_steps_in_direction(game, start, 0, -1),
    .steps_right = get_possible_steps_in_direction(game, start, 1, 0),
    .steps_down = get_possible_steps_in_direction(game, start, 0, 1),
    .steps_left = get_possible_steps_in_direction(game, start, -1, 0),
  };
  result.all_steps = result.steps_up + result.steps_right + result.steps_down + result.steps_left;
  return result;
}

void move_penguin(Game* game, Coords start, Coords target) {
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
}
