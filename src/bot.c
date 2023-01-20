#include "bot.h"
#include "board.h"
#include "game.h"
#include "movement.h"
#include "placement.h"
#include "random.h"
#include "utils.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// The only distance function that is relevant for us since penguins can move
// only along the axes.
static int distance(Coords start, Coords end) {
  return abs(end.x - start.x) + abs(end.y - start.y);
}

static int pick_best_scores(int scores_length, int* scores, int best_length, int* best_indexes) {
  int prev_best_score = INT_MAX;
  int i;
  for (i = 0; i < best_length; i++) {
    int best_score = INT_MIN;
    int best_index = -1;
    for (int j = 0; j < scores_length; j++) {
      int score = scores[j];
      if (score >= best_score && score < prev_best_score) {
        best_score = score;
        best_index = j;
      }
    }
    if (best_score <= 0 && i > 0) {
      // If the other scores are too bad - don't include them
      break;
    }
    best_indexes[i] = best_index;
    prev_best_score = best_score;
  }
  return i;
}

bool bot_make_placement(Game* game) {
  Coords* tile_coords = malloc(sizeof(int) * game->board_width * game->board_height);
  int* tile_scores = malloc(sizeof(int) * game->board_width * game->board_height);
  int tiles_count = 0;
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      if (validate_placement_simple(game, coords)) {
        tile_coords[tiles_count] = coords;
        tile_scores[tiles_count] = bot_rate_placement(game, coords);
        tiles_count += 1;
      }
    }
  }
  if (tiles_count == 0) {
    free(tile_coords);
    free(tile_scores);
    return false;
  }

  printf("Tile scores:");
  int row = 0;
  Coords prev_coords = { -1, -1 };
  for (int i = 0; i < tiles_count; i++) {
    Coords coords = tile_coords[i];
    if (coords.y != prev_coords.y) {
      printf("\nRow %2d: ", coords.y);
      row = 0;
    }
    row++;
    if (row > 8) {
      printf("\n");
      row = 0;
    }
    printf("(%2d, %2d) = %5d   ", coords.x, coords.y, tile_scores[i]);
    prev_coords = coords;
  }
  if (row > 0) {
    printf("\n");
  }

  static const int BEST_MOVES_COUNT = 5;
  int best_indexes[BEST_MOVES_COUNT];
  int available_tiles = pick_best_scores(tiles_count, tile_scores, BEST_MOVES_COUNT, best_indexes);
  assert(available_tiles > 0);

  printf("Best tiles:\n");
  for (int i = 0; i < available_tiles; i++) {
    Coords coords = tile_coords[best_indexes[i]];
    int score = tile_scores[best_indexes[i]];
    printf("(%d, %d) = %d\n", coords.x, coords.y, score);
  }

  Coords picked_tile = tile_coords[best_indexes[random_range(0, available_tiles - 1)]];
  printf("Picked (%d, %d)\n", picked_tile.x, picked_tile.y);

  free(tile_coords);
  free(tile_scores);

  place_penguin(game, picked_tile);
  return true;
}

int bot_rate_placement(const Game* game, Coords penguin) {
  int score = 0;

  static const int AREA_SIZE = 6;
  int area_start_x = penguin.x - AREA_SIZE;
  int area_start_y = penguin.y - AREA_SIZE;
  int area_end_x = penguin.x + AREA_SIZE;
  int area_end_y = penguin.y + AREA_SIZE;

  for (int y = area_start_y; y <= area_end_y; y++) {
    for (int x = area_start_x; x <= area_end_x; x++) {
      if (x == penguin.x && y == penguin.y) continue;
      Coords coords = { x, y };
      int tile_score = 0;
      if (is_tile_in_bounds(game, coords)) {
        int tile = get_tile(game, coords);
        int fish, player_id;
        if ((fish = get_tile_fish(tile))) {
          tile_score += 10 * fish;
        } else if ((player_id = get_tile_player_id(tile))) {
          tile_score += player_id == game->current_player_index ? -500 : -600;
        } else if (is_water_tile(tile)) {
          tile_score += -40;
        }
      } else {
        tile_score += 10;
      }
      int d = distance(penguin, coords);
      static const int DIV_PRECISION = 1000;
      score += tile_score * 4 * DIV_PRECISION / (d * d + 1) / DIV_PRECISION;
    }
  }

  for (int i = 0; i < 4; i++) {
    Coords neighbor = penguin;
    int dx = 0, dy = 0;
    switch (i) {
      case 0: dx = 1; break;
      case 1: dy = 1; break;
      case 2: dx = -1; break;
      case 3: dy = -1; break;
    }
    neighbor.x += dx, neighbor.y += dy;
    if (!(is_tile_in_bounds(game, neighbor) && is_fish_tile(get_tile(game, neighbor)))) {
      // The side is obstructed
      score += -1000;
    }
  }

  return score;
}

bool bot_make_move(Game* game) {
  Player* player = game_get_current_player(game);

  PossibleMoves* possible_moves_structs = malloc(sizeof(PossibleMoves) * player->penguins_count);
  int moves_count = 0;
  for (int i = 0; i < player->penguins_count; i++) {
    PossibleMoves moves = calculate_all_possible_moves(game, player->penguins[i]);
    moves_count += moves.all_steps;
    possible_moves_structs[i] = moves;
  }
  if (moves_count == 0) {
    free(possible_moves_structs);
    return false;
  }

  BotMove* all_moves = malloc(sizeof(BotMove) * moves_count);
  BotMove* current_penguin_moves = all_moves;
  for (int i = 0; i < player->penguins_count; i++) {
    current_penguin_moves += iterate_possible_moves_struct(
      player->penguins[i], possible_moves_structs[i], current_penguin_moves
    );
  }
  assert(current_penguin_moves == all_moves + moves_count);
  free(possible_moves_structs);

  int* move_scores = malloc(sizeof(int) * moves_count);
  for (int i = 0; i < moves_count; i++) {
    move_scores[i] = bot_rate_move(game, all_moves[i]);
  }

  static const int BEST_MOVES_COUNT = 5;
  int best_indexes[BEST_MOVES_COUNT];
  int available_moves = pick_best_scores(moves_count, move_scores, BEST_MOVES_COUNT, best_indexes);
  assert(available_moves > 0);

  printf("Best moves:\n");
  for (int i = 0; i < available_moves; i++) {
    BotMove move = all_moves[best_indexes[i]];
    int score = move_scores[best_indexes[i]];
    printf(
      "(%d, %d) -> (%d, %d) = %d\n",
      move.penguin.x,
      move.penguin.y,
      move.target.x,
      move.target.y,
      score
    );
  }

  BotMove picked_move = all_moves[best_indexes[random_range(0, available_moves - 1)]];
  printf(
    "Picked (%d, %d) -> (%d, %d)\n",
    picked_move.penguin.x,
    picked_move.penguin.y,
    picked_move.target.x,
    picked_move.target.y
  );

  free(all_moves);
  free(move_scores);

  move_penguin(game, picked_move.penguin, picked_move.target);

  return true;
}

int iterate_possible_moves_struct(Coords penguin, PossibleMoves moves, BotMove* list) {
  int count = 0;
  count += iterate_possible_moves_steps(penguin, moves.steps_right, 1, 0, list + count);
  count += iterate_possible_moves_steps(penguin, moves.steps_down, 0, 1, list + count);
  count += iterate_possible_moves_steps(penguin, moves.steps_left, -1, 0, list + count);
  count += iterate_possible_moves_steps(penguin, moves.steps_up, 0, -1, list + count);
  assert(count == moves.all_steps);
  return count;
}

int iterate_possible_moves_steps(Coords penguin, int steps, int dx, int dy, BotMove* list) {
  Coords target = penguin;
  int i;
  for (i = 0; i < steps; i++) {
    target.x += dx, target.y += dy;
    BotMove move = { penguin, target };
    list[i] = move;
  }
  return i;
}

int bot_rate_move(const Game* game, BotMove move) {
  int score = 0;

  score += 100 / distance(move.penguin, move.target) - 10;
  int fish = get_tile_fish(get_tile(game, move.target));
  score += 10 * fish * fish;

  return score;
}
