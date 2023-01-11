#include "stdbool.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "gamestate.h"
#include "io.h"
#include "movement.h"
#include "placement.h"
#include "random.h"

int main(int argc, char* argv[]) {
  random_init();

  clear_screen();

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

  // static data for testing movement {{{
  /*
  int player_count = 2;
  Player player_data[2] = { { .id = 1, .name = "player_1", .points = 2 },
                            { .id = 2, .name = "player_2", .points = 2 } };
  int row_1[4] = { 1, 0, 1, -1 };
  int row_2[4] = { 0, -2, 3, 0 };
  int row_3[4] = { 3, 1, -1, 3 };
  int row_4[4] = { 0, -2, 1, 0 };
  int* grid[4] = { row_1, row_2, row_3, row_4 };
  Board board = { .height = 4, .width = 4, .grid = grid };
  */
  // }}}

  GameState state = {
    .players = player_data,
    .player_count = player_count,
    .penguin_count = penguin_count,
  };

  interactive_placement(&board, &state);
  interactive_movement(&board, &state);

  return 0;
}
