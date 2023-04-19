#pragma once

/// @file
/// @brief The core of the unified game logic library, contains the #Game struct

#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief The values of #Game::phase.
///
/// The phase transition diagram:
/// @code{.c}
///
///                           +-----------------+
///                           | GAME_PHASE_NONE |
///                           +-----------------+
///                                    |
///                             game_begin_setup
///                                    |
///                                    v
///                           +------------------+
///                           | GAME_PHASE_SETUP |
///                           +------------------+
///                                    |
///                              game_end_setup
///                                    |
///                                    v
///                        +-----------------------+
/// +--------------------- | GAME_PHASE_SETUP_DONE | <-------------------+
/// |                      +-----------------------+                     |
/// |                                                                    |
/// |                       +----------------------+                     |
/// +-- placement_begin --> | GAME_PHASE_PLACEMENT | --- placement_end --+
/// |                       +----------------------+                     |
/// |                                                                    |
/// |                       +---------------------+                      |
/// +--- movement_begin --> | GAME_PHASE_MOVEMENT | ---- movement_end ---+
/// |                       +---------------------+
/// |
/// +------- game_end -----------------+
///                                    |
///                                    v
///                            +----------------+
///                            | GAME_PHASE_END |
///                            +----------------+
///
/// @endcode
typedef enum GamePhase {
  GAME_PHASE_NONE = 0,   ///< The default phase, set when a #Game is initially constructed.
  GAME_PHASE_SETUP,      ///< Set by #game_begin_setup
  GAME_PHASE_SETUP_DONE, ///< Set by #game_end_setup, #placement_end, #movement_end
  GAME_PHASE_PLACEMENT,  ///< Set by #placement_begin
  GAME_PHASE_MOVEMENT,   ///< Set by #movement_begin
  GAME_PHASE_END,        ///< Set by #game_end
} GamePhase;

/// @brief Holds the player data.
///
/// You shouldn't create instances of this struct yourself, they are allocated
/// by #game_set_players_count.
typedef struct Player {
  /// The unique ID, usually (but not necessarily) is just <tt>index + 1</tt>.
  short id;
  /// Use #game_set_player_name to set this.
  char* name;
  /// The score of the player, i.e. the number of collected fish.
  int points;
  /// The length of the #penguins array.
  int penguins_count;
  /// @brief The list of the positions of all of the player's penguins.
  /// @note Must be kept in sync with the board when setting penguin tiles!
  Coords* penguins;
  /// The number of moves (penguin placements and movements) made by the player.
  int moves_count;
  /// The color of the penguins, currently used only in the TUI.
  int color;
} Player;

/// The values of #GameLogEntry::type.
typedef enum GameLogEntryType {
  GAME_LOG_ENTRY_PHASE_CHANGE,  ///< See #GameLogPhaseChange
  GAME_LOG_ENTRY_PLAYER_CHANGE, ///< See #GameLogPlayerChange
  GAME_LOG_ENTRY_PLACEMENT,     ///< See #GameLogPlacement
  GAME_LOG_ENTRY_MOVEMENT,      ///< See #GameLogMovement
} GameLogEntryType;

/// A #GameLogEntry created by #game_set_phase.
typedef struct GameLogPhaseChange {
  GamePhase old_phase;
  GamePhase new_phase;
} GameLogPhaseChange;

/// A #GameLogEntry created by #game_set_current_player.
typedef struct GameLogPlayerChange {
  int old_player_index;
  int new_player_index;
} GameLogPlayerChange;

/// A #GameLogEntry created by #place_penguin.
typedef struct GameLogPlacement {
  Coords target;
  short undo_tile;
} GameLogPlacement;

/// A #GameLogEntry created by #move_penguin.
typedef struct GameLogMovement {
  Coords penguin;
  Coords target;
  short undo_tile;
} GameLogMovement;

/// @brief An entry in the #Game::log_buffer, implemented as a tagged union.
///
/// The way log entries are stored is basically a union + a type tag. A union
/// in C is a construct which allows interpreting the same data stored in the
/// same memory location in different ways. Sometimes this is used for
/// converting between types (e.g. unpacking an @c int into the bytes it is
/// comprised of), in our case it is used for the purpose of compact storage of
/// the different log entry types. Basically, the field #type indicates the
/// kind of entry stored within the #data field, so, for inspecting this
/// struct, you first @b ALWAYS need to check the #type, and then you can
/// access the appropriate entry #data:
///
/// @code{.c}
/// const GameLogEntry* entry = ...;
/// switch (entry->type) {
///   case GAME_LOG_ENTRY_PLACEMENT: {
///     const GameLogPlacement* data = &entry->data.placement;
///     ...
///   }
///   case GAME_LOG_ENTRY_MOVEMENT: {
///     const GameLogMovement* data = &entry->data.movement;
///     ...
///   }
///   ...
/// }
/// @endcode
///
/// The data structs of particular entry kinds should contain the delta
/// (difference) between the previous and the next game state - this is enough
/// information to both undo and redo the performed action. Also note that the
/// total union (and consequently this struct) will be as large as the longest
/// struct it can contain, the shorter structs will be padded to the longest
/// length - this padding is wasted memory (currently the largest entry is
/// #GameLogMovement, though most of entries will be of this kind anyway, so we
/// aren't wasting much).
///
/// @see <https://en.wikipedia.org/wiki/Tagged_union>
typedef struct GameLogEntry {
  GameLogEntryType type;
  union GameLogEntryData {
    GameLogPhaseChange phase_change;
    GameLogPlayerChange player_change;
    GameLogPlacement placement;
    GameLogMovement movement;
  } data;
} GameLogEntry;

/// @brief The central struct of the application, holds the game data and
/// settings.
///
/// This struct contains the common information used everywhere in the game, so
/// you'll see it being passed around in almost all functions. It is modelled
/// as a <b>state machine</b>: there is a field that holds the current "state",
/// called #phase (note that there are way more phases than just the placement
/// and movement phases described by the rules of the game) and to do something
/// useful with it you have to switch it between those phases. The functions
/// for transitioning between phases typically have requirements for what has
/// to be done prior to making the transition (e.g. #game_end_setup verifies
/// that everything has been initialized properly) and from what other phase
/// the transition can occur (e.g. you can't call #game_begin_setup once the
/// game has already been configured). The functions relating to various phases
/// also typically check that the game is in the correct state (e.g. obviously,
/// #place_penguin only works in the placement phase), <em>although</em> if the
/// function doesn't modify any internal state then there is no harm in not
/// performing the check (e.g. it is possible to call #validate_movement even
/// in the placement phase).
///
/// It also records the history of performed actions (player moves, certain
/// state changes, apart from the changes of the settings during setup) in a
/// log, making it possible to undo or redo any of them with
/// #game_rewind_state_to_log_entry.
///
/// @see <https://en.wikipedia.org/wiki/Finite-state_machine>
///
/// This struct is implemented sort of as a class in my poor-man's OOP system.
/// Basically, you shouldn't initialize the struct yourself, instead you have
/// to call the constructor #game_new, and once you are done with using it, you
/// must to invoke the destructor #game_free. The "methods" of the class are
/// just functions which take <tt>#Game*</tt> or <tt>const #Game*</tt> (when
/// the method doesn't actually change any fields) as the first argument.
///
/// I don't adhere to the traditional OOP style dogmatically though: all fields
/// of the struct are exposed instead of it being fully opaque and having a
/// getter and a setter for every field, and some of the "methods" technically
/// reside in other files (notably board.h, placement.h, movement.h). Although
/// getters and setters are used where it makes sense: setting some fields is
/// not as trivial as just writing a single value, and getters can ensure the
/// correctness of, for example, access into arrays (i.e. check that the index
/// is in the bounds of the array), e.g. #game_get_player. In short, if a
/// getter or a setter for a given field or array exists - you should use it,
/// otherwise accessing the field directly is perfectly fine.
///
/// So in general, the most basic usage of this struct is:
///
/// @code{.c}
/// Game* game = game_new();
///
/// game_begin_setup(game);
/// // ...
/// game_end_setup(game);
///
/// placement_begin(game);
/// // ...
/// placement_end(game);
///
/// movement_begin(game);
/// // ...
/// movement_end(game);
///
/// game_free(game);
/// @endcode
///
/// When working in C++, it is possible to have this struct automatically
/// managed with the help of a @c unique_ptr :
///
/// @code{.cpp}
/// std::unique_ptr<Game, decltype(&game_free)> game(game_new(), &game_free);
/// @endcode
typedef struct Game {
  /// @brief The current state of the state machine, initially set to
  /// #GAME_PHASE_NONE. Use #game_set_phase for setting this.
  ///
  /// @see #GamePhase for the phase transitions diagram.
  GamePhase phase;

  /// @name Players
  /// @{

  /// @brief The list of players with length #players_count. Initialized with
  /// #game_set_players_count. Use #game_get_player for accessing this.
  Player* players;
  /// @brief Use #game_set_players_count for setting this.
  int players_count;
  /// @brief Use #game_set_penguins_per_player for setting this.
  int penguins_per_player;
  /// @brief A negative value means that there is no current player selected.
  /// Use #game_set_current_player for setting this.
  int current_player_index;

  /// @}

  /// @name Board
  /// @{

  /// @brief Use #setup_board for setting this.
  int board_width;
  /// @brief Use #setup_board for setting this.
  int board_height;

  /// @brief A 2D grid represented as a 1D array which stores the tiles of the
  /// board. Use #setup_board for initializing, #get_tile and #set_tile for
  /// accessing.
  ///
  /// The 1D representation works as follows. Say we have a grid like this:
  /// @code{.unparsed}
  /// [ a, b, c, d ]
  /// [ e, f, g, h ]
  /// [ i, j, k, l ]
  /// [ m, n, o, p ]
  /// @endcode
  ///
  /// It will be laid out in memory like this:
  /// @code{.unparsed}
  /// [ a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p ]
  ///   |--------|  |--------|  |--------|  |--------|
  ///      row 1       row 2       row 3       row 4
  /// @endcode
  ///
  /// As you can see, the second row goes immediately after the first one in
  /// the array, the third one after the second one, and so on and so on. So,
  /// to determine the actual location of a tile in this sequential
  /// representation, we can use the formula <tt>index = y * width + x</tt>.
  /// The length of the whole array, consequently, is <tt>width * height</tt>.
  /// This method is used primarily for simplicity - it is easier to manage
  /// memory of a single array than to allocate an array of arrays.
  ///
  /// @see <https://en.wikipedia.org/wiki/Row-_and_column-major_order>
  ///
  /// The tiles themselves are encoded as integers in the following way:
  /// 1. <tt>n = 0</tt> - a water tile.
  /// 2. <tt>n = 1, 2, 3, ...</tt> - an ice tile with @c n fish.
  /// 3. <tt>n = -1, -2, -3, ...</tt> - an ice tile with a penguin of a player
  ///    with ID @c n.
  ///
  /// Since we only have a few small values that we want to store, to conserve
  /// memory the type @c short (2 bytes long) is used instead of an @c int (4
  /// bytes long).
  short* board_grid;

  /// @brief Stores auxilary data of grid tiles for use in the UIs. Use
  /// #setup_board for initializing, #get_tile_attr and #set_tile_attr for
  /// accessing.
  ///
  /// These auxiliary attributes are simply a bitfield - a number where every
  /// bit is a flag which is either on or off, sort of like a struct with bool
  /// fields, but compactly represented in memory. This representation was
  /// chosen because of how simple it was to implement while being sufficient
  /// for the needs of the UI. The utils.h file contains some macros for
  /// working with such bitfields, though in the case of tile attributes,
  /// convenient functions are provided for this: #set_tile_attr,
  /// #get_tile_attr and #set_all_tiles_attr. The list of used attributes can
  /// be seen in the #TileAttribute and #GuiTileAttribute enums. Internally,
  /// tile attributes are stored in the same grid layout as the tiles
  /// themselves are in #board_grid.
  ///
  /// @see <https://en.wikipedia.org/wiki/Bit_field>
  ///
  /// The attributes system was initially written for the graphical interface
  /// and resided in #CanvasPanel, but I have moved it since into the common
  /// library code because for the potential needs of other UIs (but really
  /// because it was more convenient that way).
  short* tile_attributes;

  /// @}

  /// @name Logging
  /// @{

  /// @brief Signals whether new log entries can be created. See
  /// #game_push_log_entry.
  bool log_disabled;
  /// @brief The stack of log entries. Use #game_push_log_entry,
  /// #game_pop_log_entry and #game_get_log_entry for modifying and accessing.
  /// @see <https://en.wikipedia.org/wiki/Stack_(abstract_data_type)>
  GameLogEntry* log_buffer;
  /// @brief The total number of elements #log_buffer was allocated for (i.e.
  /// pushing more requires reallocating it).
  /// @details #game_set_log_capacity can be called during setup to
  /// pre-allocate the buffer.
  size_t log_capacity;
  /// @brief The actual number of entries in #log_buffer. #game_push_log_entry
  /// and #game_pop_log_entry affects this.
  size_t log_length;
  /// @brief The index of the currently selected log entry. Normally equals
  /// #log_length, if less than #log_length an older entry is being viewed.
  size_t log_current;

  /// @}
} Game;

Game* game_new(void);
Game* game_clone(const Game* other);
void game_free(Game* self);

uint32_t game_compute_state_hash(const Game* self);
void game_set_log_capacity(Game* self, size_t capacity);
GameLogEntry* game_push_log_entry(Game* self, GameLogEntryType type);
const GameLogEntry* game_pop_log_entry(Game* self, GameLogEntryType expected_type);
const GameLogEntry* game_get_log_entry(const Game* self, size_t idx);

void game_set_phase(Game* self, GamePhase phase);
void game_set_current_player(Game* self, int idx);

void game_begin_setup(Game* self);
void game_end_setup(Game* self);
void game_set_penguins_per_player(Game* self, int value);
void game_set_players_count(Game* self, int count);
void game_set_player_name(Game* self, int idx, const char* name);

void game_add_player_penguin(Game* self, int idx, Coords coords);
void game_remove_player_penguin(Game* self, int idx, Coords coords);

void game_advance_state(Game* self);
void game_end(Game* self);
void game_rewind_state_to_log_entry(Game* self, size_t target_entry);

/// @relatesalso Game
/// @brief Checks if @c idx is within the bounds of #Game::players.
inline bool game_check_player_index(const Game* self, int idx) {
  return 0 <= idx && idx < self->players_count;
}

/// @relatesalso Game
/// @brief Returns a pointer to the player at the given index. Fails if the
/// index isn't within the bounds of the #Game::players list.
inline Player* game_get_player(const Game* self, int idx) {
  assert(game_check_player_index(self, idx));
  return &self->players[idx];
}

/// @relatesalso Game
/// @brief A shorthand for calling #game_get_player with #Game::current_player_index.
inline Player* game_get_current_player(const Game* self) {
  return game_get_player(self, self->current_player_index);
}

/// @relatesalso Game
/// @brief Returns an index of the player or @c -1 if no such player was found.
inline int game_find_player_by_id(const Game* self, short id) {
  for (int i = 0; i < self->players_count; i++) {
    if (self->players[i].id == id) {
      return i;
    }
  }
  return -1;
}

/// @relatesalso Game
/// @brief Finds a penguin with the given coordinates in the #Player::penguins
/// list of a player at @c idx and returns a pointer to it. Returns @c NULL if
/// no such penguin is found.
inline Coords* game_find_player_penguin(const Game* self, int idx, Coords coords) {
  Player* player = game_get_player(self, idx);
  for (int i = 0; i < player->penguins_count; i++) {
    Coords* penguin = &player->penguins[i];
    if (penguin->x == coords.x && penguin->y == coords.y) {
      return penguin;
    }
  }
  return NULL;
}

#ifdef __cplusplus
}
#endif
