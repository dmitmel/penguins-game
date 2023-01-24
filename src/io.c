#include "io.h"
#include "board.h"
#include "color.h"
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
        water_color();
        printf(" 0 ");
        reset_color();
      } else if (is_penguin_tile(tile)) {
        Player* current_player = game_get_player(game, get_tile_player_id(tile) - 1);
        player_color(current_player);
        printf("p%d ", get_tile_player_id(tile));
        reset_color();
      } else if (is_fish_tile(tile)) {
        ice_color();
        printf(" %d ", get_tile_fish(tile));
        reset_color();
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

void get_player_color(int player_number, int* color_choice) {
  printf(
    "Player %d select your color:\n\033[31m1 - red\n\033[32m2 - green\n\033[33m3 - "
    "yellow\n\033[34m4 - blue\n\033[35m5 - magenta\033[0m\n",
    player_number
  );
  do {
    scanf("%d", color_choice);
  } while (*color_choice < 1 || *color_choice > 5);
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
