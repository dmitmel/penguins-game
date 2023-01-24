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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void init_bot_parameters(BotParameters* self) {
  self->placement_strategy = BOT_PLACEMENT_SMART;
  self->placement_scan_area = 6;
  self->movement_strategy = BOT_MOVEMENT_SMART;
  self->max_move_length = INT_MAX;
  self->recursion_limit = 3;
}

BotState* bot_state_new(const BotParameters* params, Game* game) {
  BotState* self = malloc(sizeof(BotState));
  self->params = params;
  self->game = game;

  self->tile_coords_cap = 0;
  self->tile_coords = NULL;
  self->tile_scores_cap = 0;
  self->tile_scores = NULL;

  self->possible_steps_cap = 0;
  self->possible_steps = NULL;
  self->all_moves_cap = 0;
  self->all_moves = NULL;
  self->move_scores_cap = 0;
  self->move_scores = NULL;

  self->sub_state = NULL;
  self->depth = 0;
  return self;
}

void bot_state_free(BotState* self) {
  while (self != NULL) {
    free_and_clear(self->tile_coords);
    free_and_clear(self->tile_scores);
    free_and_clear(self->possible_steps);
    free_and_clear(self->all_moves);
    free_and_clear(self->move_scores);
    BotState* tmp = self->sub_state;
    free(self);
    self = tmp;
  }
}

BotState* bot_enter_sub_state(BotState* self) {
  if (self->sub_state == NULL) {
    self->sub_state = bot_state_new(self->params, self->game);
    self->sub_state->depth = self->depth + 1;
  }
  return self->sub_state;
}

// Checks if the buffer has enough capacity for data of requested size, if not
// - reallocates it to fit the requested size.
#define bot_buf_alloc(buf, capacity, size) \
  (size > capacity ? (buf = realloc(buf, size), capacity = size) : 0)

// The only distance function that is relevant for us since penguins can move
// only along the axes.
static int distance(Coords start, Coords end) {
  return abs(end.x - start.x) + abs(end.y - start.y);
}

static int count_obstructed_directions(const Game* game, Coords penguin) {
  int result = 0;
  for (int dir = 0; dir < DIRECTION_MAX; dir++) {
    Coords neighbor = DIRECTION_TO_COORDS[dir];
    neighbor.x += penguin.x, neighbor.y += penguin.y;
    if (!(is_tile_in_bounds(game, neighbor) && is_fish_tile(get_tile(game, neighbor)))) {
      result += 1;
    }
  }
  return result;
}

static int pick_best_scores(int scores_length, int* scores, int best_length, int* best_indexes) {
  best_length = my_min(best_length, scores_length);
  int prev_best_score = INT_MAX;
  int j = 0;
  while (j < best_length) {
    int best_score = INT_MIN;
    int best_index = -1;
    for (int i = 0; i < scores_length; i++) {
      int score = scores[i];
      if (score >= best_score && score < prev_best_score) {
        best_score = score;
        best_index = i;
      }
    }
    if (best_score <= 0 && j > 0) {
      // If the other scores are too bad - don't include them
      break;
    }
    best_indexes[j++] = best_index;
    prev_best_score = best_score;
  }
  return j;
}

bool bot_make_placement(BotState* self) {
  Game* game = self->game;

  bot_buf_alloc(
    self->tile_coords, self->tile_coords_cap, sizeof(int) * game->board_width * game->board_height
  );
  int tiles_count = 0;
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      if (validate_placement_simple(game, coords)) {
        self->tile_coords[tiles_count++] = coords;
      }
    }
  }
  if (tiles_count == 0) {
    return false;
  }

  BotPlacementStrategy strategy = self->params->placement_strategy;
  if (strategy == BOT_PLACEMENT_FIRST_POSSIBLE || strategy == BOT_PLACEMENT_RANDOM) {
    int picked_tile_idx = strategy == BOT_PLACEMENT_RANDOM ? random_range(0, tiles_count - 1) : 0;
    place_penguin(game, self->tile_coords[picked_tile_idx]);
    return true;
  }

  bot_buf_alloc(self->tile_scores, self->tile_scores_cap, sizeof(int) * tiles_count);
  for (int i = 0; i < tiles_count; i++) {
    self->tile_scores[i] = bot_rate_placement(self, self->tile_coords[i]);
  }

  if (strategy == BOT_PLACEMENT_MOST_FISH) {
    int best_tile_idx = 0;
    int available_tiles = pick_best_scores(tiles_count, self->tile_scores, 1, &best_tile_idx);
    assert(available_tiles == 1);
    place_penguin(game, self->tile_coords[best_tile_idx]);
    return true;
  }

  fprintf(stderr, "Tile scores:");
  int row = 0;
  Coords prev_coords = { -1, -1 };
  for (int i = 0; i < tiles_count; i++) {
    Coords coords = self->tile_coords[i];
    if (coords.y != prev_coords.y) {
      fprintf(stderr, "\nRow %2d: ", coords.y);
      row = 0;
    }
    row++;
    if (row > 8) {
      fprintf(stderr, "\n");
      row = 0;
    }
    fprintf(stderr, "(%2d, %2d) = %5d   ", coords.x, coords.y, self->tile_scores[i]);
    prev_coords = coords;
  }
  if (row > 0) {
    fprintf(stderr, "\n");
  }

  static const int BEST_MOVES_COUNT = 5;
  int best_indexes[BEST_MOVES_COUNT];
  int available_tiles =
    pick_best_scores(tiles_count, self->tile_scores, BEST_MOVES_COUNT, best_indexes);
  assert(available_tiles > 0);

  fprintf(stderr, "Best tiles:\n");
  for (int i = 0; i < available_tiles; i++) {
    Coords coords = self->tile_coords[best_indexes[i]];
    int score = self->tile_scores[best_indexes[i]];
    fprintf(stderr, "(%d, %d) = %d\n", coords.x, coords.y, score);
  }

  Coords picked_tile = self->tile_coords[best_indexes[random_range(0, available_tiles - 1)]];
  fprintf(stderr, "Picked (%d, %d)\n", picked_tile.x, picked_tile.y);

  place_penguin(game, picked_tile);
  return true;
}

int bot_rate_placement(BotState* self, Coords penguin) {
  Player* my_player = game_get_current_player(self->game);
  int score = 0;

  int area_start_x = penguin.x - self->params->placement_scan_area;
  int area_start_y = penguin.y - self->params->placement_scan_area;
  int area_end_x = penguin.x + self->params->placement_scan_area;
  int area_end_y = penguin.y + self->params->placement_scan_area;

  if (self->params->placement_strategy == BOT_PLACEMENT_MOST_FISH) {
    int total_fish = 0;
    for (int y = area_start_y; y <= area_end_y; y++) {
      for (int x = area_start_x; x <= area_end_x; x++) {
        Coords coords = { x, y };
        if (is_tile_in_bounds(self->game, coords)) {
          int tile = get_tile(self->game, coords);
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
      if (is_tile_in_bounds(self->game, coords)) {
        int tile = get_tile(self->game, coords);
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

  score += count_obstructed_directions(self->game, penguin) * -1000;

  return score;
}

bool bot_make_move(BotState* self) {
  Player* my_player = game_get_player(self->game, self->game->current_player_index);
  int moves_count = 0;
  BotMove* moves_list = bot_generate_all_moves_list(
    self, my_player->penguins_count, my_player->penguins, &moves_count
  );
  if (moves_count == 0) {
    return false;
  }

  BotMovementStrategy strategy = self->params->movement_strategy;
  if (strategy == BOT_MOVEMENT_FIRST_POSSIBLE || strategy == BOT_MOVEMENT_RANDOM) {
    int picked_move_idx = strategy == BOT_MOVEMENT_RANDOM ? random_range(0, moves_count - 1) : 0;
    BotMove picked_move = self->all_moves[picked_move_idx];
    move_penguin(self->game, picked_move.penguin, picked_move.target);
    return true;
  }

  int* move_scores = bot_rate_moves_list(self, moves_count, moves_list);

  static const int BEST_MOVES_COUNT = 5;
  int best_indexes[BEST_MOVES_COUNT];
  int available_moves = pick_best_scores(moves_count, move_scores, BEST_MOVES_COUNT, best_indexes);
  assert(available_moves > 0);

  fprintf(stderr, "Best moves:\n");
  for (int i = 0; i < available_moves; i++) {
    BotMove move = moves_list[best_indexes[i]];
    int score = move_scores[best_indexes[i]];
    fprintf(
      stderr,
      "(%d, %d) -> (%d, %d) = %d\n",
      move.penguin.x,
      move.penguin.y,
      move.target.x,
      move.target.y,
      score
    );
  }

  BotMove picked_move = moves_list[best_indexes[0]];
  fprintf(
    stderr,
    "Picked (%d, %d) -> (%d, %d)\n",
    picked_move.penguin.x,
    picked_move.penguin.y,
    picked_move.target.x,
    picked_move.target.y
  );

  move_penguin(self->game, picked_move.penguin, picked_move.target);
  return true;
}

BotMove* bot_generate_all_moves_list(
  BotState* self, int penguins_count, Coords* penguins, int* moves_count
) {
  bot_buf_alloc(
    self->possible_steps, self->possible_steps_cap, sizeof(PossibleSteps) * penguins_count
  );
  for (int i = 0; i < penguins_count; i++) {
    PossibleSteps moves = calculate_penguin_possible_moves(self->game, penguins[i]);
    for (int dir = 0; dir < DIRECTION_MAX; dir++) {
      moves.steps[dir] = my_min(moves.steps[dir], self->params->max_move_length);
      *moves_count += moves.steps[dir];
    }
    self->possible_steps[i] = moves;
  }
  bot_buf_alloc(self->all_moves, self->all_moves_cap, sizeof(BotMove) * *moves_count);
  int move_idx = 0;
  for (int i = 0; i < penguins_count; i++) {
    Coords penguin = penguins[i];
    PossibleSteps moves = self->possible_steps[i];
    for (int dir = 0; dir < DIRECTION_MAX; dir++) {
      Coords d = DIRECTION_TO_COORDS[dir];
      Coords target = penguin;
      for (int steps = moves.steps[dir]; steps > 0; steps--) {
        target.x += d.x, target.y += d.y;
        BotMove move = { penguin, target };
        self->all_moves[move_idx++] = move;
      }
    }
  }
  assert(move_idx == *moves_count);
  return self->all_moves;
}

int* bot_rate_moves_list(BotState* self, int moves_count, BotMove* moves_list) {
  bot_buf_alloc(self->move_scores, self->move_scores_cap, sizeof(int) * moves_count);
  for (int i = 0; i < moves_count; i++) {
    self->move_scores[i] = bot_rate_move(self, moves_list[i]);
  }
  return self->move_scores;
}

int bot_rate_move(BotState* self, BotMove move) {
  Coords penguin = move.penguin, target = move.target;
  Player* my_player = game_get_current_player(self->game);
  int score = 0;

  score += 100 / distance(penguin, target) - 10;
  int target_tile = get_tile(self->game, target);
  int fish = get_tile_fish(target_tile);
  score += 10 * fish * fish;

  for (int dir = 0; dir < DIRECTION_MAX; dir++) {
    Coords neighbor = DIRECTION_TO_COORDS[dir];
    neighbor.x += target.x, neighbor.y += target.y;
    if (!is_tile_in_bounds(self->game, neighbor)) continue;
    if (neighbor.x == penguin.x && neighbor.y == penguin.y) continue;

    int other_tile = get_tile(self->game, neighbor);
    int other_player_id;
    if ((other_player_id = get_tile_player_id(other_tile))) {
      if (other_player_id != my_player->id) {
        // One side will be blocked by us, so add 1.
        int blocked = count_obstructed_directions(self->game, neighbor) + 1;
        if (blocked > 2) score += 500 * (blocked - 2);
      } else {
        score += -1000;
      }
    } else if (!is_fish_tile(other_tile)) {
      // score += -100; // Makes the bot very ineffective at gathering fish.
    }
  }

  if (self->depth < self->params->recursion_limit) {
    BotState* sub = bot_enter_sub_state(self);
    int undo_tile = move_penguin(sub->game, penguin, target);
    int moves_count = 0;
    BotMove* moves_list = bot_generate_all_moves_list(sub, 1, &target, &moves_count);
    int* move_scores = bot_rate_moves_list(sub, moves_count, moves_list);
    int best_index = 0;
    if (pick_best_scores(moves_count, move_scores, 1, &best_index) == 1) {
      score += move_scores[best_index] * 3 / 4;
    }
    undo_move_penguin(sub->game, penguin, target, undo_tile);
  }

  return score;
}
