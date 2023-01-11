#include "movement.h"
#include "board.h"
#include "gamestate.h"
#include "io.h"
#include "utils.h"
#include <stdio.h>

bool any_valid_player_move_exists(const Board* board, int player_id) {
  // TODO the selected player can move their penguin
  // players are encoded on the board as an integer equal to -player_id
  // so a row [0, 1, -1, -2] means that player 1 and player 2 have a penguin on the right half of
  // the row
  for (int y = 0; y < board->height; y++) {
    for (int x = 0; x < board->width; x++) {
      int tile = board->grid[y][x];
      if (-tile == player_id) {
        PossibleMoves moves = calculate_all_possible_moves(board, (Coords){ x, y });
        if (moves.steps_up != 0 || moves.steps_right != 0 || moves.steps_down != 0 || moves.steps_left != 0) {
          return true;
        }
      }
    }
  }
  return false;
}

bool any_valid_movement_exists(const Board* board, const Player* players, int player_count) {
  for (int i = 0; i < player_count; i++) {
    if (any_valid_player_move_exists(board, players[i].id)) {
      return true;
    }
  }
  return false;
}

MovementError validate_movement(
  const Board* board, Coords start, Coords target, int current_player_id, Coords* fail
) {
  int tile = board->grid[start.y][start.x];
  if (target.x < 0 || target.x >= board->width || target.y < 0 || target.y >= board->height) {
    return OUT_OF_BOUNDS_MOVEMENT;
  } else if (-tile != current_player_id) {
    return NOT_YOUR_PENGUIN;
  } else if (target.x == start.x && target.y == start.y) {
    return CURRENT_LOCATION;
  } else if (target.x != start.x && target.y != start.y) {
    return DIAGONAL_MOVE;
  } else if (board->grid[target.y][target.x] == 0) {
    return MOVE_ONTO_EMPTY_TILE;
  } else if (board->grid[target.y][target.x] < 0) {
    return MOVE_ONTO_PENGUIN;
  }

  int x = start.x, y = start.y;
  int dx = target.x > start.x ? 1 : target.x < start.x ? -1 : 0;
  int dy = target.y > start.y ? 1 : target.y < start.y ? -1 : 0;
  while (x != target.x || y != target.y) {
    x += dx, y += dy;
    if (fail) {
      fail->x = x;
      fail->y = y;
    }
    int tile = board->grid[y][x];
    if (tile == 0) {
      return MOVE_OVER_EMPTY_TILE;
    } else if (tile < 0) {
      return MOVE_OVER_PENGUIN;
    }
  }

  return VALID_INPUT;
}

static int get_possible_steps_in_direction(const Board* board, Coords start, int dx, int dy) {
  int x = start.x, y = start.y;
  int steps = 0;
  while (true) {
    x += dx, y += dy;
    if (!(0 <= x && x < board->width && 0 <= y && y < board->height)) {
      break;
    }
    int tile = board->grid[y][x];
    if (tile <= 0) {
      break;
    }
    steps++;
  }
  return steps;
}

PossibleMoves calculate_all_possible_moves(const Board* board, Coords start) {
  PossibleMoves result = {
    .steps_up = get_possible_steps_in_direction(board, start, 0, -1),
    .steps_right = get_possible_steps_in_direction(board, start, 1, 0),
    .steps_down = get_possible_steps_in_direction(board, start, 0, 1),
    .steps_left = get_possible_steps_in_direction(board, start, -1, 0),
  };
  return result;
}

// returns the number of fish captured on the way
int move_penguin(Board* board, Coords start, Coords target, int player_id) {
  int points_gained = board->grid[target.y][target.x];
  board->grid[target.y][target.x] = -player_id;
  board->grid[start.y][start.x] = 0;

  return points_gained;
}

void handle_movement_input(Coords* penguin, Coords* target, Board* board, Player* current_player) {
  while (true) {
    get_data_for_movement(penguin, target);
    MovementError input = validate_movement(board, *penguin, *target, current_player->id, NULL);
    switch (input) {
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
    case VALID_INPUT:
      return;
    }
  }
}

void interactive_movement(Board* board, GameState* state) {
  int current_player_idx = 0;
  Coords target;
  Coords penguin;
  int points_gained;
  while (any_valid_movement_exists(board, state->players, state->player_count)) {
    if (!any_valid_player_move_exists(board, current_player_idx)) {
      display_error_message("No valid moves for the player");
    }
    Player* current_player = &state->players[current_player_idx];
    display_new_turn_message(current_player->id);
    handle_movement_input(&penguin, &target, board, current_player);
    // after this function call we have:
    // the x and y of the target tile and penguin_x and penguin_y of the penguin moving
    // and know that the movement is valid
    points_gained = move_penguin(board, penguin, target, current_player->id);
    current_player->points += points_gained;
    update_game_state_display(board, state->players, state->player_count);
    current_player_idx = (current_player_idx + 1) % state->player_count;
  }
}
