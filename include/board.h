#pragma once

#include "game.h"
#include "utils.h"
#include <assert.h>
#include <stdbool.h>

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

enum TileAttribute {
  TILE_DIRTY = 1 << 1,
};

void setup_board(Game* game, int width, int height);
void generate_board_random(Game* game);
void generate_board_island(Game* game);

inline ALWAYS_INLINE bool is_tile_in_bounds(const Game* game, Coords coords) {
  int x = coords.x, y = coords.y;
  return 0 <= x && x < game->board_width && 0 <= y && y < game->board_height;
}

inline ALWAYS_INLINE bool get_tile_attr(const Game* game, Coords coords, int attr) {
  assert(is_tile_in_bounds(game, coords));
  return (game->tile_attributes[coords.x + game->board_width * coords.y] & attr) != 0;
}

inline ALWAYS_INLINE void set_tile_attr(Game* game, Coords coords, int attr, bool value) {
  assert(is_tile_in_bounds(game, coords));
  int* ptr = &game->tile_attributes[coords.x + game->board_width * coords.y];
  *ptr = (*ptr & ~attr) | (value ? attr : 0);
}

inline ALWAYS_INLINE void set_all_tiles_attr(Game* game, int attr, bool value) {
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      set_tile_attr(game, coords, attr, value);
    }
  }
}

inline ALWAYS_INLINE int get_tile(const Game* game, Coords coords) {
  assert(is_tile_in_bounds(game, coords));
  return game->board_grid[coords.x + game->board_width * coords.y];
}

inline ALWAYS_INLINE void set_tile(Game* game, Coords coords, int value) {
  assert(is_tile_in_bounds(game, coords));
  game->board_grid[coords.x + game->board_width * coords.y] = value;
  set_tile_attr(game, coords, TILE_DIRTY, true);
}

#ifdef __cplusplus
}
#endif
