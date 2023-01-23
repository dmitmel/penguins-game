#include "bot.h"
#include "arguments.h"
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

bool bot_make_placement(const BotParameters* params, Game* game) {
  BotPlacementStrategy strategy = params->placement_strategy;

  Coords* tile_coords = malloc(sizeof(int) * game->board_width * game->board_height);
  int tiles_count = 0;
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      if (validate_placement_simple(game, coords)) {
        tile_coords[tiles_count] = coords;
        tiles_count += 1;
      }
    }
  }
  if (tiles_count == 0) {
    free(tile_coords);
    return false;
  }

  if (strategy == BOT_PLACEMENT_FIRST_POSSIBLE || strategy == BOT_PLACEMENT_RANDOM) {
    int picked_tile_idx = strategy == BOT_PLACEMENT_RANDOM ? random_range(0, tiles_count - 1) : 0;
    Coords picked_tile = tile_coords[picked_tile_idx];
    free(tile_coords);
    place_penguin(game, picked_tile);
    return true;
  }

  int* tile_scores = malloc(sizeof(int) * tiles_count);
  for (int i = 0; i < tiles_count; i++) {
    tile_scores[i] = bot_rate_placement(params, game, tile_coords[i]);
  }

  if (strategy == BOT_PLACEMENT_MOST_FISH) {
    int best_tile_idx = 0;
    assert(pick_best_scores(tiles_count, tile_scores, 1, &best_tile_idx) == 1);
    Coords picked_tile = tile_coords[best_tile_idx];
    free(tile_coords);
    free(tile_scores);
    place_penguin(game, picked_tile);
    return true;
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

int bot_rate_placement(const BotParameters* params, const Game* game, Coords penguin) {
  Player* my_player = game_get_current_player(game);
  int score = 0;

  int area_start_x = penguin.x - params->placement_scan_area;
  int area_start_y = penguin.y - params->placement_scan_area;
  int area_end_x = penguin.x + params->placement_scan_area;
  int area_end_y = penguin.y + params->placement_scan_area;

  if (params->placement_strategy == BOT_PLACEMENT_MOST_FISH) {
    int total_fish = 0;
    for (int y = area_start_y; y <= area_end_y; y++) {
      for (int x = area_start_x; x <= area_end_x; x++) {
        Coords coords = { x, y };
        if (is_tile_in_bounds(game, coords)) {
          int tile = get_tile(game, coords);
          total_fish += get_tile_fish(tile);
        }
      }
    }
    return total_fish;
  }

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
          tile_score += player_id == my_player->id ? -500 : -600;
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

  for (int dir = 0; dir < DIRECTION_MAX; dir++) {
    Coords neighbor = DIRECTION_TO_COORDS[dir];
    neighbor.x += penguin.x, neighbor.y += penguin.y;
    if (!(is_tile_in_bounds(game, neighbor) && is_fish_tile(get_tile(game, neighbor)))) {
      // The side is obstructed
      score += -1000;
    }
  }

  return score;
}

bool bot_make_move(const BotParameters* params, Game* game) {
  BotMovementStrategy strategy = params->movement_strategy;

  int moves_count = 0;
  BotMove* all_moves =
    generate_all_possible_moves_list(params, game, game->current_player_index, &moves_count);
  if (moves_count == 0) {
    free(all_moves);
    return false;
  }

  if (strategy == BOT_MOVEMENT_FIRST_POSSIBLE || strategy == BOT_MOVEMENT_RANDOM) {
    int picked_move_idx = strategy == BOT_MOVEMENT_RANDOM ? random_range(0, moves_count - 1) : 0;
    BotMove picked_move = all_moves[picked_move_idx];
    free(all_moves);
    move_penguin(game, picked_move.penguin, picked_move.target);
    return true;
  }

  int* move_scores = malloc(sizeof(int) * moves_count);
  for (int i = 0; i < moves_count; i++) {
    move_scores[i] = bot_rate_move(params, game, all_moves[i]);
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

  // BotMove picked_move = all_moves[best_indexes[random_range(0, available_moves - 1)]];
  BotMove picked_move = all_moves[best_indexes[0]];
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

BotMove* generate_all_possible_moves_list(
  const BotParameters* params, Game* game, int player_idx, int* moves_count
) {
  Player* player = game_get_player(game, player_idx);
  PossibleSteps* possible_steps = malloc(sizeof(PossibleSteps) * player->penguins_count);
  for (int i = 0; i < player->penguins_count; i++) {
    PossibleSteps moves = calculate_penguin_possible_moves(game, player->penguins[i]);
    for (int dir = 0; dir < DIRECTION_MAX; dir++) {
      moves.steps[dir] = my_min(moves.steps[dir], params->max_move_length);
      *moves_count += moves.steps[dir];
    }
    possible_steps[i] = moves;
  }
  BotMove* all_moves = malloc(sizeof(BotMove) * *moves_count);
  int move_idx = 0;
  for (int i = 0; i < player->penguins_count; i++) {
    Coords penguin = player->penguins[i];
    PossibleSteps moves = possible_steps[i];
    for (int dir = 0; dir < DIRECTION_MAX; dir++) {
      Coords d = DIRECTION_TO_COORDS[dir];
      Coords target = penguin;
      for (int steps = moves.steps[dir]; steps > 0; steps--) {
        target.x += d.x, target.y += d.y;
        BotMove move = { penguin, target };
        all_moves[move_idx++] = move;
      }
    }
  }
  assert(move_idx == *moves_count);
  free(possible_steps);
  return all_moves;
}

int bot_rate_move(const BotParameters* UNUSED(params), const Game* game, BotMove move) {
  Coords penguin = move.penguin, target = move.target;
  Player* my_player = game_get_current_player(game);
  int score = 0;

  score += 100 / distance(penguin, target) - 10;
  int target_tile = get_tile(game, target);
  int fish = get_tile_fish(target_tile);
  score += 10 * fish * fish;

  for (int dir = 0; dir < DIRECTION_MAX; dir++) {
    Coords neighbor = DIRECTION_TO_COORDS[dir];
    neighbor.x += target.x, neighbor.y += target.y;
    if (!is_tile_in_bounds(game, neighbor)) continue;
    if (neighbor.x == penguin.x && neighbor.y == penguin.y) continue;

    int other_tile = get_tile(game, neighbor);
    int other_player_id;
    if ((other_player_id = get_tile_player_id(other_tile))) {
      score += other_player_id == my_player->id ? -500 : 500;
    } else if (!is_fish_tile(other_tile)) {
      score += -100;
    }
  }

  return score;
}
