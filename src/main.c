#include "board.h"
#include "io.h"
#include "utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Player{
  int id;
  char name [16];
  int points;
} Player;

int main(int argc, char* argv[]) {
  random_init();

  int player_count;
  get_players_count(&player_count);
  Player* player_data = malloc(player_count * sizeof(*player_data));
  for (int i = 0; i < player_count; i++) {
    player_data[i].id = i;
    // player_data[i].name = malloc(32 * sizeof(char));
    player_data[i].points = 0;
    get_player_name(i, player_data[i].name);
  }

  int board_width;
  int board_height;
  get_board_dimensions(&board_width, &board_height);

  int** board = init_board(board_width, board_height);
  generate_random_board(board_width, board_height, board);

  printBoard(board, board_height, board_width);

  return 0;
}
