#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PhaseArg {
  PHASE_ARG_NONE = 0,
  PHASE_ARG_PLACEMENT = 1,
  PHASE_ARG_MOVEMENT = 2,
} PhaseArg;

typedef struct Arguments {
  PhaseArg phase;
  int penguins;
  const char* input_board_file;
  const char* output_board_file;
  bool print_name;
  bool interactive;
  const char* set_name;
} Arguments;

void print_usage(const char* prog_name);
bool parse_arguments(Arguments* result, int argc, char* argv[]);

#ifdef __cplusplus
}
#endif
