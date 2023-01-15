#include "game.h"
#include "movement.h"
#include "placement.h"
#include "utils.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

Game* game_new(void) {
  Game* self = malloc(sizeof(Game));
  self->phase = GAME_PHASE_NONE;
  self->players = NULL;
  self->players_count = -1;
  self->penguins_per_player = -1;
  self->board_width = -1;
  self->board_height = -1;
  self->board_grid = NULL;
  self->current_player_index = -1;
  return self;
}

void game_free(Game* self) {
  if (self == NULL) return;
  if (self->players) {
    for (int i = 0; i < self->players_count; i++) {
      free_and_clear(self->players[i].name);
    }
    free_and_clear(self->players);
  }
  free_and_clear(self->board_grid);
  free(self);
}

void game_begin_setup(Game* self) {
  assert(self->phase == GAME_PHASE_NONE);
  self->phase = GAME_PHASE_SETUP;
}

void game_end_setup(Game* self) {
  assert(self->phase == GAME_PHASE_SETUP);
  assert(self->players != NULL);
  assert(self->players_count > 0);
  assert(self->penguins_per_player > 0);
  for (int i = 0; i < self->players_count; i++) {
    assert(self->players[i].name != NULL);
  }
  self->phase = GAME_PHASE_SETUP_DONE;
}

void game_set_penguins_per_player(Game* self, int value) {
  assert(self->phase == GAME_PHASE_SETUP);
  assert(value > 0);
  self->penguins_per_player = value;
}

void game_set_players_count(Game* self, int count) {
  assert(self->phase == GAME_PHASE_SETUP);
  assert(count > 0);
  assert(self->players == NULL);
  self->players = malloc(sizeof(Player) * count);
  self->players_count = count;
  for (int i = 0; i < count; i++) {
    Player* player = &self->players[i];
    player->id = i + 1;
    player->name = NULL;
    player->points = 0;
    player->penguins = 0;
  }
}

Player* game_get_player(const Game* self, int idx) {
  assert(0 <= idx && idx < self->players_count);
  return &self->players[idx];
}

Player* game_get_player_by_id(const Game* self, int id) {
  assert(1 <= id && id <= self->players_count);
  return &self->players[id - 1];
}

int game_get_current_player_id(const Game* self) {
  return game_get_player(self, self->current_player_index)->id;
}

void game_set_player_name(Game* self, int idx, const char* name) {
  assert(self->phase == GAME_PHASE_SETUP);
  Player* player = game_get_player(self, idx);
  free_and_clear(player->name);
  player->name = strdup(name);
}

void game_set_player_score(Game* self, int idx, int points) {
  game_get_player(self, idx)->points = points;
}

void game_advance_state(Game* self) {
  if (self->phase == GAME_PHASE_SETUP) {
    game_end_setup(self);
  }
  if (self->phase == GAME_PHASE_SETUP_DONE) {
    placement_begin(self);
  }
  if (self->phase == GAME_PHASE_PLACEMENT) {
    int result = placement_switch_player(self);
    if (result < 0) placement_end(self);
  }
  if (self->phase == GAME_PHASE_PLACEMENT_DONE) {
    movement_begin(self);
  }
  if (self->phase == GAME_PHASE_MOVEMENT) {
    int result = movement_switch_player(self);
    if (result < 0) movement_end(self);
  }
  if (self->phase == GAME_PHASE_MOVEMENT_DONE) {
    game_end(self);
  }
}

void game_end(Game* self) {
  assert(self->phase >= GAME_PHASE_SETUP_DONE);
  self->phase = GAME_PHASE_END;
}
