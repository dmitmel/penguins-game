#pragma once

#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum GamePhase {
  GAME_PHASE_NONE = 0,
  GAME_PHASE_SETUP,
  GAME_PHASE_SETUP_DONE,
  GAME_PHASE_PLACEMENT,
  GAME_PHASE_PLACEMENT_DONE,
  GAME_PHASE_MOVEMENT,
  GAME_PHASE_MOVEMENT_DONE,
  GAME_PHASE_END,
} GamePhase;

typedef struct Player {
  short id;
  char* name;
  int points;
  int penguins_count;
  Coords* penguins;
  int moves_count;
  int color;
} Player;

typedef enum GameLogEntryType {
  GAME_LOG_ENTRY_PHASE_CHANGE = 1,
  GAME_LOG_ENTRY_PLAYER_CHANGE,
  GAME_LOG_ENTRY_PLACEMENT,
  GAME_LOG_ENTRY_MOVEMENT,
} GameLogEntryType;

typedef struct GameLogPhaseChange {
  GamePhase old_phase;
  GamePhase new_phase;
} GameLogPhaseChange;

typedef struct GameLogPlayerChange {
  int old_player_index;
  int new_player_index;
} GameLogPlayerChange;

typedef struct GameLogPlacement {
  Coords target;
  short undo_tile;
} GameLogPlacement;

typedef struct GameLogMovement {
  Coords penguin;
  Coords target;
  short undo_tile;
} GameLogMovement;

typedef struct GameLogEntry {
  GameLogEntryType type;
  union GameLogEntryData {
    GameLogPhaseChange phase_change;
    GameLogPlayerChange player_change;
    GameLogPlacement placement;
    GameLogMovement movement;
  } data;
} GameLogEntry;

typedef struct Game {
  GamePhase phase;
  Player* players;
  int players_count;
  int penguins_per_player;
  int board_width;
  int board_height;
  short* board_grid;
  short* tile_attributes;
  int current_player_index;
  bool log_disabled;
  GameLogEntry* log_buffer;
  size_t log_capacity;
  size_t log_length;
  size_t log_current;
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
void game_set_player_score(Game* self, int idx, int points);
void game_add_player_penguin(Game* self, int idx, Coords coords);
void game_remove_player_penguin(Game* self, int idx, Coords coords);

void game_advance_state(Game* self);
void game_end(Game* self);
void game_rewind_state_to_log_entry(Game* self, size_t target_entry);

inline bool game_check_player_index(const Game* self, int idx) {
  return 0 <= idx && idx < self->players_count;
}

inline Player* game_get_player(const Game* self, int idx) {
  assert(game_check_player_index(self, idx));
  return &self->players[idx];
}

inline Player* game_get_current_player(const Game* self) {
  return game_get_player(self, self->current_player_index);
}

inline int game_find_player_by_id(const Game* self, short id) {
  for (int i = 0; i < self->players_count; i++) {
    if (self->players[i].id == id) {
      return i;
    }
  }
  return -1;
}

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
