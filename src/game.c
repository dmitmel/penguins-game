#include "game.h"
#include "movement.h"
#include "placement.h"
#include "utils.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

extern Player* game_get_player(const Game* self, int idx);
extern Player* game_get_current_player(const Game* self);
extern int game_find_player_by_id(const Game* self, int id);
extern Coords* game_find_player_penguin(const Game* self, int idx, Coords coords);

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
      free_and_clear(self->players[i].penguins);
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
  assert(self->board_width > 0);
  assert(self->board_height > 0);
  assert(self->board_grid != NULL);
  assert(self->players != NULL);
  assert(self->players_count > 0);
  assert(self->penguins_per_player >= 0);
  for (int i = 0; i < self->players_count; i++) {
    assert(self->players[i].name != NULL);
    assert(self->players[i].penguins != NULL);
  }
  self->phase = GAME_PHASE_SETUP_DONE;
}

void game_set_penguins_per_player(Game* self, int value) {
  assert(self->phase == GAME_PHASE_SETUP);
  assert(value > 0);
  self->penguins_per_player = value;
  for (int i = 0; i < self->players_count; i++) {
    Player* player = &self->players[i];
    player->penguins_count = my_min(player->penguins_count, self->penguins_per_player);
    player->penguins = realloc(player->penguins, sizeof(Coords) * self->penguins_per_player);
  }
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
    player->penguins_count = 0;
    player->penguins = malloc(sizeof(Coords) * my_max(0, self->penguins_per_player));
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
  self->current_player_index = -1;
}
