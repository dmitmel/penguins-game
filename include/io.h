#pragma once

#include "game.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

void print_board(const Game* game);

void get_board_dimensions(int* width, int* height);

void get_players_count(int* count);

void get_penguin_count(int* count);

void get_player_name(int player_number, char name[32]);

void ask_player_for_input(int player_number);

void get_penguin_coordinates(Coords* coords);

void display_new_turn_message(int player_number);

void display_error_message(const char* message);

void update_game_state_display(const Game* game);

void clear_screen(void);

void print_end_placement_phase(const Game* game);

void get_data_for_movement(Coords* start, Coords* target);

#ifdef __cplusplus
}
#endif
