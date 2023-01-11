#pragma once

#include "utils.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Board {
  int width;
  int height;
  int** grid;
} Board;

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

bool placeable_spot_exists(const Board* board);

bool is_spot_valid_for_placement(const Board* board, Coords coords);

#ifdef __cplusplus
}
#endif
