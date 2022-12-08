#pragma once

#include "stdbool.h"
#include "gamestate.h"

typedef struct Board {
  int width;
  int height;
  int** grid;
} Board;

typedef enum MovementInput{
  OUT_OF_BOUNDS_MOVEMENT,
  CURRENT_LOCATION,
  DIAGONAL_MOVE,
  NOT_YOUR_PENGUIN,
  VALID_INPUT,
  EMPTY_FLOE
}MovementInput;

typedef enum CheckedTile {
  EMPTY,
  PENGUIN,
  VALID_TILE
} CheckedTile;

Board init_board(int width, int height);

void free_board(Board* board);

void generate_random_board(Board* board);

bool placeable_spot_exists(Board* board);

bool any_valid_player_move_exists(Board* board, int player_id) ;

MovementInput check_movement_input(int target_x, int target_y, int start_x, int start_y, Board* board, Player* current_player);

bool any_valid_movement_exists(Board* board, Player* players, int player_count);

bool movement_is_valid(Board* board, int start_x, int start_y, int target_x, int target_y);

int move_penguin(Board* board, int start_x, int start_y, int target_x, int target_y, int player_id);