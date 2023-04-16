#pragma once

/// @file
/// @brief Functions for working with the game board (and the specifics of its encoding)

#include "game.h"
#include "utils.h"
#include <assert.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @name Macros for encoding/decoding board tiles
/// See #Game::board_grid for more info. Also note that since these are macros,
/// they unfortunately may evaluate their arguments multiple times, so don't do
/// stuff like <tt>FISH_TILE(rand() % 4)</tt> (if the parameter is evaluated
/// two times here, it will give two different random values).
/// @{
#define WATER_TILE 0
#define FISH_TILE(fish) ((fish) > 0 ? (fish) : 0)
#define PENGUIN_TILE(player_id) ((player_id) > 0 ? -(player_id) : 0)
#define is_water_tile(tile) ((tile) == 0)
#define is_fish_tile(tile) ((tile) > 0)
#define is_penguin_tile(tile) ((tile) < 0)
#define get_tile_fish(tile) ((tile) > 0 ? (tile) : 0)
#define get_tile_player_id(tile) ((tile) < 0 ? -(tile) : 0)
/// @}

/// @brief The list of attributes built into the common library.
///
/// The variants of this enum are indexes into the bits in the numeric values
/// that the tile attributes are, so they must be sequential numbers starting
/// at zero. Because of this property this enum can be "extended" by UIs as
/// follows:
///
/// @code{.c}
/// enum MyTileAttribute {
///   TILE_ABC = TILE_ATTR_MAX,
///   TILE_DEF,
///   TILE_XYZ,
///   MY_TILE_ATTR_MAX,
/// };
/// @endcode
///
/// @see #Game::tile_attributes
/// @see #GuiTileAttribute
enum TileAttribute {
  /// @brief Set when a tile is changed with #set_tile, can be unset by the UI.
  /// All tiles initially have this attribute on when the board is initialized.
  TILE_DIRTY,
  TILE_ATTR_MAX,
};

/// @relatesalso Game
/// @brief Sets #Game::board_width and #Game::board_height and allocates
/// #Game::board_grid and #Game::tile_attributes. Can only be called within
/// #GAME_PHASE_SETUP. The @c width and @c height values must be positive.
void setup_board(Game* game, int width, int height);

/// @brief Generates the board by setting every tile purely randomly. The
/// resulting board will look sort of like a maze.
void generate_board_random(Game* game, Rng* rng);

/// @brief Generates the board which looks sort of like a big icy island.
void generate_board_island(Game* game, Rng* rng);

/// @name Functions for accessing the board tiles and attributes
/// All of these are inline because they are used very very frequently in
/// virtually every module of the app, thus in computation-heavy code, such as
/// the bot, inlining those functions enables very efficient optimizations by
/// the compiler (the bot in particular has been sped up 2 times).
/// @{

/// @relatesalso Game
/// @brief Checks if the given @c coords are within the bounds of the board.
inline ALWAYS_INLINE bool is_tile_in_bounds(const Game* game, Coords coords) {
  int x = coords.x, y = coords.y;
  return 0 <= x && x < game->board_width && 0 <= y && y < game->board_height;
}

/// @relatesalso Game
/// @brief Checks whether the attribute @c attr of the tile at @c coords is set.
/// @see #Game::tile_attributes
inline ALWAYS_INLINE bool get_tile_attr(const Game* game, Coords coords, short attr) {
  assert(is_tile_in_bounds(game, coords));
  return test_bit(game->tile_attributes[coords.x + game->board_width * coords.y], 1 << attr);
}

/// @relatesalso Game
/// @brief Sets (or resets) the attribute @c attr of the tile at @c coords.
/// @see #Game::tile_attributes
inline ALWAYS_INLINE void set_tile_attr(Game* game, Coords coords, short attr, bool value) {
  assert(is_tile_in_bounds(game, coords));
  short* ptr = &game->tile_attributes[coords.x + game->board_width * coords.y];
  *ptr = change_bit(*ptr, 1 << attr, value);
}

/// @relatesalso Game
/// @brief Sets the attribute @c attr on all tiles.
/// @see #Game::tile_attributes
inline ALWAYS_INLINE void set_all_tiles_attr(Game* game, short attr, bool value) {
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      set_tile_attr(game, coords, attr, value);
    }
  }
}

/// @relatesalso Game
/// @brief Returns the value of the tile at @c coords. Fails if @c coords are
/// outside the bounds.
/// @see #Game::board_grid
inline ALWAYS_INLINE short get_tile(const Game* game, Coords coords) {
  assert(is_tile_in_bounds(game, coords));
  return game->board_grid[coords.x + game->board_width * coords.y];
}

/// @relatesalso Game
/// @brief Sets the value of the tile at @c coords (and also sets the attribute
/// #TILE_DIRTY). Fails if @c coords are outside the bounds.
/// @see #Game::board_grid
inline ALWAYS_INLINE void set_tile(Game* game, Coords coords, short value) {
  assert(is_tile_in_bounds(game, coords));
  game->board_grid[coords.x + game->board_width * coords.y] = value;
  set_tile_attr(game, coords, TILE_DIRTY, true);
}

/// @}

#ifdef __cplusplus
}
#endif
