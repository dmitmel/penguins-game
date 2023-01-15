#include "movement.h"
#include "board.h"
#include "game.h"
#include "io.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>

void movement_begin(Game* game) {
  assert(game->phase == GAME_PHASE_PLACEMENT_DONE);
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
    if (any_valid_player_move_exists(game, game_get_player(game, index)->id)) {
      game->current_player_index = index;
      return index;
    }
    checked_players++;
  }
  return -1;
}

bool any_valid_player_move_exists(const Game* game, int player_id) {
  assert(game->phase == GAME_PHASE_MOVEMENT);
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      if (get_tile(game, coords) != -player_id) continue;
      PossibleMoves moves = calculate_all_possible_moves(game, coords);
      if (moves.steps_up != 0 || moves.steps_right != 0 || moves.steps_down != 0 || moves.steps_left != 0) {
        return true;
      }
    }
  }
  return false;
}

MovementError validate_movement(const Game* game, Coords start, Coords target, Coords* fail) {
  assert(game->phase == GAME_PHASE_MOVEMENT);
  int tile = is_tile_in_bounds(game, start) ? get_tile(game, start) : 0;
  if (!is_tile_in_bounds(game, target)) {
    return OUT_OF_BOUNDS_MOVEMENT;
  } else if (-tile != game_get_current_player_id(game)) {
    return NOT_YOUR_PENGUIN;
  } else if (target.x == start.x && target.y == start.y) {
    return CURRENT_LOCATION;
  } else if (target.x != start.x && target.y != start.y) {
    return DIAGONAL_MOVE;
  } else if (get_tile(game, target) == 0) {
    return MOVE_ONTO_EMPTY_TILE;
  } else if (get_tile(game, target) < 0) {
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
    if (tile == 0) {
      return MOVE_OVER_EMPTY_TILE;
    } else if (tile < 0) {
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
    if (get_tile(game, coords) <= 0) break;
    steps++;
  }
  return steps;
}

PossibleMoves calculate_all_possible_moves(const Game* game, Coords start) {
  assert(game->phase == GAME_PHASE_MOVEMENT);
  assert(is_tile_in_bounds(game, start));
  PossibleMoves result = {
    .steps_up = get_possible_steps_in_direction(game, start, 0, -1),
    .steps_right = get_possible_steps_in_direction(game, start, 1, 0),
    .steps_down = get_possible_steps_in_direction(game, start, 0, 1),
    .steps_left = get_possible_steps_in_direction(game, start, -1, 0),
  };
  return result;
}

void move_penguin(Game* game, Coords start, Coords target) {
  assert(game->phase == GAME_PHASE_MOVEMENT);
  assert(validate_movement(game, start, target, NULL) == VALID_INPUT);
  Player* player = game_get_player(game, game->current_player_index);
  int fish = get_tile(game, target);
  assert(fish > 0);
  set_tile(game, target, -player->id);
  player->points += fish;
  set_tile(game, start, 0);
}

void handle_movement_input(Game* game, Coords* penguin, Coords* target) {
  while (true) {
    get_data_for_movement(penguin, target);
    MovementError input = validate_movement(game, *penguin, *target, NULL);
    switch (input) {
    case VALID_INPUT:
      return;
    case OUT_OF_BOUNDS_MOVEMENT:
      display_error_message("You cant move oustide the board!");
      break;
    case CURRENT_LOCATION:
      display_error_message("Thats your current location");
      break;
    case DIAGONAL_MOVE:
      display_error_message("You cant move diagonaly!");
      break;
    case NOT_YOUR_PENGUIN:
      display_error_message("Chose YOUR PENGUIN for movement");
      break;
    case MOVE_ONTO_EMPTY_TILE:
      display_error_message("Can't move onto an empty tile");
      break;
    case MOVE_ONTO_PENGUIN:
      display_error_message("Can't move onto another penguin!");
      break;
    case MOVE_OVER_EMPTY_TILE:
      display_error_message("You cant move over an empty tile!");
      break;
    case MOVE_OVER_PENGUIN:
      display_error_message("You cant move over another penguin!");
      break;
    }
  }
}

void interactive_movement(Game* game) {
  Coords target = { 0, 0 };
  Coords penguin = { 0, 0 };
  movement_begin(game);
  while (true) {
    int result = movement_switch_player(game);
    if (result < 0) break;
    display_new_turn_message(game->current_player_index + 1);
    handle_movement_input(game, &penguin, &target);
    move_penguin(game, penguin, target);
    update_game_state_display(game);
  }
  movement_end(game);
}
