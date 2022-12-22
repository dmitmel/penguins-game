#pragma once

#include "board.h"
#include "gamestate.h"

typedef enum MovementInput {
  OUT_OF_BOUNDS_MOVEMENT,
  CURRENT_LOCATION,
  DIAGONAL_MOVE,
  NOT_YOUR_PENGUIN,
  VALID_INPUT,
  EMPTY_FLOE
} MovementInput;

bool any_valid_player_move_exists(Board* board, int player_id);

bool any_valid_movement_exists(Board* board, Player* players, int player_count);

CheckedTile check_a_tile(int x, int y, Board* board);

MovementInput check_movement_input(
  int target_x, int target_y, int start_x, int start_y, Board* board, Player* current_player
);

bool movement_is_valid(Board* board, int start_x, int start_y, int target_x, int target_y);

void handle_movement_input(
  int* penguin_x,
  int* penguin_y,
  int* target_x,
  int* target_y,
  Board* board,
  Player* current_player,
  int player_count
);

int move_penguin(
  Board* board, int start_x, int start_y, int target_x, int target_y, int player_id
);

void interactive_movement(Board* board, Player player_data[], int player_count);
