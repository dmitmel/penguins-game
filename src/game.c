#include "game.h"
#include "movement.h"
#include "placement.h"
#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern bool game_check_player_index(const Game* self, int idx);
extern Player* game_get_player(const Game* self, int idx);
extern Player* game_get_current_player(const Game* self);
extern int game_find_player_by_id(const Game* self, short id);
extern Coords* game_find_player_penguin(const Game* self, int idx, Coords coords);

Game* game_new(void) {
  Game* self = malloc(sizeof(*self));
  self->phase = GAME_PHASE_NONE;
  self->players = NULL;
  self->players_count = -1;
  self->penguins_per_player = -1;
  self->board_width = -1;
  self->board_height = -1;
  self->board_grid = NULL;
  self->tile_attributes = NULL;
  self->current_player_index = -1;
  self->log_disabled = false;
  self->log_buffer = NULL;
  self->log_capacity = 0;
  self->log_length = 0;
  self->log_current = 0;
  return self;
}

Game* game_clone(const Game* other) {
  Game* self = memdup(other, sizeof(*self));
  if (other->players) {
    self->players = memdup(other->players, sizeof(*other->players) * other->players_count);
    for (int i = 0; i < other->players_count; i++) {
      Player *player = &self->players[i], *other_player = &other->players[i];
      player->name = other_player->name ? strdup(other_player->name) : NULL;
      player->penguins = memdup(
        other_player->penguins, sizeof(*other_player->penguins) * other_player->penguins_count
      );
    }
  }
  if (self->board_grid) {
    self->board_grid = memdup(
      other->board_grid, sizeof(*other->board_grid) * other->board_width * other->board_height
    );
  }
  if (self->tile_attributes) {
    self->tile_attributes = memdup(
      other->tile_attributes,
      sizeof(*self->tile_attributes) * other->board_width * other->board_height
    );
  }
  if (self->log_buffer) {
    self->log_buffer = memdup(other->log_buffer, sizeof(*self->log_buffer) * other->log_capacity);
  }
  return self;
}

void game_free(Game* self) {
  if (self == NULL) return;
  if (self->players) {
    for (int i = 0; i < self->players_count; i++) {
      free_and_clear(self->players[i].name);
      free_and_clear(self->players[i].penguins);
    }
    free_and_clear(self->players);
  }
  free_and_clear(self->board_grid);
  free_and_clear(self->tile_attributes);
  free_and_clear(self->log_buffer);
  free(self);
}

#define fnv32_hash_value(state, value) state = fnv32_hash(state, &value, sizeof(value))
#define fnv32_hash_array(state, ptr, len) state = fnv32_hash(state, ptr, sizeof(*ptr) * len)
uint32_t game_compute_state_hash(const Game* self) {
  uint32_t hash = FNV32_INITIAL_STATE;
  fnv32_hash_value(hash, self->phase);
  for (int i = 0; i < self->players_count; i++) {
    const Player* player = &self->players[i];
    fnv32_hash_value(hash, player->id);
    fnv32_hash_value(hash, player->points);
    fnv32_hash_value(hash, player->penguins_count);
    for (int i = 0; i < player->penguins_count; i++) {
      const Coords* coords = &player->penguins[i];
      fnv32_hash_value(hash, coords->x);
      fnv32_hash_value(hash, coords->y);
    }
    fnv32_hash_value(hash, player->moves_count);
  }
  fnv32_hash_value(hash, self->players_count);
  fnv32_hash_value(hash, self->penguins_per_player);
  fnv32_hash_value(hash, self->board_width);
  fnv32_hash_value(hash, self->board_height);
  if (self->board_grid) {
    fnv32_hash_array(hash, self->board_grid, self->board_width * self->board_height);
  }
  fnv32_hash_value(hash, self->current_player_index);
  return hash;
}
#undef fnv32_hash_value
#undef fnv32_hash_array

void game_set_log_capacity(Game* self, size_t capacity) {
  self->log_capacity = capacity;
  self->log_length = my_min(self->log_length, capacity);
  self->log_current = my_min(self->log_current, capacity);
  self->log_buffer = realloc(self->log_buffer, sizeof(*self->log_buffer) * capacity);
}

GameLogEntry* game_push_log_entry(Game* self, GameLogEntryType type) {
  if (self->log_disabled) {
    return NULL;
  }
  if (self->log_current >= self->log_capacity) {
    game_set_log_capacity(self, my_max(self->log_capacity * 2, 1));
  }
  GameLogEntry* entry = &self->log_buffer[self->log_current];
  entry->type = type;
  self->log_current += 1;
  // If some entries were undone (so log_current < log_length) and the user
  // then pushes a new entry, this line will discard all the undone entries.
  self->log_length = self->log_current;
  return entry;
}

const GameLogEntry* game_pop_log_entry(Game* self, GameLogEntryType expected_type) {
  assert(self->log_current > 0);
  const GameLogEntry* entry = &self->log_buffer[self->log_current - 1];
  assert(entry->type == expected_type);
  // Unsure about this, but let's check the type even in the release mode.
  // Interpreting an entry as the wrong type is nasty even by C standards.
  if (entry->type != expected_type) {
    return NULL;
  }
  self->log_current -= 1;
  return entry;
}

const GameLogEntry* game_get_log_entry(const Game* self, size_t idx) {
  assert(idx < self->log_length);
  return &self->log_buffer[idx];
}

void game_set_phase(Game* self, GamePhase phase) {
  if (self->phase == phase) return;
  GameLogEntry* entry;
  if ((entry = game_push_log_entry(self, GAME_LOG_ENTRY_PHASE_CHANGE)) != NULL) {
    GameLogPhaseChange* entry_data = &entry->data.phase_change;
    entry_data->old_phase = self->phase;
    entry_data->new_phase = phase;
  }
  self->phase = phase;
}

void game_set_current_player(Game* self, int idx) {
  if (self->current_player_index == idx) return;
  GameLogEntry* entry;
  if ((entry = game_push_log_entry(self, GAME_LOG_ENTRY_PLAYER_CHANGE)) != NULL) {
    GameLogPlayerChange* entry_data = &entry->data.player_change;
    entry_data->old_player_index = self->current_player_index;
    entry_data->new_player_index = idx;
  }
  self->current_player_index = idx;
}

void game_begin_setup(Game* self) {
  assert(self->phase == GAME_PHASE_NONE);
  game_set_phase(self, GAME_PHASE_SETUP);
}

void game_end_setup(Game* self) {
  assert(self->phase == GAME_PHASE_SETUP);
  assert(self->board_width > 0);
  assert(self->board_height > 0);
  assert(self->board_grid != NULL);
  assert(self->tile_attributes != NULL);
  assert(self->players_count >= 0);
  if (self->players_count != 0) {
    assert(self->players != NULL);
  }
  assert(self->penguins_per_player >= 0);
  for (int i = 0; i < self->players_count; i++) {
    assert(self->players[i].name != NULL);
    if (self->penguins_per_player != 0) {
      assert(self->players[i].penguins != NULL);
    }
  }
  game_set_phase(self, GAME_PHASE_SETUP_DONE);
}

void game_set_penguins_per_player(Game* self, int value) {
  assert(self->phase == GAME_PHASE_SETUP);
  assert(value >= 0);
  self->penguins_per_player = value;
  for (int i = 0; i < self->players_count; i++) {
    Player* player = &self->players[i];
    player->penguins_count = my_min(player->penguins_count, self->penguins_per_player);
    player->penguins =
      realloc(player->penguins, sizeof(*player->penguins) * self->penguins_per_player);
  }
}

void game_set_players_count(Game* self, int count) {
  assert(self->phase == GAME_PHASE_SETUP);
  assert(count >= 0);
  assert(self->players == NULL);
  self->players = malloc(sizeof(*self->players) * count);
  self->players_count = count;
  for (int i = 0; i < count; i++) {
    Player* player = &self->players[i];
    player->id = (short)(i + 1);
    player->name = NULL;
    player->points = 0;
    player->penguins_count = 0;
    player->penguins = malloc(sizeof(*player->penguins) * my_max(0, self->penguins_per_player));
    player->moves_count = 0;
    player->color = 0;
  }
}

void game_set_player_name(Game* self, int idx, const char* name) {
  assert(self->phase == GAME_PHASE_SETUP);
  Player* player = game_get_player(self, idx);
  free_and_clear(player->name);
  player->name = name ? strdup(name) : NULL;
}

void game_add_player_penguin(Game* self, int idx, Coords coords) {
  Player* player = game_get_player(self, idx);
  assert(0 <= player->penguins_count && player->penguins_count < self->penguins_per_player);
  player->penguins[player->penguins_count++] = coords;
}

void game_remove_player_penguin(Game* self, int idx, Coords coords) {
  Player* player = game_get_player(self, idx);
  Coords* penguin = game_find_player_penguin(self, idx, coords);
  assert(penguin != NULL);
  // The penguin pointer will be within the boundaries of the penguins array,
  // this is legal.
  int penguin_idx = (int)(penguin - player->penguins);
  for (int i = penguin_idx; i < player->penguins_count - 1; i++) {
    player->penguins[i] = player->penguins[i + 1];
  }
  player->penguins_count -= 1;
}

void game_advance_state(Game* self) {
  // Notice that we don't return immediately after making a phase transition:
  // this allows handling multiple consequent transitions in situations like if
  // no player can make any moves after the placement phase ends, the game
  // should just end.
  if (self->phase == GAME_PHASE_SETUP) {
    game_end_setup(self);
  }
  if (self->phase == GAME_PHASE_SETUP_DONE) {
    placement_begin(self);
  }
  if (self->phase == GAME_PHASE_PLACEMENT) {
    int result = placement_switch_player(self);
    if (result < 0) {
      placement_end(self);
      movement_begin(self);
    }
  }
  if (self->phase == GAME_PHASE_MOVEMENT) {
    int result = movement_switch_player(self);
    if (result < 0) {
      movement_end(self);
      game_end(self);
    }
  }
}

void game_end(Game* self) {
  assert(self->phase == GAME_PHASE_SETUP_DONE);
  game_set_current_player(self, -1);
  game_set_phase(self, GAME_PHASE_END);
}

void game_rewind_state_to_log_entry(Game* self, size_t target_entry) {
  bool prev_log_disabled = self->log_disabled;
  // No new entries should be created if we are redoing stuff.
  self->log_disabled = true;

  // Undo
  while (self->log_current > target_entry) {
    const GameLogEntry* entry = game_get_log_entry(self, self->log_current - 1);
    switch (entry->type) {
      case GAME_LOG_ENTRY_PHASE_CHANGE: {
        const GameLogPhaseChange* entry_data =
          &game_pop_log_entry(self, GAME_LOG_ENTRY_PHASE_CHANGE)->data.phase_change;
        assert(self->phase == entry_data->new_phase);
        // Phase switching within the undo/redo system is currently performed
        // without calling the actual phase changing functions (movement_begin,
        // game_end etc) as a simplification. Currently they don't really
        // have much functionality besides checking preconditions and setting
        // the current player, so there isn't much point in calling them anyway.
        self->phase = entry_data->old_phase;
        break;
      }
      case GAME_LOG_ENTRY_PLAYER_CHANGE: {
        const GameLogPlayerChange* entry_data =
          &game_pop_log_entry(self, GAME_LOG_ENTRY_PLAYER_CHANGE)->data.player_change;
        assert(self->current_player_index == entry_data->new_player_index);
        self->current_player_index = entry_data->old_player_index;
        break;
      }
      case GAME_LOG_ENTRY_PLACEMENT: undo_place_penguin(self); break;
      case GAME_LOG_ENTRY_MOVEMENT: undo_move_penguin(self); break;
    }
  }

  // Redo
  for (; self->log_current < target_entry; self->log_current += 1) {
    const GameLogEntry* entry = game_get_log_entry(self, self->log_current);
    switch (entry->type) {
      case GAME_LOG_ENTRY_PHASE_CHANGE: {
        const GameLogPhaseChange* entry_data = &entry->data.phase_change;
        assert(self->phase == entry_data->old_phase);
        // The comment about phase changes from above applies here as well.
        self->phase = entry_data->new_phase;
        break;
      }
      case GAME_LOG_ENTRY_PLAYER_CHANGE: {
        const GameLogPlayerChange* entry_data = &entry->data.player_change;
        assert(self->current_player_index == entry_data->old_player_index);
        self->current_player_index = entry_data->new_player_index;
        break;
      }
      case GAME_LOG_ENTRY_PLACEMENT: {
        const GameLogPlacement* entry_data = &entry->data.placement;
        place_penguin(self, entry_data->target);
        break;
      }
      case GAME_LOG_ENTRY_MOVEMENT: {
        const GameLogMovement* entry_data = &entry->data.movement;
        move_penguin(self, entry_data->penguin, entry_data->target);
        break;
      }
    }
  }

  self->log_disabled = prev_log_disabled;
}
