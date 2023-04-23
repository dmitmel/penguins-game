#pragma once

/// @file
/// @brief The bot algorithm
/// @see autonomous.h

#include "game.h"
#include "movement.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Values of #BotParameters::placement_strategy.
typedef enum BotPlacementStrategy {
  /// @brief The standard "smart" algorithm, see #bot_compute_placement for its
  /// description.
  BOT_PLACEMENT_SMART,
  /// Pick a random tile for placement.
  BOT_PLACEMENT_RANDOM,
  /// Pick the first possible tile (first tile in the first row).
  BOT_PLACEMENT_FIRST_POSSIBLE,
  /// Pick the tiles with the most fish in the vicinity.
  BOT_PLACEMENT_MOST_FISH,
} BotPlacementStrategy;

/// Values of #BotParameters::movement_strategy.
typedef enum BotMovementStrategy {
  /// @brief The standard "smart" algorithm, see #bot_compute_move for its
  /// description.
  BOT_MOVEMENT_SMART,
  /// Pick a random move.
  BOT_MOVEMENT_RANDOM,
  /// Pick the first possible move (also known as the "dumb" algorithm).
  BOT_MOVEMENT_FIRST_POSSIBLE,
} BotMovementStrategy;

/// @brief Various parameters for the bot algorithm.
///
/// Mainly used for configuring the bot to be a dumber opponent for testing.
typedef struct BotParameters {
  /// #BOT_PLACEMENT_SMART by default.
  BotPlacementStrategy placement_strategy;
  /// An area surrounding the placement tile which #bot_rate_placement considers.
  int placement_scan_area;
  /// #BOT_MOVEMENT_SMART by default.
  BotMovementStrategy movement_strategy;
  /// The maximum number of tiles allowed for bot's moves, must be positive.
  int max_move_length;
  /// The maximum recursion depth, must be positive. Zero means no recursion.
  int recursion_limit;
  /// The maximum recursion depth at which junction checks are performed.
  int junction_check_recursion_limit;
} BotParameters;

void init_bot_parameters(BotParameters* self);

/// @brief A <tt>penguin -> target</tt> pair.
typedef struct BotMove {
  Coords penguin, target;
} BotMove;

/// @brief Used internally by #flood_fill.
typedef struct FillSpan {
  int x1, x2, y, dy;
} FillSpan;

/// @brief Contains temporary data created during the evaluation of bot's moves.
///
/// Unlike the #Game, this is really just a complementary struct for
/// #bot_compute_move and #bot_compute_placement and less of a class in the OOP
/// sense, though it is implemented as such (refer to the documentation of
/// #Game for the explanation). The primary reason for this being the
/// autonomous mode, where the program is invoked each time just to make a
/// single move and is not kept alive for the entire game, meaning that we
/// can't have some additional state (@e unless we store it in a file,
/// which I didn't want to do to keep things simple). Hence the logic in the
/// autonomous mode and, therefore, the bot algorithm, is generally stateless
/// or, in other words, "fire and forget" -- at any point of the game, you can
/// create a #BotState, which stores only the intermediate data, compute and
/// make a move, and then destroy it (though of course the #BotState objects
/// can be reused later). You could say that this kind of makes the bot
/// algorithm a function of the game state in the mathematical sense.
///
/// For the purposes of recursive evaluation (see #bot_compute_move), the
/// #BotState is actually represented as a linked list (because why not) of
/// substates. Also, it keeps around the previously allocated memory blocks for
/// lists (the @c _cap fields are for capacity of the lists) because constantly
/// making the OS allocate and free temporary buffers is slow, but also since
/// this makes the overall memory management in the bot much simpler -- every
/// function can return whenever and not care about freeing stuff because
/// #bot_state_free will free everything in the end.
///
/// Additional note regarding the rating functions (#bot_rate_placement and
/// #bot_rate_move): all score calculations are performed with integers and not
/// floats because integer arithmetic is faster (though marginally on modern
/// CPUs) than floating point one, and we need to perform A LOT of it.
typedef struct BotState {
  /// @name References to other stuff
  /// The #BotState isn't responsible for freeing these.
  /// @{

  /// Shouldn't be changed while the bot is running (hence marked as @c const).
  const BotParameters* params;
  /// @brief May be modified during the move evaluation, but once the job is
  /// done the #Game will be returned to its original state.
  Game* game;
  /// Just the #Rng, nothing special.
  Rng* rng;

  /// @}

  /// @name Substates
  /// See #bot_enter_substate.
  /// @{

  /// @brief The link to the next recursive substate. Substates are allocated
  /// on demand by #bot_enter_substate.
  struct BotState* substate;
  /// @brief The recursion depth of the current state, starts at 0 for the base
  /// state and increases in substates.
  int depth;

  /// @}

  /// @brief Can be set to @c true from another thread to cancel the move
  /// evaluation.
  ///
  /// The @c volatile keyword stops the compiler from being too smart and
  /// overoptimizing stuff. For example, given this loop:
  /// @code{.c}
  /// while (!self->cancelled) {
  ///   do_work();
  /// }
  /// @endcode
  ///
  /// The compiler will think: \"Well, the @c cancelled flag in the condition
  /// doesn't get changed inside the loop, isn't it? No point in constantly
  /// loading its value from memory!\" and optimize this into:
  /// @code{.c}
  /// if (!self->cancelled) {
  ///   while (true) {
  ///     do_work();
  ///   }
  /// }
  /// @endcode
  ///
  /// The @c volatile keyword on the other hand ensures that every time the
  /// variable is used, the compiler doesn't optimize away (or reorder) the
  /// reads and writes to the memory, and they happen exactly in the way the
  /// programmer intended it in the code.
  ///
  /// It, @e however, doesn't ensure synchronized access to the variable (or
  /// makes other guarantees) when it is used across multiple threads: for
  /// example, the changed value in memory might not be observed immediately by
  /// other processor cores (because of memory caches). Multithreading is not
  /// really the intended purpose of @c volatile and such use is generally
  /// frowned upon, but in our case its fine, since it isn't used for anything
  /// critical -- the bot algorithm simply must be stopped @e eventually (the
  /// precise timing doesn't matter), doesn't require using compiler- or
  /// OS-specific thread synchronization facilities like mutexes or atomics
  /// (heard that C11 has those built-in), and a simple memory read of a value
  /// that almost never gets changed is generally faster than those.
  ///
  /// @see <https://en.wikipedia.org/wiki/Volatile_(computer_programming)>
  /// @see <https://stackoverflow.com/a/246148/12005228>
  /// @see <https://stackoverflow.com/a/2485733/12005228>
  volatile bool cancelled;

  /// @name Allocation caches
  /// See also #bot_alloc_buf.
  /// @{

  size_t tile_coords_cap;
  Coords* tile_coords;
  size_t tile_scores_cap;
  int* tile_scores;

  size_t possible_steps_cap;
  PossibleSteps* possible_steps;
  size_t all_moves_cap;
  BotMove* all_moves;
  size_t move_scores_cap;
  int* move_scores;
  size_t fill_grid1_cap;
  short* fill_grid1;
  size_t fill_grid2_cap;
  short* fill_grid2;
  size_t fill_stack_cap;
  FillSpan* fill_stack;

  /// @}
} BotState;

BotState* bot_state_new(const BotParameters* params, Game* game, Rng* rng);
void bot_state_free(BotState* self);
BotState* bot_enter_substate(BotState* self);

bool bot_compute_placement(BotState* self, Coords* out_target);
int bot_rate_placement(BotState* self, Coords penguin);

bool bot_compute_move(BotState* self, Coords* out_penguin, Coords* out_target);
BotMove* bot_generate_all_moves_list(
  BotState* self, int penguins_count, Coords* penguins, int* moves_count
);
int* bot_rate_moves_list(BotState* self, int moves_count, BotMove* moves_list);
int bot_rate_move(BotState* self, BotMove move);
bool bot_quick_junction_check(BotState* self, Coords coords);
short* bot_flood_fill_reset_grid(BotState* self, short** fill_grid, size_t* fill_grid_cap);
int bot_flood_fill_count_fish(BotState* self, short* grid, Coords start, short marker_value);

void flood_fill(
  int x,
  int y,
  bool (*check)(int x, int y, void* data),
  void (*mark)(int x, int y, void* data),
  FillSpan* (*alloc_stack)(size_t capacity, void* data),
  void* data
);

#ifdef __cplusplus
}
#endif
