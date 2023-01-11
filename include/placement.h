#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "board.h"
#include "gamestate.h"

TileType get_player_input_tile(Coords* target, const Board* board, const Player* current_player);

void handle_placement_input(
  Coords* selected, Board* board, const Player* current_player, int player_count
);

void interactive_placement(Board* board, GameState* gameState);

#ifdef __cplusplus
}
#endif