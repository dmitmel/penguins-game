#include "stdbool.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "gamestate.h"
#include "io.h"
#include "utils.h"

int main(int argc, char* argv[]) {
  random_init();

  int player_count;
  get_players_count(&player_count);
  Player* player_data = malloc(player_count * sizeof(*player_data));
  for (int i = 0; i < player_count; i++) {
    player_data[i].id = i + 1;
    player_data[i].points = 0;
    get_player_name(player_data[i].id, player_data[i].name);
  }

  int penguin_count;
  get_penguin_count(&penguin_count);

  int board_width;
  int board_height;
  get_board_dimensions(&board_width, &board_height);

  Board board = init_board(board_width, board_height);
  generate_random_board(&board);

  // in-memory players are 0-indexed, but for the board and for the players they are 1-indexed
  // when you want to save a user penguin on the board or call a UI method that shows player index,
  // use "player_data[current_player].id" instead of "current_player+1"
  int current_player = 0;

  update_game_state_display(&board, player_data, player_count); // bad for gui
  // placeholder, actual logic for determining the ending of the placement loop should go here
  // ctrl+c to exit for now
  int penguins_to_place = player_count * penguin_count;
  while (penguins_to_place > 0) {
    if (!placeable_spot_exists(&board)) {
      display_error_message("No more penguins are placeable!");
      break;
    }
    int x, y;
    display_new_turn_message(player_data[current_player].id);
    while (true) {
      get_penguin_coordinates(&x, &y, player_data[current_player].id);
      if (x < 0 || x > board.width || y < 0 || y > board.height) {
        display_error_message("Inputted coordinates are outside the bounds of the board");
      } else if (board.grid[y][x] == 0) {
        update_game_state_display(&board, player_data, player_count); // bad for gui

        display_error_message(
          "This tile is empty, you can't put a penguin on an empty(water) tile");
      } else if (board.grid[y][x] < 0) {
        update_game_state_display(&board, player_data, player_count); // bad for gui

        display_error_message("This tile is already occupied by a penguin");
      } else {
        player_data[current_player].points += board.grid[y][x];
        board.grid[y][x] = -player_data[current_player].id;
        penguins_to_place--;
        break;
      }
    }
    update_game_state_display(&board, player_data, player_count);
    current_player = (current_player + 1) % player_count;
  }
  print_end_placement_phase(&board, player_data, player_count);

  return 0;
}
