#pragma once

#include <stdbool.h>

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
  int penguins;
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

Player* game_get_player(const Game* self, int idx);
Player* game_get_player_by_id(const Game* self, int id);
int game_get_current_player_id(const Game* self);

void game_set_player_name(Game* self, int idx, const char* name);
void game_set_player_score(Game* self, int idx, int points);

void game_advance_state(Game* self);
void game_end(Game* self);

#ifdef __cplusplus
}
#endif
