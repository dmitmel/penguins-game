#include "board.h"
#include "random.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

void setup_board(Game* game, int width, int height) {
  assert(game->phase == GAME_PHASE_SETUP);
  assert(width > 0 && height > 0);
  free_and_clear(game->board_grid);
  game->board_width = width;
  game->board_height = height;
  game->board_grid = calloc(game->board_width * game->board_height, sizeof(int));
}

bool is_tile_in_bounds(const Game* game, Coords coords) {
  int x = coords.x, y = coords.y;
  return 0 <= x && x < game->board_width && 0 <= y && y < game->board_height;
}

int get_tile(const Game* game, Coords coords) {
  assert(is_tile_in_bounds(game, coords));
  return game->board_grid[coords.x + game->board_width * coords.y];
}

void set_tile(Game* game, Coords coords, int value) {
  assert(is_tile_in_bounds(game, coords));
  game->board_grid[coords.x + game->board_width * coords.y] = value;
}

void generate_random_board(Game* game) {
  assert(game->phase == GAME_PHASE_SETUP);
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      set_tile(game, coords, random_range(0, 3));
    }
  }
}
