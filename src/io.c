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

void get_players_count(int* count) {
  printf("Please input number of players\n");
  scanf("%d", count);
}

void get_player_name(int id, char* name) {
  printf("Player %d, please input name\n", id);
  scanf("%s", name);
}