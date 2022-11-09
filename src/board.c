#include "board.h"
#include "utils.h"
#include <stdlib.h>

int** init_board(int width, int height) {
  int** grid = calloc(height, sizeof(int*));
  for (int i = 0; i < height; i++) {
    int* row = calloc(height, sizeof(int));
    grid[i] = row;
  }
  return grid;
}

void generate_random_board(int width, int height, int** grid) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      grid[y][x] = random_range(0, 3);
    }
  }
}
