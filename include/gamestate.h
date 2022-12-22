#pragma once

typedef struct Player {
  int id;
  char name[16];
  int points;
} Player;

typedef struct Coords {
  int x;
  int y;
} Coords;