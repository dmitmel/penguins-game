#include "stdbool.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "gamestate.h"
#include "io.h"
#include "random.h"

typedef struct Coords {
  int x;
  int y;
} Coords;

typedef enum SelectedTile {
  EMPTY,
  PENGUIN_OWN,
  PENGUIN_ENEMY,
  FISH_MULTIPLE,
  FISH_SINGLE,
  OUT_OF_BOUNDS
} SelectedTile;

SelectedTile get_player_input_tile(int* x, int* y, Board* board, Player* current_player) {
  get_penguin_coordinates(x, y);
  if (*x < 0 || *x >= board->width || *y < 0 || *y >= board->height) {
    return OUT_OF_BOUNDS;
  }
  int tile = board->grid[*y][*x];
  if (tile == 0) {
    return EMPTY;
  } else if (tile < 0) {
    if (-tile == current_player->id) {
      return PENGUIN_OWN;
    } else {
      return PENGUIN_ENEMY;
    }
  } else {
    if (tile == 1) {
      return FISH_SINGLE;
    }
  }
  return FISH_MULTIPLE;
}

void handle_placement_input(
  int* x, int* y, Board* board, Player* current_player, int player_count
) {
  while (true) {
    SelectedTile tile = get_player_input_tile(x, y, board, current_player);
    switch (tile) {
    case OUT_OF_BOUNDS:
      display_error_message("Inputted coordinates are outside the bounds of the board");
      break;
    case EMPTY:
      display_error_message("This tile is empty, you can't select an empty(water) tile");
      break;
    case PENGUIN_ENEMY:
      display_error_message("This tile is already occupied by a penguin");
      break;
    case PENGUIN_OWN:
      display_error_message("This tile is already occupied by a penguin");
      break;
    case FISH_MULTIPLE:
      display_error_message("Only a tile with just one fish can be selected");
      break;
    case FISH_SINGLE:
      return;
    default:
      display_error_message("ERROR: what on god's green earth did you just select???");
      break;
    }
  }
}

void handle_movement_input(
  int* penguin_x,
  int* penguin_y,
  int* target_x,
  int* target_y,
  Board* board,
  Player* current_player,
  int player_count
) {
  // TODO: look how handle_placement_input is handled
  // use movement_is_valid() method to check if the movement is valid
  // only return (exit) from this method if the movement is valid
  movement_is_valid(board, *penguin_x, *penguin_y, *target_x, *target_y);
  return;
}

int main(int argc, char* argv[]) {
  random_init();

  /*
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
  // use "player_data[current_player].id" instead of "current_player+1" for clarity
  int current_player = 0;
  int penguins_to_place = player_count * penguin_count;

  update_game_state_display(&board, player_data, player_count);

  /// PLACEMENT PHASE
  while (penguins_to_place > 0) {
    if (!placeable_spot_exists(&board)) {
      display_error_message("No more penguins are placeable!");
      break;
    }
    int x, y;
    display_new_turn_message(player_data[current_player].id);
    handle_placement_input(&x, &y, &board, &(player_data[current_player]), player_count);
    player_data[current_player].points += board.grid[y][x];
    board.grid[y][x] = -player_data[current_player].id;
    penguins_to_place--;
    update_game_state_display(&board, player_data, player_count);
    current_player = (current_player + 1) % player_count;
  }
  print_end_placement_phase(&board, player_data, player_count);

  */

  // END OF PLACEMENT PHASE
  //////////////////////////////////////////////
  // START OF MOVEMENT PHASE

  // start of static data for testing movement
  int player_count = 2;
  int current_player = 0;
  Player player_data[2] = { { .id = 0, .name = "player_1", .points = 2 },
                            { .id = 1, .name = "player_2", .points = 2 } };
  int row_1[4] = { 1, 0, 1, -1 };
  int row_2[4] = { 0, -2, 3, 0 };
  int row_3[4] = { 3, 1, -1, 3 };
  int row_4[4] = { 0, -2, 1, 0 };
  int* grid[4] = { row_1, row_2, row_3, row_4 };
  Board board = { .height = 4, .width = 4, .grid = grid };
  print_board(&board);
  int x;
  int y;
  // end of static data for testing movement

  int penguin_x;
  int penguin_y;
  while (any_valid_player_move_exist()) {
    if(!valid_player_move_exist(current_player)){
      display_error_message("No valid moves for the player");
    }
    
    handle_movement_input(
      &penguin_x, &penguin_y, &x, &y, &board, &player_data[current_player], player_count
    );
    // after this function call we have:
    // the x and y of the target tile and penguin_x and penguin_y of the penguin moving 
    // and know that the movement is valid
    move_penguin(&board, &penguin_x, &penguin_y, &x, &y, player_data[player_count].id);
  }

  return 0;
}
