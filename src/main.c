#include "io.h"
#include <stdio.h>
#include <math.h>

int main(int argc, char *argv[])
{
  printf("hi\n");


  int board[3][4] = {{0, 1, -2, 3}, {0, -1, 0, 0}, {0, 0, 0, 0}};
  printBoard(board, 3, 4);
  return 0;
}
