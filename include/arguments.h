#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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
} Arguments;

void print_usage(const char* prog_name);
bool parse_arguments(Arguments* result, int argc, char* argv[]);

#ifdef __cplusplus
}
#endif
