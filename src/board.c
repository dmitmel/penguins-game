#include "board.h"
#include "game.h"
#include "random.h"
#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

extern bool is_tile_in_bounds(const Game* game, Coords coords);
extern bool get_tile_attr(const Game* game, Coords coords, int attr);
extern void set_tile_attr(Game* game, Coords coords, int attr, bool value);
extern void set_all_tiles_attr(Game* game, int attr, bool value);
extern int get_tile(const Game* game, Coords coords);
extern void set_tile(Game* game, Coords coords, int value);

void setup_board(Game* game, int width, int height) {
  assert(game->phase == GAME_PHASE_SETUP);
  assert(width > 0 && height > 0);
  free_and_clear(game->board_grid);
  free_and_clear(game->tile_attributes);
  game->board_width = width;
  game->board_height = height;
  game->board_grid = calloc(game->board_width * game->board_height, sizeof(int));
  game->tile_attributes = calloc(game->board_width * game->board_height, sizeof(int));
  set_all_tiles_attr(game, TILE_DIRTY, true);
}

void generate_board_random(Game* game) {
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      int fish = random_range(0, 3);
      set_tile(game, coords, FISH_TILE(fish));
    }
  }
}

void generate_board_island(Game* game) {
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
      switch (random_range(0, 3)) {
        case 0: coords.x += 1; break;
        case 1: coords.y += 1; break;
        case 2: coords.x -= 1; break;
        case 3: coords.y -= 1; break;
      }
      if (is_tile_in_bounds(game, coords)) {
        int fish = random_range(1, 3);
        set_tile(game, coords, FISH_TILE(fish));
      } else {
        break;
      }
    }
  }
}
