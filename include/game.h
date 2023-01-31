#pragma once

#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

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
  int id;
  char* name;
  int points;
  int penguins_count;
  Coords* penguins;
  int moves_count;
  int color;
} Player;

typedef struct Game {
  GamePhase phase;
  Player* players;
  int players_count;
  int penguins_per_player;
  int board_width;
  int board_height;
  int* board_grid;
  int current_player_index;
} Game;

Game* game_new(void);
void game_free(Game* self);

void game_begin_setup(Game* self);
void game_end_setup(Game* self);

void game_set_penguins_per_player(Game* self, int value);
void game_set_players_count(Game* self, int count);
void game_set_player_name(Game* self, int idx, const char* name);
void game_set_player_score(Game* self, int idx, int points);
void game_add_player_penguin(Game* self, int idx, Coords coords);

void game_advance_state(Game* self);
void game_end(Game* self);

inline Player* game_get_player(const Game* self, int idx) {
  assert(0 <= idx && idx < self->players_count);
  return &self->players[idx];
}

inline Player* game_get_current_player(const Game* self) {
  return game_get_player(self, self->current_player_index);
}

inline int game_find_player_by_id(const Game* self, int id) {
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
