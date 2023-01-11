#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Player {
  int id;
  char name[16];
  int points;
} Player;

enum GamePhase {
  PHASE_PLACEMENT = 1,
  PHASE_MOVEMENT = 2,
};

typedef struct GameState {
  Player* players;
  int player_count;
  int penguin_count;
} GameState;

#ifdef __cplusplus
}
#endif
