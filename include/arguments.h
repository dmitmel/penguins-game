#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum BotPlacementStrategy {
  BOT_PLACEMENT_SMART,
  BOT_PLACEMENT_RANDOM,
  BOT_PLACEMENT_FIRST_POSSIBLE,
  BOT_PLACEMENT_MOST_FISH,
} BotPlacementStrategy;

typedef enum BotMovementStrategy {
  BOT_MOVEMENT_SMART,
  BOT_MOVEMENT_RANDOM,
  BOT_MOVEMENT_FIRST_POSSIBLE,
} BotMovementStrategy;

typedef struct BotParameters {
  BotPlacementStrategy placement_strategy;
  int placement_scan_area;
  BotMovementStrategy movement_strategy;
  int max_move_length;
  int recursion_limit;
} BotParameters;

typedef enum ActionArg {
  ACTION_ARG_INTERACTIVE,
  ACTION_ARG_PRINT_NAME,
  ACTION_ARG_PLACEMENT,
  ACTION_ARG_MOVEMENT,
} ActionArg;

typedef struct Arguments {
  ActionArg action;
  int penguins;
  const char* input_board_file;
  const char* output_board_file;
  const char* set_name;
  BotParameters bot;
} Arguments;

void print_usage(const char* prog_name);
bool parse_arguments(Arguments* result, int argc, char* argv[]);

#ifdef __cplusplus
}
#endif
