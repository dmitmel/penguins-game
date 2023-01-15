#pragma once

#include "game.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

void setup_board(Game* game, int width, int height);

bool is_tile_in_bounds(const Game* game, Coords coords);
int get_tile(const Game* game, Coords coords);
void set_tile(Game* game, Coords coords, int value);

void generate_random_board(Game* game);

#ifdef __cplusplus
}
#endif
