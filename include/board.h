#pragma once

typedef struct Board {
  int width;
  int height;
  int** grid;
} Board;

Board init_board(int width, int height);

void free_board(Board* board);

void generate_random_board(Board* board);
