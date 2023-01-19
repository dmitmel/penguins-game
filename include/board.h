#pragma once

#include "game.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WATER_TILE 0
#define FISH_TILE(fish) ((fish) > 0 ? (fish) : 0)
#define PENGUIN_TILE(player_id) ((player_id) > 0 ? -(player_id) : 0)
#define is_water_tile(tile) ((tile) == 0)
#define is_fish_tile(tile) ((tile) > 0)
#define is_penguin_tile(tile) ((tile) < 0)
#define get_tile_fish(tile) ((tile) > 0 ? (tile) : 0)
#define get_tile_player_id(tile) ((tile) < 0 ? -(tile) : 0)

void setup_board(Game* game, int width, int height);

bool is_tile_in_bounds(const Game* game, Coords coords);
int get_tile(const Game* game, Coords coords);
void set_tile(Game* game, Coords coords, int value);

void generate_board_random(Game* game);
void generate_board_island(Game* game);

#ifdef __cplusplus
}
#endif
