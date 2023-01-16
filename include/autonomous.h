#pragma once

#include "arguments.h"
#include "game.h"
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum AutonomousExitCode {
  EXIT_OK = 0,
  EXIT_NO_POSSIBLE_MOVES = 1,
  EXIT_INPUT_FILE_ERROR = 2,
  EXIT_INTERNAL_ERROR = 3,
} AutonomousExitCode;

extern const char* MY_AUTONOMOUS_PLAYER_NAME;

int run_autonomous_mode(const Arguments* args);

bool load_game_state(Game* game, FILE* file, int penguins_arg);
bool save_game_state(const Game* game, FILE* file);

#ifdef __cplusplus
}
#endif
