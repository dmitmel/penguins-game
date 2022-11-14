#include <stdio.h>

void printBoard(int** board, int ySize, int xSize) {
  for (int y = 0; y < ySize; y++) {
    printf("| ");
    for (int x = 0; x < xSize; x++) {
      int val = board[y][x];
      if (val == 0) {
        printf("-  ");
      } else if (val < 0) {
        printf("p%d ", -val);
      } else {
        printf("%d  ", val);
      }
    }
    printf("|\n");
  }
}

void get_board_dimensions(int* width, int* height){
  printf("Please specify width and height of the board\n");
  printf("Eg.:' 10 5 -> width=10, height=5\n");
  scanf("%d %d", width, height);
}
