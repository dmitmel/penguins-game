#include "io.h"
#include "board.h"
#include "utils.h"
#include <stdio.h>
#include <math.h>

int main(int argc, char* argv[]) {
  random_init();

  int board_width;
  int board_height;
  get_board_dimensions(&board_width, &board_height);
  
  int** board = init_board(board_width, board_height);
  generate_random_board(board_width, board_height, board);

  printBoard(board, board_height, board_width);

  return 0;
}
