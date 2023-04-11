#pragma once

#include "game.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

int run_interactive_mode(void);

void print_board(const Game* game);
void print_player_stats(const Game* game);
void print_game_state(const Game* game);

void interactive_placement(Game* game);
void handle_placement_input(Game* game, Coords* selected);

void interactive_movement(Game* game);
void handle_movement_input(Game* game, Coords* penguin, Coords* target);

#ifdef __cplusplus
}
#endif
