#pragma once

void print_board(int** board, int ySize, int xSize);

void get_board_dimensions(int* width, int* height);

void get_players_count(int* count);

void get_penguin_count(int* count);

void get_player_name(int player_number, char* name);

void get_penguin_coordinates(int *x, int *y, int player_number);

void display_new_turn_message(int player_number);