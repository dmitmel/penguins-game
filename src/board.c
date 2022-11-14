#include "board.h"
#include "utils.h"
#include <stdlib.h>

Board init_board(int width, int height) {
  int** grid = calloc(height, sizeof(int*));
  for (int i = 0; i < height; i++) {
    int* row = calloc(width, sizeof(int));
    grid[i] = row;
  }
  Board board = {
    .width = width,
    .height = height,
    .grid = grid,
  };
  return board;
}

void free_board(Board* board) {
  for (int i = 0; i < board->height; i++) {
    int* row = board->grid[i];
    free(row);
  }
  free(board->grid);
}

void generate_random_board(Board* board) {
  for (int y = 0; y < board->height; y++) {
    for (int x = 0; x < board->width; x++) {
      board->grid[y][x] = random_range(0, 3);
    }
  }
}
