#include "interactive.h"
#include "board.h"
#include "color.h"
#include "game.h"
#include "movement.h"
#include "placement.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static void clear_screen(void) {
#ifdef _WIN32
  system("cls");
#else
  system("clear");
#endif
}

static void print_board(const Game* game) {
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
        int player_idx = game_find_player_by_id(game, get_tile_player_id(tile));
        Player* player = game_get_player(game, player_idx);
        player_color(player);
        printf("p%d ", player_idx + 1);
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

static void display_new_turn_message(int player_number) {
  printf("\nPlayer %d's turn.\n", player_number);
}

static void display_error_message(const char* message) {
  printf("\n%s\n", message);
}

static void print_player_stats(const Game* game) {
  printf("id\t| name\t| score\n");
  for (int i = 0; i < game->players_count; i++) {
    Player* player = game_get_player(game, i);
    printf("%d\t| %s\t| %d\n", i + 1, player->name, player->points);
  }
}

static void update_game_state_display(const Game* game) {
  clear_screen();
  print_player_stats(game);
  print_board(game);
}

int run_interactive_mode(void) {
  clear_screen();

  Game* game = game_new();
  game_begin_setup(game);

  int players_count;
  printf("Please input number of players:\n");
  scanf("%d", &players_count);
  game_set_players_count(game, players_count);

  for (int i = 0; i < players_count; i++) {
    Player* player = game_get_player(game, i);

    char name[33];
    printf("Player %d, please input name:\n", i + 1);
    scanf("%32s", name);
    game_set_player_name(game, i, name);

    printf(
      "Player %d select your color:\n"
      "\033[31m1 - red\n"
      "\033[32m2 - green\n"
      "\033[33m3 - yellow\n"
      "\033[34m4 - blue\n"
      "\033[35m5 - magenta"
      "\033[0m\n",
      i + 1
    );
    int color_choice;
    do {
      scanf("%d", &color_choice);
    } while (!(1 <= color_choice && color_choice <= 5));
    player->color = color_choice;
  }

  int penguin_count;
  printf("Please input number of penguins per player:\n");
  scanf("%d", &penguin_count);
  game_set_penguins_per_player(game, penguin_count);

  int board_width;
  int board_height;
  printf("Please specify width and height of the board\n");
  printf("Eg.:' 10 5 -> width=10, height=5\n");
  scanf("%d %d", &board_width, &board_height);
  setup_board(game, board_width, board_height);
  generate_board_random(game);

  game_end_setup(game);

  update_game_state_display(game);
  interactive_placement(game);
  interactive_movement(game);
  game_end(game);

  game_free(game);

  return 0;
}

void interactive_placement(Game* game) {
  Coords target = { 0, 0 };
  placement_begin(game);
  while (true) {
    int result = placement_switch_player(game);
    if (result < 0) break;
    display_new_turn_message(game->current_player_index + 1);
    handle_placement_input(game, &target);
    place_penguin(game, target);
    update_game_state_display(game);
  }
  placement_end(game);
  clear_screen();
  printf("No more penguins can be placed, placement phase ended!\n");
  print_board(game);
  printf("\n");
  print_player_stats(game);
}

void handle_placement_input(Game* game, Coords* selected) {
  printf(
    "Player %d, please input x and y coordinates to place the penguin:\n",
    game->current_player_index + 1
  );
  while (true) {
    scanf("%d %d", &selected->x, &selected->y);
    switch (validate_placement(game, *selected)) {
      case PLACEMENT_VALID: return;
      case PLACEMENT_OUT_OF_BOUNDS:
        display_error_message("Inputted coordinates are outside the bounds of the board");
        break;
      case PLACEMENT_EMPTY_TILE:
        display_error_message("This tile is empty, you can't select an empty(water) tile");
        break;
      case PLACEMENT_ENEMY_PENGUIN:
        display_error_message("This tile is already occupied by a penguin");
        break;
      case PLACEMENT_OWN_PENGUIN:
        display_error_message("This tile is already occupied by a penguin");
        break;
      case PLACEMENT_MULTIPLE_FISH:
        display_error_message("Only a tile with just one fish can be selected");
        break;
      default:
        display_error_message("ERROR: what on god's green earth did you just select???");
        break;
    }
  }
}

void interactive_movement(Game* game) {
  Coords target = { 0, 0 };
  Coords penguin = { 0, 0 };
  movement_begin(game);
  while (true) {
    int result = movement_switch_player(game);
    if (result < 0) break;
    display_new_turn_message(game->current_player_index + 1);
    handle_movement_input(game, &penguin, &target);
    move_penguin(game, penguin, target);
    update_game_state_display(game);
  }
  movement_end(game);
}

void handle_movement_input(Game* game, Coords* penguin, Coords* target) {
  while (true) {
    printf("Chose a penguin\n");
    scanf("%d %d", &penguin->x, &penguin->y);
    printf("Where do you want to move?\n");
    scanf("%d %d", &target->x, &target->y);
    MovementError input = validate_movement(game, *penguin, *target, NULL);
    switch (input) {
      case MOVEMENT_VALID: return;
      case MOVEMENT_OUT_OF_BOUNDS:
        display_error_message("You cant move oustide the board!");
        break;
      case MOVEMENT_CURRENT_LOCATION: display_error_message("Thats your current location"); break;
      case MOVEMENT_DIAGONAL: display_error_message("You cant move diagonaly!"); break;
      case MOVEMENT_NOT_A_PENGUIN: display_error_message("Chose a penguin for movement"); break;
      case MOVEMENT_NOT_YOUR_PENGUIN:
        display_error_message("Chose YOUR PENGUIN for movement");
        break;
      case MOVEMENT_ONTO_EMPTY_TILE: display_error_message("Can't move onto an empty tile"); break;
      case MOVEMENT_ONTO_PENGUIN: display_error_message("Can't move onto another penguin!"); break;
      case MOVEMENT_OVER_EMPTY_TILE:
        display_error_message("You cant move over an empty tile!");
        break;
      case MOVEMENT_OVER_PENGUIN:
        display_error_message("You cant move over another penguin!");
        break;
      case MOVEMENT_PENGUIN_BLOCKED:
        display_error_message("There are no possible moves for this penguin!");
        break;
    }
  }
}
