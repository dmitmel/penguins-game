#pragma once

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

enum GuiTileAttribute {
  TILE_BLOCKED = 1 << 2,
  TILE_WAS_BLOCKED = 1 << 3,
  TILE_BLOCKED_FOR_CURSOR = 1 << 4,
  TILE_NEEDS_REDRAW = 1 << 5,
  TILE_OVERLAY_NEEDS_REDRAW = 1 << 6,
};
