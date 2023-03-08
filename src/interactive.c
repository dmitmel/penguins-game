#include "interactive.h"
#include "board.h"
#include "game.h"
#include "io.h"
#include "movement.h"
#include "placement.h"
#include <stdbool.h>
#include <stddef.h>

int run_interactive_mode(void) {
  clear_screen();

  Game* game = game_new();
  game_begin_setup(game);

  int players_count;
  get_players_count(&players_count);
  game_set_players_count(game, players_count);
  for (int i = 0; i < players_count; i++) {
    Player* player = game_get_player(game, i);
    char name[32];
    get_player_name(i + 1, name);
    game_set_player_name(game, i, name);
    get_player_color(i + 1, &player->color);
  }

  int penguin_count;
  get_penguin_count(&penguin_count);
  game_set_penguins_per_player(game, penguin_count);

  int board_width;
  int board_height;
  get_board_dimensions(&board_width, &board_height);
  setup_board(game, board_width, board_height);
  generate_board_random(game);

  game_end_setup(game);

  interactive_placement(game);
  interactive_movement(game);
  game_end(game);

  game_free(game);

  return 0;
}

void interactive_placement(Game* game) {
  Coords target = { 0, 0 };
  placement_begin(game);
  update_game_state_display(game);
  while (true) {
    int result = placement_switch_player(game);
    if (result < 0) break;
    display_new_turn_message(game->current_player_index + 1);
    handle_placement_input(game, &target);
    place_penguin(game, target);
    update_game_state_display(game);
  }
  placement_end(game);
  print_end_placement_phase(game);
}

void handle_placement_input(Game* game, Coords* selected) {
  while (true) {
    get_penguin_coordinates(selected);
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
    get_data_for_movement(penguin, target);
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
