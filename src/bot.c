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
#include <string.h>

#define BEST_MOVES_COUNT 5

void init_bot_parameters(BotParameters* self) {
  self->placement_strategy = BOT_PLACEMENT_SMART;
  self->placement_scan_area = 6;
  self->movement_strategy = BOT_MOVEMENT_SMART;
  self->max_move_length = INT_MAX;
  self->recursion_limit = 4;
  self->junction_check_recursion_limit = 2;
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
  self->fill_grid1_cap = 0;
  self->fill_grid1 = NULL;
  self->fill_grid2_cap = 0;
  self->fill_grid2 = NULL;
  self->fill_stack_cap = 0;
  self->fill_stack = NULL;

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
    free_and_clear(self->fill_grid1);
    free_and_clear(self->fill_grid2);
    free_and_clear(self->fill_stack);
    BotState* next = self->sub_state;
    free(self);
    self = next;
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
#define bot_alloc_buf(buf, capacity, size) \
  (size > capacity ? (buf = realloc(buf, size), capacity = size) : 0)

// The only distance function that is relevant for us since penguins can move
// only along the axes.
static inline int distance(Coords start, Coords end) {
  return abs(end.x - start.x) + abs(end.y - start.y);
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

bool bot_make_placement(BotState* self, Coords* out_target) {
  Game* game = self->game;

  bot_alloc_buf(
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
    *out_target = self->tile_coords[picked_tile_idx];
    return true;
  }

  bot_alloc_buf(self->tile_scores, self->tile_scores_cap, sizeof(int) * tiles_count);
  for (int i = 0; i < tiles_count; i++) {
    self->tile_scores[i] = bot_rate_placement(self, self->tile_coords[i]);
  }

  if (strategy == BOT_PLACEMENT_MOST_FISH) {
    int best_tile_idx = 0;
    int available_tiles UNUSED_ATTR =
      pick_best_scores(tiles_count, self->tile_scores, 1, &best_tile_idx);
    assert(available_tiles == 1);
    *out_target = self->tile_coords[best_tile_idx];
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

  *out_target = picked_tile;
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
      const int DIV_PRECISION = 1000;
      score += tile_score * 4 * DIV_PRECISION / (d * d + 1) / DIV_PRECISION;
    }
  }

  score += count_obstructed_directions(self->game, penguin) * -1000;

  return score;
}

bool bot_make_move(BotState* self, Coords* out_penguin, Coords* out_target) {
  Player* my_player = game_get_current_player(self->game);
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
    *out_penguin = picked_move.penguin, *out_target = picked_move.target;
    return true;
  }

  int* move_scores = bot_rate_moves_list(self, moves_count, moves_list);

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

  *out_penguin = picked_move.penguin, *out_target = picked_move.target;
  return true;
}

BotMove* bot_generate_all_moves_list(
  BotState* self, int penguins_count, Coords* penguins, int* moves_count
) {
  bot_alloc_buf(
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
  bot_alloc_buf(self->all_moves, self->all_moves_cap, sizeof(BotMove) * *moves_count);
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
  Coords prev_penguin = { -1, -1 };
  int fishes_per_dir[DIRECTION_MAX];
  int* fill_grid = NULL;

  bot_alloc_buf(self->move_scores, self->move_scores_cap, sizeof(int) * moves_count);
  for (int i = 0; i < moves_count; i++) {
    BotMove move = moves_list[i];
    int score = bot_rate_move(self, move);
    Coords penguin = move.penguin, target = move.target;

    if (self->depth == 0) {
      if (!(prev_penguin.x == penguin.x && prev_penguin.y == penguin.y)) {
        for (int dir = 0; dir < DIRECTION_MAX; dir++) {
          fishes_per_dir[dir] = 0;
        }
        fill_grid = NULL;
        if (bot_quick_junction_check(self, penguin)) {
          fill_grid = bot_flood_fill_reset_grid(self, &self->fill_grid1, &self->fill_grid1_cap);
          // Fill the cells of the fill grid with the directions they are
          // reachable from and set the elements of fishes_per_dir to the total
          // number of fish reachable in that direction.
          for (int dir = 0; dir < DIRECTION_MAX; dir++) {
            Coords neighbor = DIRECTION_TO_COORDS[dir];
            neighbor.x += penguin.x, neighbor.y += penguin.y;
            if (!is_tile_in_bounds(self->game, neighbor)) continue;
            if (fill_grid[neighbor.x + neighbor.y * self->game->board_width] == 0) {
              fishes_per_dir[dir] = bot_flood_fill_count_fish(self, fill_grid, neighbor, dir + 1);
            }
          }
        }
      }

      if (fill_grid) {
        int available_fish =
          fishes_per_dir[self->fill_grid1[target.x + target.y * self->game->board_width] - 1];
        for (int dir = 0; dir < DIRECTION_MAX; dir++) {
          int missed_fish = fishes_per_dir[dir];
          if (missed_fish != 0) {
            score += 10 * (available_fish - missed_fish);
          }
        }
      }
    }

    self->move_scores[i] = score;
    prev_penguin = penguin;
  }
  return self->move_scores;
}

int bot_rate_move(BotState* self, BotMove move) {
  Coords penguin = move.penguin, target = move.target;
  Player* my_player = game_get_current_player(self->game);
  int score = 0;

  int move_len = distance(penguin, target);
  score += 64 / move_len - 8;
  int target_tile = get_tile(self->game, target);
  int fish = get_tile_fish(target_tile);
  score += 10 * fish * fish;

  if (self->depth == 0 && count_obstructed_directions(self->game, penguin) == 3) {
    // Emergency escape mode
    score += 1000;
  }
  if (self->depth == 0 && count_obstructed_directions(self->game, target) == 4) {
    // A suicide move
    score += -10000;
  }

  for (int dir = 0; dir < DIRECTION_MAX; dir++) {
    Coords neighbor = DIRECTION_TO_COORDS[dir];
    neighbor.x += penguin.x, neighbor.y += penguin.y;
    if (!is_tile_in_bounds(self->game, neighbor)) continue;
    int other_tile = get_tile(self->game, neighbor);
    int other_player_id;
    if ((other_player_id = get_tile_player_id(other_tile))) {
      for (int dir = 0; dir < DIRECTION_MAX; dir++) {
        Coords neighbor = DIRECTION_TO_COORDS[dir];
        neighbor.x += penguin.x, neighbor.y += penguin.y;
        if (!is_tile_in_bounds(self->game, neighbor)) continue;
        if (!is_fish_tile(get_tile(self->game, neighbor))) continue;
        if (neighbor.x == target.x && neighbor.y == target.y) {
          score += -1000;
        }
      }
    }
  }

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
        if (2 <= blocked && blocked <= 3 && move_len == 1) {
          score += 250;
        } else if (blocked > 2) {
          score += 500 * (blocked - 2);
        }
      } else {
        score += -1000;
      }
    }
  }

  if (self->depth <= self->params->junction_check_recursion_limit) {
    int undo_tile = move_penguin(self->game, penguin, target);

    if (bot_quick_junction_check(self, target)) {
      int* fill_grid = bot_flood_fill_reset_grid(self, &self->fill_grid2, &self->fill_grid2_cap);
      int dir;
      for (dir = 0; dir < DIRECTION_MAX; dir++) {
        Coords neighbor = DIRECTION_TO_COORDS[dir];
        neighbor.x += target.x, neighbor.y += target.y;
        if (!is_tile_in_bounds(self->game, neighbor)) continue;
        int marker = dir + 1;
        if (bot_flood_fill_count_fish(self, fill_grid, neighbor, marker) > 0) {
          break;
        }
      }
      // If there was no unobstructed direction, dir will be equal to
      // DIRECTION_MAX at the start of this second loop.
      for (; dir < DIRECTION_MAX; dir++) {
        Coords neighbor = DIRECTION_TO_COORDS[dir];
        neighbor.x += target.x, neighbor.y += target.y;
        if (!is_tile_in_bounds(self->game, neighbor)) continue;
        int other_tile = get_tile(self->game, neighbor);
        if (is_fish_tile(other_tile)) {
          int marker = fill_grid[neighbor.x + neighbor.y * self->game->board_width];
          if (marker == 0) {
            // If a tile in some other direction is not reachable from the
            // direction at which we ran the flood fill, then we must created
            // junction somewhere.
            score += -200;
            break;
          }
        }
      }
    }

    undo_move_penguin(self->game, penguin, target, undo_tile);
  }

  if (self->depth < self->params->recursion_limit) {
    int undo_tile = move_penguin(self->game, penguin, target);

    BotState* sub = bot_enter_sub_state(self);
    int moves_count = 0;
    BotMove* moves_list = bot_generate_all_moves_list(sub, 1, &target, &moves_count);
    int* move_scores = bot_rate_moves_list(sub, moves_count, moves_list);
    int best_index = 0;
    if (pick_best_scores(moves_count, move_scores, 1, &best_index) == 1) {
      score += move_scores[best_index] * 3 / 4;
    }

    undo_move_penguin(self->game, penguin, target, undo_tile);
  }

  return score;
}

// A "junction" is a state when then bot has to check between two or more
// directions because once it makes a move the other paths will be
// inaccessible. This function only performs a quick heuristic check which can
// only tell if the current tile is definitely not a junction, so it is used to
// know if a more time-consuming flood fill test is necessary.
bool bot_quick_junction_check(BotState* self, Coords coords) {
  bool in_bounds[NEIGHBOR_MAX];
  bool is_fish[NEIGHBOR_MAX];
  bool connected[NEIGHBOR_MAX];
  int dir;
  for (dir = 0; dir < NEIGHBOR_MAX; dir++) {
    Coords neighbor = NEIGHBOR_TO_COORDS[dir];
    neighbor.x += coords.x, neighbor.y += coords.y;
    connected[dir] = false;
    is_fish[dir] = false;
    if ((in_bounds[dir] = is_tile_in_bounds(self->game, neighbor))) {
      int tile = get_tile(self->game, neighbor);
      is_fish[dir] = is_fish_tile(tile);
    }
  }

  // Find the starting direction
  int start_dir = NEIGHBOR_MAX;
  for (dir = 0; dir < DIRECTION_MAX; dir++) {
    start_dir = DIRECTION_TO_NEIGHBOR[dir];
    if (is_fish[start_dir]) break;
  }
  if (dir == DIRECTION_MAX) {
    // The tile is obstructed on all sides - definitely not a junction.
    return false;
  }

  // Walk in the clockwise direction
  for (dir = start_dir; dir < NEIGHBOR_MAX; dir++) {
    if (!is_fish[dir]) break;
    connected[dir] = true;
  }
  // Walk in the counter-clockwise direction
  for (dir = start_dir - 1; dir != start_dir; dir--) {
    if (dir < 0) dir += NEIGHBOR_MAX;
    if (!is_fish[dir]) break;
    connected[dir] = true;
  }

  for (dir = 0; dir < DIRECTION_MAX; dir++) {
    if (is_fish[DIRECTION_TO_NEIGHBOR[dir]] && !connected[DIRECTION_TO_NEIGHBOR[dir]]) {
      // Found an unobstructed tile which is not connected - maybe a junction.
      return true;
    }
  }

  // All neighbor tiles are connected - definitely not a junction.
  return false;
}

int* bot_flood_fill_reset_grid(BotState* self, int** fill_grid, size_t* fill_grid_cap) {
  int w = self->game->board_width, h = self->game->board_height;
  bot_alloc_buf(*fill_grid, *fill_grid_cap, sizeof(int) * w * h);
  memset(*fill_grid, 0, sizeof(int) * w * h);
  return *fill_grid;
}

struct BotFloodFillCtx {
  BotState* self;
  int* fill_grid;
  int marker_value;
  int fish_count;
};

static bool bot_flood_fill_check(int x, int y, void* user_data) {
  struct BotFloodFillCtx* ctx = user_data;
  Game* game = ctx->self->game;
  int w = game->board_width, h = game->board_height;
  if ((0 <= x && x < w && 0 <= y && y < h) && ctx->fill_grid[x + y * w] == 0) {
    int tile = game->board_grid[x + y * w];
    return is_fish_tile(tile);
  }
  return false;
}

static void bot_flood_fill_mark(int x, int y, void* user_data) {
  struct BotFloodFillCtx* ctx = user_data;
  Game* game = ctx->self->game;
  int w = game->board_width;
  ctx->fish_count += game->board_grid[x + y * w];
  ctx->fill_grid[x + y * w] = ctx->marker_value;
}

static FillSpan* bot_flood_fill_alloc_stack(size_t capacity, void* user_data) {
  struct BotFloodFillCtx* ctx = user_data;
  BotState* self = ctx->self;
  bot_alloc_buf(self->fill_stack, self->fill_stack_cap, capacity);
  return self->fill_stack;
}

int bot_flood_fill_count_fish(BotState* self, int* grid, Coords start, int marker_value) {
  assert(marker_value != 0);
  struct BotFloodFillCtx ctx;
  ctx.self = self;
  ctx.fill_grid = grid;
  ctx.fish_count = 0;
  ctx.marker_value = marker_value;
  flood_fill(
    start.x, start.y, bot_flood_fill_check, bot_flood_fill_mark, bot_flood_fill_alloc_stack, &ctx
  );
  return ctx.fish_count;
}

static void flood_fill_push(
  FillSpan** stack,
  int* stack_len,
  int* stack_cap,
  int x1,
  int x2,
  int y,
  int dy,
  FillSpan* (*alloc_stack)(size_t capacity, void* user_data),
  void* user_data
) {
  if (*stack_len >= *stack_cap) {
    *stack_cap = my_max(*stack_cap * 2, 256);
    *stack = alloc_stack(*stack_cap * sizeof(FillSpan), user_data);
  }
  FillSpan span = { x1, x2, y, dy };
  (*stack)[*stack_len] = span;
  *stack_len += 1;
}

#define stack_push(x1, x2, y, dy) \
  flood_fill_push(&stack, &stack_len, &stack_cap, x1, x2, y, dy, alloc_stack, user_data)

// See <https://en.wikipedia.org/wiki/Flood_fill#Span_Filling> and
// <https://github.com/erich666/GraphicsGems/blob/c3263439c281da62df4a559ec8164cf8c9eb88ca/gems/SeedFill.c>
void flood_fill(
  int x,
  int y,
  bool (*check)(int x, int y, void* user_data),
  void (*mark)(int x, int y, void* user_data),
  FillSpan* (*alloc_stack)(size_t capacity, void* user_data),
  void* user_data
) {
  if (!check(x, y, user_data)) return;
  int stack_len = 0;
  int stack_cap = 0;
  FillSpan* stack = NULL;
  stack_push(x, x, y, 1);
  stack_push(x, x, y - 1, -1);
  while (stack_len > 0) {
    FillSpan span = stack[--stack_len];
    int x1 = span.x1, x2 = span.x2, y = span.y, dy = span.dy;
    int x = x1;
    if (check(x, y, user_data)) {
      while (check(x - 1, y, user_data)) {
        mark(x - 1, y, user_data);
        x -= 1;
      }
    }
    if (x < x1) {
      stack_push(x, x1 - 1, y - dy, -dy);
    }
    while (x1 <= x2) {
      while (check(x1, y, user_data)) {
        mark(x1, y, user_data);
        stack_push(x, x1, y + dy, dy);
        if (x1 > x2) {
          stack_push(x2 + 1, x1, y - dy, -dy);
        }
        x1 += 1;
      }
      x1 += 1;
      while (x1 < x2 && !check(x1, y, user_data)) {
        x1 += 1;
      }
      x = x1;
    }
  }
}
#undef stack_push
