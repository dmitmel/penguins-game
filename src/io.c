#include "io.h"
#include "board.h"
#include "game.h"
#include <stdio.h>
#include <stdlib.h>

void print_board(const Game* game) {
  printf("   ");
  for (int x = 0; x < game->board_width; x++) {
    if (x % 5 == 0) {
      printf("%3d", x);
    } else {
      printf("   ");
    }
  }
  printf("\n");

  for (int y = 0; y < game->board_height; y++) {
    if (y % 5 == 0) {
      printf("%3d", y);
    } else {
      printf("   ");
    }
    printf("| ");
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      int tile = get_tile(game, coords);
      if (is_water_tile(tile)) {
        printf("-  ");
      } else if (is_penguin_tile(tile)) {
        printf("p%d ", get_tile_player_id(tile));
      } else if (is_fish_tile(tile)) {
        printf("%d  ", get_tile_fish(tile));
      } else {
        printf("   ");
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

void get_player_name(int player_number, char name[32]) {
  printf("Player %d, please input name:\n", player_number);
  scanf("%31s", name);
}

void ask_player_for_input(int player_number) {
  printf("Player %d, please input x and y coordinates to place the penguin:\n", player_number);
}

void get_penguin_coordinates(Coords* coords) {
  scanf("%d %d", &coords->x, &coords->y);
}

void display_new_turn_message(int player_number) {
  printf("\nPlayer %d's turn.\n", player_number);
}

void display_error_message(const char* message) {
  printf("\n%s\n", message);
}

static void print_player_stats(const Game* game) {
  printf("id\t| name\t| score\n");
  for (int i = 0; i < game->players_count; i++) {
    Player* player = game_get_player(game, i);
    printf("%d\t| %s\t| %d\n", i + 1, player->name, player->points);
  }
}

void update_game_state_display(const Game* game) {
  clear_screen();
  print_player_stats(game);
  print_board(game);
}

void clear_screen(void) {
#ifdef _WIN32
  system("cls");
#else
  system("clear");
#endif
}

void print_end_placement_phase(const Game* game) {
  clear_screen();
  printf("No more penguins can be placed, placement phase ended!\n");
  print_board(game);
  printf("\n");
  print_player_stats(game);
}

void get_data_for_movement(Coords* start, Coords* target) {
  printf("Chose a penguin\n");
  scanf("%d %d", &start->x, &start->y);
  printf("Where do you want to move?\n");
  scanf("%d %d", &target->x, &target->y);
}
