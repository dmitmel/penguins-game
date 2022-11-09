void printBoard(int *board, int ySize, int xSize)
{
  for (int y = 0; y < ySize; y++)
  {
    printf("| ");
    for (int x = 0; x < xSize; x++)
    {
      int val = board[y * xSize + x];
      if (val == 0)
      {
        printf("-  ");
      }
      else if (val < 0)
      {
        printf("p%d ", abs(val));
      }
      else
      {
        printf("%d  ", val);
      }
    }
    printf("|\n");
  }
}