#include "movement.h"
#include "board.h"
#include "io.h"
#include "gamestate.h"

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

CheckedTile check_a_tile(int x, int y, Board* board) {
  int Tile = board->grid[y][x];
  if (Tile == 0) {
    return EMPTY;
  } else if (Tile < 0) {
    return PENGUIN;
  }
  return VALID_TILE;
}

MovementInput check_movement_input(
  int target_x, int target_y, int start_x, int start_y, Board* board, Player* current_player
) {
  int tile = board->grid[start_y][start_x];
  if (target_x < 0 || target_x >= board->width || target_y < 0 || target_y >= board->height) {
    return OUT_OF_BOUNDS_MOVEMENT;
  } else if (target_x == start_x && target_y == start_y) {
    return CURRENT_LOCATION;
  } else if (target_x != start_x && target_y != start_y) {
    return DIAGONAL_MOVE;
  } else if (-tile != current_player->id) {
    return NOT_YOUR_PENGUIN;
  } else if (board->grid[target_y][target_x]==0){
    return EMPTY_FLOE;
  }
  return VALID_INPUT;
}

bool movement_is_valid(Board* board, int start_x, int start_y, int target_x, int target_y) {
  // TODO no penguins or empty tiles in the way to interrupt the itended movement
  int x, y;
  int movement_start, movement_end;

  if (target_x != start_x) {
    if (start_x > target_x) {
      movement_start = target_x;
      movement_end = start_x - 1;
    } else {
      movement_start = start_x + 1;
      movement_end = target_x;
    }
    for (x = movement_start; x <= movement_end; x++) {
      CheckedTile tile = check_a_tile(x, start_y, board);
      switch (tile) {
      case EMPTY:
        display_error_message("You cant move over an empty tile!");
        return false;
        break;
      case PENGUIN:
        display_error_message("You cant move over another penguin!");
        return false;
        break;
      case VALID_TILE:
        break;
      }
    }
  } else {
    if (start_y > target_y) {
      movement_start = target_y;
      movement_end = start_y - 1;
    } else {
      movement_start = start_y + 1;
      movement_end = target_y;
    }
    for (y = movement_start; y <= movement_end; y++) {
      CheckedTile tile = check_a_tile(start_x, y, board);
      switch (tile) {
      case EMPTY:
        display_error_message("You cant move over an empty tile!");
        return false;
      case PENGUIN:
        display_error_message("You cant move over another penguin!");
        return false;
      case VALID_TILE:
        break;
      }
    }
  }

  return true;
}


// returns the number of fish captured on the way
int move_penguin(
  Board* board, int start_x, int start_y, int target_x, int target_y, int player_id
) {
  int points_gained = board->grid[target_y][target_x];
  board->grid[target_y][target_x] = -player_id;
  board->grid[start_y][start_x] = 0;

  return points_gained;
}

void handle_movement_input(
  int* penguin_x,
  int* penguin_y,
  int* target_x,
  int* target_y,
  Board* board,
  Player* current_player,
  int player_count
) {
  while (true) {
    get_data_for_movement(penguin_x, penguin_y, target_x, target_y);
    MovementInput input =
      check_movement_input(*target_x, *target_y, *penguin_x, *penguin_y, board, current_player);
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
    case EMPTY_FLOE:
      display_error_message("Can't move onto an empty tile");
      break;
    case VALID_INPUT:
      if (movement_is_valid(board, *penguin_x, *penguin_y, *target_x, *target_y)) {
        return;
      }
      break;
    }
  }
  // TODO: look how handle_placement_input is handled
  // use movement_is_valid() method to check if the movement is valid
  // only return (exit) from this method if the movement is valid

  return;
}

void interactive_movement(Board* board, Player player_data[], int player_count) {
  int current_player = 0;
  int x;
  int y;
  int penguin_x;
  int penguin_y;
  int points_gained;
  while (any_valid_movement_exists(board, player_data, player_count)) {
    if (!any_valid_player_move_exists(board, current_player)) {
      display_error_message("No valid moves for the player");
    }
    display_new_turn_message(player_data[current_player].id);
    handle_movement_input(
      &penguin_x, &penguin_y, &x, &y, board, &player_data[current_player], player_count
    );
    // after this function call we have:
    // the x and y of the target tile and penguin_x and penguin_y of the penguin moving
    // and know that the movement is valid
    points_gained =
      move_penguin(board, penguin_x, penguin_y, x, y, player_data[current_player].id);
    player_data[current_player].points += points_gained;
    update_game_state_display(board, player_data, player_count);
    current_player = (current_player + 1) % player_count;
  }
}