#include <stdbool.h>
#include <stdlib.h>

#include "board.h"
#include "random.h"
#include "gamestate.h"

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

bool placeable_spot_exists(Board* board) {
  for (int y = 0; y < board->height; y++) {
    for (int x = 0; x < board->width; x++) {
      if (board->grid[y][x] == 1) {
        return true;
      }
    }
  }
  return false;
}

bool valid_movement_exists(Board* board, int player_id) {
  // TODO the selected player can move their penguin
  // players are encoded on the board as an integer equal to -player_id
  // so a row [0, 1, -1, -2] means that player 1 and player 2 have a penguin on the right half of the row
  return true;
}

bool any_valid_movement_exists(Board* board, Player* players, int player_count){
  for(int i =0; i<player_count; i++){
    if(valid_movement_exists(board, players[i].id)){
      return true;
    }
  }
  return false;
}


typedef enum MovementInput{
  OUT_OF_BOUNDS_MOVEMENT,
  CURRENT_LOCATION,
  DIAGONAL_MOVE,
  VALID_INPUT
}MovementInput;

typedef enum CheckedTile {
  EMPTY,
  PENGUIN,
  VALID_TILE
} CheckedTile;

MovementInput check_movement_input(int* target_x, int* target_y, int* start_x, int* start_y, Board* board)
{
  if (*target_x < 0 || *target_x >= board->width || *target_y < 0 || *target_y >= board->height)
  {
    return OUT_OF_BOUNDS_MOVEMENT; 
  }else if(*target_x==*start_x&&*target_y==*start_y)
  {
    return CURRENT_LOCATION;
  }else if(*target_x!=*start_x&&*target_y!=*start_y)
  {
    return DIAGONAL_MOVE;
  }
  return VALID_INPUT;
}

CheckedTile check_a_tile(int* x, int* y, Board* board) {
  int Tile = board->grid[*y][*x];
  if (Tile == 0) {
    return EMPTY;
  } else if (Tile < 0)
  {
    return PENGUIN;
  }
  return VALID_TILE;
}

bool movement_is_valid(Board* board, int start_x, int start_y, int target_x, int target_y) {
  // TODO no penguins or empty tiles in the way to interrupt the itended movement  
  int i, x_path, y_path;
  
  if(start_x>target_x)
  {
    x_path=start_x-target_x;
  }else
  {
    x_path=target_x-start_x;
  }

  if(start_y>target_y)
  {
    y_path=start_y-target_y;
  }else
  {
    y_path=target_y-start_y;
  }
  
  if(target_x!=start_x)
  {
    for(i=0;i<x_path;i++)
    {
      start_x++;
      CheckedTile Tile_x=check_a_tile(start_x, start_y, board);
      switch(Tile_x)
      {
        case EMPTY:
        display_error_message("You cant move over an empty tile!");
        break;
        case PENGUIN:
        display_error_message("You cant move over another penguin!");
        break;
        case VALID_TILE:
        return;
      }
    }
  }else
  {
    for(i=0;i<y_path;i++)
    {
      start_y++;
      CheckedTile Tile_y=check_a_tile(start_x, start_y, board);
      switch(Tile_y)
      {
        case EMPTY:
        display_error_message("You cant move over an empty tile!");
        break;
        case PENGUIN:
        display_error_message("You cant move over another penguin!");
        break;
        case VALID_TILE:
        return;
      }
    }
  }

  return true;
}

// returns the number of fish captured on the way
int move_penguin(Board* board, int start_x, int start_y, int target_x, int target_y, int player_id) {
  // TODO update the board
  // remove the tiles destroyed by the movement (set to 0)
  // set the target position as -player_id (board->grid[target_y][target_x] = -player_id)
  // count and then return the number of fish captured on the way
  return 1;
}