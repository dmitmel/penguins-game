#pragma once

#include <stdbool.h>

#include "gamestate.h"

typedef struct Board {
  int width;
  int height;
  int** grid;
} Board;

typedef enum CheckedTile {
  EMPTY,
  PENGUIN,
  VALID_TILE
} CheckedTile;


typedef enum TileType {
  EMPTY_TILE,
  PENGUIN_OWN,
  PENGUIN_ENEMY,
  FISH_MULTIPLE,
  FISH_SINGLE,
  OUT_OF_BOUNDS
} TileType;

Board init_board(int width, int height);

void free_board(Board* board);

void generate_random_board(Board* board);

bool placeable_spot_exists(Board* board);

bool any_valid_player_move_exists(Board* board, int player_id) ;

bool any_valid_movement_exists(Board* board, Player* players, int player_count);

bool movement_is_valid(Board* board, int start_x, int start_y, int target_x, int target_y);

int move_penguin(Board* board, int start_x, int start_y, int target_x, int target_y, int player_id);