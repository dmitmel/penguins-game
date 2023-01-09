#pragma once

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
  int playerCount;
  int penguinCount;
} GameState;

typedef struct Coords {
  int x;
  int y;
} Coords;
