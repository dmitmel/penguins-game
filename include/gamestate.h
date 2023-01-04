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
