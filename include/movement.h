#pragma once

#include "board.h"
#include "gamestate.h"

typedef enum MovementInput {
  VALID_INPUT = 0,
  OUT_OF_BOUNDS_MOVEMENT,
  CURRENT_LOCATION,
  DIAGONAL_MOVE,
  NOT_YOUR_PENGUIN,
  MOVE_ONTO_EMPTY_TILE,
  MOVE_ONTO_PENGUIN,
  MOVE_OVER_EMPTY_TILE,
  MOVE_OVER_PENGUIN,
} MovementError;

bool any_valid_player_move_exists(Board* board, int player_id);

bool any_valid_movement_exists(Board* board, Player* players, int player_count);

MovementError
validate_movement(Board* board, Coords start, Coords target, int current_player_id, Coords* fail);

void handle_movement_input(
  Coords* penguin, Coords* target, Board* board, Player* current_player, int player_count
);

int move_penguin(Board* board, Coords start, Coords target, int player_id);

void interactive_movement(Board* board, Player player_data[], int player_count);
