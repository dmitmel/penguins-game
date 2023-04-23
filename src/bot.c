// The implementation of the bot algorithm was informed mainly by my vague
// memory of how chess bots work, plus some non-trivial on-the-spot solutions.
// The code leaves a lot to be desired: it is very inefficient (it has to
// evaluate a lot of useless moves, performs some costly calculations twice
// etc), is generally a spaghetti mess, and also apparently I have implemented
// cancellation incorrectly (the sub states don't see it since the `cancelled`
// flag is set only on the root state), but I think it's best to leave it in
// this imperfect state.
//
// If ever desired, here are some nice projects from which some inspiration
// could be pulled:
// <https://github.com/jthemphill/htmf>
// <https://github.com/camc/chess>
// <https://github.com/ethiery/HeyThatsMyFish>

#include "bot.h"
#include "board.h"
#include "game.h"
#include "movement.h"
#include "placement.h"
#include "utils.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/// @relatedalso BotParameters
/// @brief Initializes all fields of the given #BotParameters to default values.
void init_bot_parameters(BotParameters* self) {
  self->placement_strategy = BOT_PLACEMENT_SMART;
  self->placement_scan_area = 6;
  self->movement_strategy = BOT_MOVEMENT_SMART;
  self->max_move_length = INT_MAX;
  self->recursion_limit = 4;
  self->junction_check_recursion_limit = 2;
}

/// @relatedalso BotState
/// @brief Constructs a #BotState (similarly #game_new).
BotState* bot_state_new(const BotParameters* params, Game* game, Rng* rng) {
  BotState* self = malloc(sizeof(*self));
  self->params = params;
  self->game = game;
  self->rng = rng;
  self->substate = NULL;
  self->depth = 0;
  self->cancelled = false;

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

  return self;
}

/// @relatedalso BotState
/// @brief Recursively destroys a #BotState and its substates (similarly to
/// #game_free).
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
    BotState* next = self->substate;
    self->substate = NULL;
    free(self);
    self = next;
  }
}

/// @relatedalso BotState
/// @brief Allocates #BotState::substate if necessary and returns it.
BotState* bot_enter_substate(BotState* self) {
  if (self->substate == NULL) {
    self->substate = bot_state_new(self->params, self->game, self->rng);
    self->substate->depth = self->depth + 1;
  }
  return self->substate;
}

/// @brief A helper for allocating the buffers cached within the #BotState.
///
/// Checks if the buffer has enough capacity for data of the requested size, if
/// not -- reallocates it at the requested size. The @c capacity and @c size
/// arguments are the number of elements (not the number of bytes), and are
/// implicitly converted to @c size_t (the multiplication by 1 is used for that).
#define bot_alloc_buf(buf, capacity, size) \
  ((size_t)1 * size > capacity ? (buf = realloc(buf, sizeof(*buf) * size), capacity = size) : 0)

/// @details The only distance function that is relevant for us since penguins
/// can move only along the axes.
/// @see <https://en.wikipedia.org/wiki/Taxicab_geometry>
static inline int distance(Coords start, Coords end) {
  return abs(end.x - start.x) + abs(end.y - start.y);
}

static int pick_best_score(int scores_length, int* scores) {
  int best_index = -1;
  int best_score = INT_MIN;
  for (int i = 0; i < scores_length; i++) {
    int score = scores[i];
    if (score >= best_score) {
      best_score = score;
      best_index = i;
    }
  }
  return best_index;
}

/// @relatedalso BotState
/// @brief Computes the best placement for the current player given the current
/// game state.
///
/// The algorithm for the placement phase is very simple:
/// 1. Generate a list of valid tiles for placement.
/// 2. Assign a score to every tile according to the rules in #bot_rate_placement.
/// 3. Pick the tile with the highest score.
///
/// @returns @c true if the function managed to find a placement, @c false if
/// there are no possible placements for the player or if the computation was
/// cancelled. The coordinates of the resulting tile are written to @c out_target.
bool bot_compute_placement(BotState* self, Coords* out_target) {
  Game* game = self->game;

  bot_alloc_buf(self->tile_coords, self->tile_coords_cap, game->board_width * game->board_height);
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
    Rng* rng = self->rng;
    int picked_tile_idx =
      strategy == BOT_PLACEMENT_RANDOM ? rng->random_range(rng, 0, tiles_count - 1) : 0;
    *out_target = self->tile_coords[picked_tile_idx];
    return true;
  }

  bot_alloc_buf(self->tile_scores, self->tile_scores_cap, tiles_count);
  for (int i = 0; i < tiles_count; i++) {
    self->tile_scores[i] = bot_rate_placement(self, self->tile_coords[i]);
    if (self->cancelled) return false;
  }

  int best_tile_idx = pick_best_score(tiles_count, self->tile_scores);
  assert(best_tile_idx >= 0);
  *out_target = self->tile_coords[best_tile_idx];
  return true;
}

/// @relatedalso BotState
/// @brief Assigns a score to the placement at the given coordinates.
///
/// Evaluates the nearby tiles to determine how good the placement location is.
int bot_rate_placement(BotState* self, Coords penguin) {
  int score = 0;
  if (self->cancelled) return score;
  Player* my_player = game_get_current_player(self->game);

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
          short tile = get_tile(self->game, coords);
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
        short tile = get_tile(self->game, coords), fish, player_id;
        if ((fish = get_tile_fish(tile))) {
          // Prioritize areas with more fish in the vicinity
          tile_score += 10 * fish;
        } else if ((player_id = get_tile_player_id(tile))) {
          // Try to distribute the players uniformly (this doesn't work well in
          // practice however)
          tile_score += player_id == my_player->id ? -500 : -600;
        } else if (is_water_tile(tile)) {
          // We don't want to put the penguin beside the water
          tile_score += -40;
        }
      } else {
        // Makes the board bounds less unattractive
        tile_score += 10;
      }
      int d = distance(penguin, coords);
      const int DIV_PRECISION = 1000;
      // The further away the tile is from the penguin, the less effect it
      // should have on the calculation
      score += tile_score * 4 * DIV_PRECISION / (d * d + 1) / DIV_PRECISION;
    }
  }

  // This is a significant one
  score += count_obstructed_directions(self->game, penguin) * -1000;

  return score;
}

/// @relatedalso BotState
/// @brief Computes the best move for the current player given the current game
/// state.
///
/// The overall algorithm is roughly the following:
///
/// 1. Generate a list of all possible moves with #bot_generate_all_moves_list.
/// 2. Evaluate all moves in the list by calling #bot_rate_moves_list.
/// 3. Assign a score to every move using #bot_rate_move.
///    1. Some basic rules are considered first, such as: collecting more fish
///       means a higher score, longer moves mean a lower score.
///    2. Then, rules for making the bot @e aggressive are applied: if the move
///       resulted in blocking an opponent's penguin, its score is increased
///       dramatically.
///    3. The scores of obviously stupid moves (such as blocking yourself with
///       your own penguin) are reduced significantly so as to not take them
///       seriously.
///    4. And lastly, the most important part of the algorithm takes place: the
///       move is applied to the #Game instance, and the algorithm is repeated
///       recursively from step 1 (generate a list of all moves available from
///       there, assign scores to each and so on and so on). The best next move
///       is selected and its score is added to the score of the currently
///       considered move -- this allows the bot to see future opportunities,
///       including locating and blocking other penguins, which makes it REALLY
///       aggressive (sometimes, though rarely, it even can discover combos).
///       Afterwards, the applied move is undone.
/// 4. Finally, the move with the highest is selected and returned.
///
/// Note that when evaluating moves recursively, the provided #Game is modified
/// instead of creating a copy of it each time. However, recursive evaluation
/// currently has a significant limitation -- it only considers sequences of
/// moves of a single penguin, and doesn't take the opponents' moves into
/// account at all (doing otherwise consumes too much computing time).
///
/// @returns @c true if a move was found, @c false if there were no moves
/// available for the player or if the computation was cancelled. If found, the
/// move is written to @c out_penguin and @c out_target.
///
/// The aggressive strategy proved to be very effective against other bots,
/// which are usually passive (meaning that they simply try to collect fish) -
/// it first blocks all other players and then collects the rest of the fish.
/// It is even somewhat effective against humans because its aggressiveness
/// essentially relies on the opponent making a mistake, which is not an
/// unreasonable assumption when playing against a human (especially myself).
/// Although, consequently, the stupidest bots, which are simply making
/// literally the first available move after scanning the board (this is what
/// the #BOT_MOVEMENT_FIRST_POSSIBLE strategy does), ended up being the
/// algorithm's fatal weakness (well, not exactly fatal, it can still win
/// against those) -- primarily because they are very persistent at just moving
/// in a single direction.
bool bot_compute_move(BotState* self, Coords* out_penguin, Coords* out_target) {
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
    Rng* rng = self->rng;
    int picked_move_idx =
      strategy == BOT_MOVEMENT_RANDOM ? rng->random_range(rng, 0, moves_count - 1) : 0;
    BotMove picked_move = self->all_moves[picked_move_idx];
    *out_penguin = picked_move.penguin, *out_target = picked_move.target;
    return true;
  }

  int* move_scores = bot_rate_moves_list(self, moves_count, moves_list);
  if (self->cancelled) return false;

  int best_index = pick_best_score(moves_count, move_scores);
  assert(best_index >= 0);
  BotMove picked_move = moves_list[best_index];
  *out_penguin = picked_move.penguin, *out_target = picked_move.target;
  return true;
}

/// @relatedalso BotState
/// @brief Creates a list with all the possible moves (as <tt>penguin ->
/// target</tt> pairs) of the provided penguins.
/// @returns A pointer to the resulting list, whose length is written to the @c
/// moves_count argument.
BotMove* bot_generate_all_moves_list(
  BotState* self, int penguins_count, Coords* penguins, int* moves_count
) {
  // The total number of all moves must be known before allocating the final
  // list, so a list of PossibleSteps structs is collected first.
  bot_alloc_buf(self->possible_steps, self->possible_steps_cap, penguins_count);
  *moves_count = 0;
  for (int i = 0; i < penguins_count; i++) {
    PossibleSteps moves = calculate_penguin_possible_moves(self->game, penguins[i]);
    for (int dir = 0; dir < DIRECTION_MAX; dir++) {
      moves.steps[dir] = my_min(moves.steps[dir], self->params->max_move_length);
      *moves_count += moves.steps[dir];
    }
    self->possible_steps[i] = moves;
  }

  // Then, the PossibleSteps are expanded into actual moves.
  bot_alloc_buf(self->all_moves, self->all_moves_cap, *moves_count);
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

/// @relatedalso BotState
/// @brief Applies #bot_rate_move to every move in the provided list and
/// returns a pointer to the list of scores.
int* bot_rate_moves_list(BotState* self, int moves_count, BotMove* moves_list) {
  if (self->cancelled) return NULL;
  Coords prev_penguin = { -1, -1 };
  int fishes_per_dir[DIRECTION_MAX];
  short* fill_grid = NULL;

  bot_alloc_buf(self->move_scores, self->move_scores_cap, moves_count);
  for (int i = 0; i < moves_count; i++) {
    BotMove move = moves_list[i];
    int score = bot_rate_move(self, move);
    if (self->cancelled) return NULL;
    Coords penguin = move.penguin, target = move.target;

    // This is the primary junction check: whenever a choice must be made, we
    // choose in favor of the direction from which the most fish is accessible.
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
              short marker = (short)(dir + 1);
              fishes_per_dir[dir] = bot_flood_fill_count_fish(self, fill_grid, neighbor, marker);
            }
          }
        }
      }

      if (fill_grid) {
        short marker = fill_grid[target.x + target.y * self->game->board_width];
        int available_fish = fishes_per_dir[marker - 1];
        for (int dir = 0; dir < DIRECTION_MAX; dir++) {
          int missed_fish = fishes_per_dir[dir];
          if (missed_fish != 0) {
            // Will give a bonus if the chosen direction has more fish
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

/// @relatedalso BotState
/// @brief The heart of the bot, assigns a score to the given move according to
/// its usefulness.
///
/// In case a cancellation is requested (see #BotState::cancelled), this
/// function starts unwinding the recursive calls and returns some garbage
/// score, but otherwise the move computation is stopped.
int bot_rate_move(BotState* self, BotMove move) {
  int score = 0;
  if (self->cancelled) return score;
  Coords penguin = move.penguin, target = move.target;
  Player* my_player = game_get_current_player(self->game);

  int move_len = distance(penguin, target);
  // Prioritize shorter moves
  score += 64 / move_len - 8;
  short target_tile = get_tile(self->game, target);
  short fish = get_tile_fish(target_tile);
  // Prioritize collecting more fish
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
    short other_tile = get_tile(self->game, neighbor);
    if (is_penguin_tile(other_tile) && distance(penguin, target) == 1) {
      // This was supposed to prevent the bot from chasing the penguins of the
      // stupidest first-available-move bots, though it didn't work in
      // practice.
      score += -1000;
    }
  }

  for (int dir = 0; dir < DIRECTION_MAX; dir++) {
    Coords neighbor = DIRECTION_TO_COORDS[dir];
    neighbor.x += target.x, neighbor.y += target.y;
    if (!is_tile_in_bounds(self->game, neighbor)) continue;
    if (neighbor.x == penguin.x && neighbor.y == penguin.y) continue;
    short other_tile = get_tile(self->game, neighbor), other_player_id;
    if ((other_player_id = get_tile_player_id(other_tile))) {
      if (other_player_id != my_player->id) {
        // One side will be blocked by us, so add 1.
        int blocked = count_obstructed_directions(self->game, neighbor) + 1;
        if (2 <= blocked && blocked <= 3 && move_len == 1) {
          // This is also a (non-functional) chasing protection, see the
          // comment above.
          score += 250;
        } else if (blocked > 2) {
          // The aggressiveness bonus
          score += 500 * (blocked - 2);
        }
      } else {
        // Don't block our own penguins!
        score += -1000;
      }
    }
  }

  if (self->depth < self->params->recursion_limit) {
    move_penguin(self->game, penguin, target);

    if (self->depth <= self->params->junction_check_recursion_limit) {
      // Another junction check, this one discourages the bot from creating
      // junctions in the first place. This prevents it from getting itself
      // into trouble sometimes.
      if (bot_quick_junction_check(self, target)) {
        short* fill_grid =
          bot_flood_fill_reset_grid(self, &self->fill_grid2, &self->fill_grid2_cap);
        int dir;
        for (dir = 0; dir < DIRECTION_MAX; dir++) {
          Coords neighbor = DIRECTION_TO_COORDS[dir];
          neighbor.x += target.x, neighbor.y += target.y;
          if (!is_tile_in_bounds(self->game, neighbor)) continue;
          short marker = (short)(dir + 1);
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
          short other_tile = get_tile(self->game, neighbor);
          if (is_fish_tile(other_tile)) {
            short marker = fill_grid[neighbor.x + neighbor.y * self->game->board_width];
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
    }

    BotState* sub = bot_enter_substate(self);
    int moves_count = 0;
    BotMove* moves_list = bot_generate_all_moves_list(sub, 1, &target, &moves_count);
    int* move_scores = bot_rate_moves_list(sub, moves_count, moves_list);
    if (!self->cancelled) {
      int best_index = pick_best_score(moves_count, move_scores);
      if (best_index >= 0) {
        // The score of recursive moves is scaled by a coefficient so as to
        // encourage the bot to make quicker gains and not dream too much about
        // the future, since the situation might change quickly.
        score += move_scores[best_index] * 3 / 4;
      }
    }

    undo_move_penguin(self->game);
  }

  return score;
}

/// @relatedalso BotState
/// @brief A precondition for junction checks to know if a more expensive flood
/// fill test is necessary.
///
/// A "junction" is a state when then bot has to choose between two or more
/// directions because once it makes a move the other paths will become
/// inaccessible. This function only performs a quick heuristic check which can
/// only tell if the current tile is <em>definitely not</em> a junction, so it
/// is used to know if a further more time-consuming flood fill test is
/// necessary.
bool bot_quick_junction_check(BotState* self, Coords coords) {
  bool in_bounds[NEIGHBOR_MAX];
  bool is_fish[NEIGHBOR_MAX];
  bool connected[NEIGHBOR_MAX];

  // Collect the information about neighboring tiles first
  int dir;
  for (dir = 0; dir < NEIGHBOR_MAX; dir++) {
    Coords neighbor = NEIGHBOR_TO_COORDS[dir];
    neighbor.x += coords.x, neighbor.y += coords.y;
    connected[dir] = false;
    is_fish[dir] = false;
    if ((in_bounds[dir] = is_tile_in_bounds(self->game, neighbor))) {
      short tile = get_tile(self->game, neighbor);
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

/// @relatedalso BotState
/// @brief Allocates a grid for use in #bot_flood_fill_count_fish and fills it
/// with zeroes.
short* bot_flood_fill_reset_grid(BotState* self, short** fill_grid, size_t* fill_grid_cap) {
  int w = self->game->board_width, h = self->game->board_height;
  bot_alloc_buf(*fill_grid, *fill_grid_cap, w * h);
  memset(*fill_grid, 0, sizeof(**fill_grid) * w * h);
  return *fill_grid;
}

/// @relatedalso BotState
/// @brief See #bot_flood_fill_count_fish.
struct BotFloodFillCtx {
  BotState* self;
  short* fill_grid;
  short marker_value;
  int fish_count;
};

/// @relatedalso BotState
/// @brief See #bot_flood_fill_count_fish and #flood_fill.
static bool bot_flood_fill_check(int x, int y, void* data) {
  struct BotFloodFillCtx* ctx = data;
  Game* game = ctx->self->game;
  int w = game->board_width, h = game->board_height;
  if ((0 <= x && x < w && 0 <= y && y < h) && ctx->fill_grid[x + y * w] == 0) {
    short tile = game->board_grid[x + y * w];
    return is_fish_tile(tile);
  }
  return false;
}

/// @relatedalso BotState
/// @brief See #bot_flood_fill_count_fish and #flood_fill.
static void bot_flood_fill_mark(int x, int y, void* data) {
  struct BotFloodFillCtx* ctx = data;
  Game* game = ctx->self->game;
  int w = game->board_width;
  ctx->fish_count += game->board_grid[x + y * w];
  ctx->fill_grid[x + y * w] = ctx->marker_value;
}

/// @relatedalso BotState
/// @brief See #bot_flood_fill_count_fish and #flood_fill.
static FillSpan* bot_flood_fill_alloc_stack(size_t capacity, void* data) {
  struct BotFloodFillCtx* ctx = data;
  BotState* self = ctx->self;
  bot_alloc_buf(self->fill_stack, self->fill_stack_cap, capacity);
  return self->fill_stack;
}

/// @relatedalso BotState
/// @brief Counts all fish accessible within an enclosed area starting at the
/// given point using the flood fill algorithm.
///
/// Returns the number of reachable fish, the counted tiles are marked on the
/// provided fill grid with @c marker_value, which must be non-zero.
/// Additionally is used for junction checks.
///
/// @see #flood_fill
int bot_flood_fill_count_fish(BotState* self, short* grid, Coords start, short marker_value) {
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
  size_t* stack_len,
  size_t* stack_cap,
  int x1,
  int x2,
  int y,
  int dy,
  FillSpan* (*alloc_stack)(size_t capacity, void* data),
  void* data
) {
  if (*stack_len >= *stack_cap) {
    *stack_cap = my_max(*stack_cap * 2, 256);
    *stack = alloc_stack(*stack_cap, data);
  }
  FillSpan span = { x1, x2, y, dy };
  (*stack)[*stack_len] = span;
  *stack_len += 1;
}

/// @brief An implementation of flood fill using the span filling algorithm.
///
/// This is essentially the algorithm behind the "bucket" tool in paint
/// programs. The code was pretty much copied (and translated from pseudocode)
/// from Wikipedia.
///
/// @param x the starting point
/// @param y the starting point
/// @param check a function that returns @c true if the given cell should be marked
/// @param mark a function that marks the given cell
/// @param alloc_stack a function that allocates that stack for #FillSpan s
/// @param data extra data to be passed into the provided functions
///
/// @see <https://en.wikipedia.org/wiki/Flood_fill#Span_Filling>
/// @see <https://github.com/erich666/GraphicsGems/blob/c3263439c281da62df4a559ec8164cf8c9eb88ca/gems/SeedFill.c>
void flood_fill(
  int x,
  int y,
  bool (*check)(int x, int y, void* data),
  void (*mark)(int x, int y, void* data),
  FillSpan* (*alloc_stack)(size_t capacity, void* data),
  void* data
) {
  if (!check(x, y, data)) return;

  size_t stack_len = 0;
  size_t stack_cap = 0;
  FillSpan* stack = NULL;
#define stack_push(x1, x2, y, dy) \
  flood_fill_push(&stack, &stack_len, &stack_cap, x1, x2, y, dy, alloc_stack, data)

  stack_push(x, x, y, 1);
  stack_push(x, x, y - 1, -1);
  while (stack_len > 0) {
    FillSpan span = stack[--stack_len];
    int x1 = span.x1, x2 = span.x2, y = span.y, dy = span.dy;
    int x = x1;
    if (check(x, y, data)) {
      while (check(x - 1, y, data)) {
        mark(x - 1, y, data);
        x -= 1;
      }
    }
    if (x < x1) {
      stack_push(x, x1 - 1, y - dy, -dy);
    }
    while (x1 <= x2) {
      while (check(x1, y, data)) {
        mark(x1, y, data);
        stack_push(x, x1, y + dy, dy);
        if (x1 > x2) {
          stack_push(x2 + 1, x1, y - dy, -dy);
        }
        x1 += 1;
      }
      x1 += 1;
      while (x1 < x2 && !check(x1, y, data)) {
        x1 += 1;
      }
      x = x1;
    }
  }

#undef stack_push
}
