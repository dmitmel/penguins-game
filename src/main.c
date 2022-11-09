#include "board.h"
#include "utils.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
  random_init();

  int board_width = 20;
  int board_height = 20;
  int** board = init_board(board_width, board_height);
  generate_random_board(board_width, board_height, board);

  return 0;
}
