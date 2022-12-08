#include <stdbool.h>
#include <stdlib.h>

#include "board.h"
#include "gamestate.h"
#include "random.h"

#include "io.h" // bad, remove this after

Board init_board(int width, int height) {
  int** grid = calloc(height, sizeof(int*));
  for (int i = 0; i < height; i++) {
    int* row = calloc(width, sizeof(int));
    grid[i] = row;
  }
  Board board = {
    .width = width,
    .height = height,
    .grid = grid,
  };
  return board;
}

void free_board(Board* board) {
  for (int i = 0; i < board->height; i++) {
    int* row = board->grid[i];
    free(row);
  }
  free(board->grid);
}

void generate_random_board(Board* board) {
  for (int y = 0; y < board->height; y++) {
    for (int x = 0; x < board->width; x++) {
      board->grid[y][x] = random_range(0, 3);
    }
  }
}

bool placeable_spot_exists(Board* board) {
  for (int y = 0; y < board->height; y++) {
    for (int x = 0; x < board->width; x++) {
      if (board->grid[y][x] == 1) {
        return true;
      }
    }
  }
  return false;
}

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

CheckedTile check_a_tile(int x, int y, Board* board) {
  int Tile = board->grid[y][x];
  if (Tile == 0) {
    return EMPTY;
  } else if (Tile < 0) {
    return PENGUIN;
  }
  return VALID_TILE;
}

bool movement_is_valid(Board* board, int start_x, int start_y, int target_x, int target_y) {
  // TODO no penguins or empty tiles in the way to interrupt the itended movement
  int x, y;
  int movement_start, movement_end;

  if (target_x != start_x) {
    if (start_x > target_x) {
      movement_start = target_x;
      movement_end = start_x -1;
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
  // TODO update the board
  // remove the tiles destroyed by the movement (set to 0)
  // set the target position as -player_id (board->grid[target_y][target_x] = -player_id)
  // count and then return the number of fish captured on the way
  int points_gained = board->grid[target_y][target_x];
  board->grid[target_y][target_x] = -player_id;
  board->grid[start_y][start_x] = 0;

  return points_gained;
}