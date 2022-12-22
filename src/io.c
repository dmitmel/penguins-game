#include "io.h"
#include <stdio.h>
#include <stdlib.h>

#include "gamestate.h"
#include "board.h"

void print_board(Board* board) {
  printf("   ");
  for (int x = 0; x < board->width; x++) {
    if (x % 5 == 0) {
      printf("%3d", x);
    } else {
      printf("   ");
    }
  }
  printf("\n");

  for (int y = 0; y < board->height; y++) {
    if (y % 5 == 0) {
      printf("%3d", y);
    } else {
      printf("   ");
    }
    printf("| ");
    for (int x = 0; x < board->width; x++) {
      int val = board->grid[y][x];
      if (val == 0) {
        printf("-  ");
      } else if (val < 0) {
        printf("p%d ", -val);
      } else {
        printf("%d  ", val);
      }
    }
    printf("|\n");
  }
}

void get_board_dimensions(int* width, int* height) {
  printf("Please specify width and height of the board\n");
  printf("Eg.:' 10 5 -> width=10, height=5\n");
  scanf("%d %d", width, height);
}

void get_players_count(int* count) {
  printf("Please input number of players:\n");
  scanf("%d", count);
}

void get_penguin_count(int* count) {
  printf("Please input number of penguins per player:\n");
  scanf("%d", count);
}

void get_player_name(int player_number, char* name) {
  printf("Player %d, please input name:\n", player_number);
  // TODO: guard against name being >15 characters
  scanf("%s", name);
}

void ask_player_for_input(int player_number){
  printf("Player %d, please input x and y coordinates to place the penguin:\n", player_number);
}

void get_penguin_coordinates(int* x, int* y) {
  scanf("%d %d", x, y);
}

void display_new_turn_message(int player_number) {
  printf("\nPlayer %d's turn.\n", player_number);
}

void display_error_message(char* message) {
  printf("\n%s\n", message);
}

void print_player_stats(Player players[], int count) {
  printf("id\t| name\t| score\n");
  for (int i = 0; i < count; i++) {
    printf("%d\t| %s\t| %d\n", players[i].id, players[i].name, players[i].points);
  }
}

void update_game_state_display(Board* board, Player players[], int player_count) {
  system("clear");
  print_player_stats(players, player_count);
  print_board(board);
}

void clear_screen() {
#ifdef WIN32
  system("cls");
#else
  system("clear");
#endif
}

void print_end_placement_phase(Board* board, Player players[], int player_count) {
  clear_screen();
  printf("No more penguins can be placed, placement phase ended!\n");
  print_board(board);
  printf("\n");
  print_player_stats(players, player_count);
}


void get_data_for_movement(Coords* start, Coords* target)
{
  printf("Chose a penguin\n");
  scanf("%d %d", &(start->x), &(start->y));
  printf("Where do you want to move?\n");
  scanf("%d %d", &(target->x), &(target->y));
}