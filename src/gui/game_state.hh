#pragma once

#include "board.h"

enum BoardGenType {
  BOARD_GEN_RANDOM,
  BOARD_GEN_ISLAND,
  BOARD_GEN_MAX,
};

enum PlayerType {
  PLAYER_NORMAL,
  PLAYER_BOT,
  PLAYER_TYPE_MAX,
};

/// @see TileAttribute
enum GuiTileAttribute {
  TILE_BLOCKED = TILE_ATTR_MAX,
  TILE_WAS_BLOCKED,
  TILE_BLOCKED_FOR_CURSOR,
  TILE_NEEDS_REDRAW,
  TILE_OVERLAY_NEEDS_REDRAW,
  GUI_TILE_ATTR_MAX,
};
