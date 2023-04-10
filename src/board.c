#include "board.h"
#include "game.h"
#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

extern bool is_tile_in_bounds(const Game* game, Coords coords);
extern bool get_tile_attr(const Game* game, Coords coords, short attr);
extern void set_tile_attr(Game* game, Coords coords, short attr, bool value);
extern void set_all_tiles_attr(Game* game, short attr, bool value);
extern short get_tile(const Game* game, Coords coords);
extern void set_tile(Game* game, Coords coords, short value);

void setup_board(Game* game, int width, int height) {
  assert(game->phase == GAME_PHASE_SETUP);
  assert(width > 0 && height > 0);
  free_and_clear(game->board_grid);
  free_and_clear(game->tile_attributes);
  game->board_width = width;
  game->board_height = height;
  game->board_grid = calloc(width * height, sizeof(*game->board_grid));
  game->tile_attributes = calloc(width * height, sizeof(*game->tile_attributes));
  set_all_tiles_attr(game, TILE_DIRTY, true);
}

void generate_board_random(Game* game, Rng* rng) {
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      short fish = (short)rng->random_range(rng, 0, 3);
      set_tile(game, coords, FISH_TILE(fish));
    }
  }
}

void generate_board_island(Game* game, Rng* rng) {
  int w = game->board_width, h = game->board_height;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      Coords coords = { x, y };
      set_tile(game, coords, WATER_TILE);
    }
  }
  for (int i = 0; i < w + h; i++) {
    Coords coords = { w / 2, h / 2 };
    for (int j = 0; j < w + h; j++) {
      switch (rng->random_range(rng, 0, 3)) {
        case 0: coords.x += 1; break;
        case 1: coords.y += 1; break;
        case 2: coords.x -= 1; break;
        case 3: coords.y -= 1; break;
      }
      if (is_tile_in_bounds(game, coords)) {
        short fish = (short)rng->random_range(rng, 1, 3);
        set_tile(game, coords, FISH_TILE(fish));
      } else {
        break;
      }
    }
  }
}
