#include "movement.h"
#include "board.h"
#include "gamestate.h"
#include "io.h"
#include <stdio.h>

bool any_valid_player_move_exists(Board* board, int player_id) {
  // TODO the selected player can move their penguin
  // players are encoded on the board as an integer equal to -player_id
  // so a row [0, 1, -1, -2] means that player 1 and player 2 have a penguin on the right half of
  // the row
  return true;
}

bool any_valid_movement_exists(Board* board, Player* players, int player_count) {
  for (int i = 0; i < player_count; i++) {
    if (any_valid_player_move_exists(board, players[i].id)) {
      return true;
    }
  }
  return false;
}

MovementError
validate_movement(Board* board, Coords start, Coords target, int current_player_id, Coords* fail) {
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

// returns the number of fish captured on the way
int move_penguin(Board* board, Coords start, Coords target, int player_id) {
  int points_gained = board->grid[target.y][target.x];
  board->grid[target.y][target.x] = -player_id;
  board->grid[start.y][start.x] = 0;

  return points_gained;
}

void handle_movement_input(
  Coords* penguin, Coords* target, Board* board, Player* current_player, int player_count
) {
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

void interactive_movement(Board* board, Player player_data[], int player_count) {
  int current_player = 0;
  Coords target;
  Coords penguin;
  int points_gained;
  while (any_valid_movement_exists(board, player_data, player_count)) {
    if (!any_valid_player_move_exists(board, current_player)) {
      display_error_message("No valid moves for the player");
    }
    display_new_turn_message(player_data[current_player].id);
    handle_movement_input(&penguin, &target, board, &player_data[current_player], player_count);
    // after this function call we have:
    // the x and y of the target tile and penguin_x and penguin_y of the penguin moving
    // and know that the movement is valid
    points_gained = move_penguin(board, penguin, target, player_data[current_player].id);
    player_data[current_player].points += points_gained;
    update_game_state_display(board, player_data, player_count);
    current_player = (current_player + 1) % player_count;
  }
}