#pragma once

#include "board.h"
#include "gamestate.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

void print_board(const Board* board);

void get_board_dimensions(int* width, int* height);

void get_players_count(int* count);

void get_penguin_count(int* count);

void get_player_name(int player_number, char* name);

void ask_player_for_input(int player_number);

void get_penguin_coordinates(Coords* coords);

void display_new_turn_message(int player_number);

void display_error_message(const char* message);

void update_game_state_display(const Board* board, const Player players[], int player_count);

void clear_screen(void);

void print_end_placement_phase(const Board* board, const Player players[], int player_count);

void get_data_for_movement(Coords* start, Coords* target);

#ifdef __cplusplus
}
#endif
