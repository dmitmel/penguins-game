#include "board.h"
#include "io.h"
#include "utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct player {
  int id;
  char name[16];
  int points;
} player;

int main(int argc, char* argv[]) {
  random_init();

  int player_count;
  get_players_count(&player_count);
  player* player_data = malloc(player_count * sizeof(*player_data));
  for (int i = 0; i < player_count; i++) {
    player_data[i].id = i;
    player_data[i].points = 0;
    get_player_name(i+1, player_data[i].name);
  }

  int penguin_count;
  get_penguin_count(&penguin_count);

  int board_width;
  int board_height;
  get_board_dimensions(&board_width, &board_height);

  int** board = init_board(board_width, board_height);
  generate_random_board(board_width, board_height, board);

  print_board(board, board_height, board_width);

  // placeholder, actual logic should for the placement loop should go here, ctrl+z to exit for now
  //TODO: logic for iterating through the players and penguins in the placement phase
  int current_player = 1;
  while (1) {
    int x, y;
    printf("Player %d's turn.\n", current_player);
    // TODO: validate inputs
    get_penguin_coordinates(&x, &y, current_player);
    board[y][x] = (-current_player);
    print_board(board, board_height, board_width);
    current_player = (current_player) % player_count + 1;
  }

  return 0;
}
