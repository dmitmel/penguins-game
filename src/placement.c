#include "placement.h"
#include "board.h"
#include "gamestate.h"
#include "io.h"

TileType get_player_input_tile(Coords* target, const Board* board, const Player* current_player) {
  get_penguin_coordinates(target);
  if (target->x < 0 || target->x >= board->width || target->y < 0 || target->y >= board->height) {
    return OUT_OF_BOUNDS;
  }
  int tile = board->grid[target->y][target->x];
  if (tile == 0) {
    return EMPTY_TILE;
  } else if (tile < 0) {
    if (-tile == current_player->id) {
      return PENGUIN_OWN;
    } else {
      return PENGUIN_ENEMY;
    }
  } else {
    if (tile == 1) {
      return FISH_SINGLE;
    }
  }
  return FISH_MULTIPLE;
}

void handle_placement_input(
  Coords* selected, Board* board, const Player* current_player, int player_count
) {
  while (true) {
    TileType tile = get_player_input_tile(selected, board, current_player);
    switch (tile) {
    case OUT_OF_BOUNDS:
      display_error_message("Inputted coordinates are outside the bounds of the board");
      break;
    case EMPTY_TILE:
      display_error_message("This tile is empty, you can't select an empty(water) tile");
      break;
    case PENGUIN_ENEMY:
      display_error_message("This tile is already occupied by a penguin");
      break;
    case PENGUIN_OWN:
      display_error_message("This tile is already occupied by a penguin");
      break;
    case FISH_MULTIPLE:
      display_error_message("Only a tile with just one fish can be selected");
      break;
    case FISH_SINGLE:
      return;
    default:
      display_error_message("ERROR: what on god's green earth did you just select???");
      break;
    }
  }
}

void interactive_placement(Board* board, GameState* state) {
  Coords target;
  int current_player_idx = 0;
  int penguins_to_place = state->player_count * state->penguin_count;

  update_game_state_display(board, state->players, state->player_count);

  while (penguins_to_place > 0) {
    if (!placeable_spot_exists(board)) {
      display_error_message("No more penguins are placeable!");
      break;
    }
    int x, y;
    Player* current_player = &state->players[current_player_idx];
    display_new_turn_message(current_player->id);
    handle_placement_input(&target, board, current_player, state->player_count);
    current_player->points += board->grid[y][x];
    board->grid[y][x] = -current_player->id;
    penguins_to_place--;
    update_game_state_display(board, state->players, state->player_count);
    current_player_idx = (current_player_idx + 1) % state->player_count;
  }
  print_end_placement_phase(board, state->players, state->player_count);
}
