#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum PhaseArg {
  PHASE_ARG_NONE = 0,
  PHASE_ARG_PLACEMENT = 1,
  PHASE_ARG_MOVEMENT = 2,
};

struct Arguments {
  enum PhaseArg phase;
  int penguins;
  const char* input_board_file;
  const char* output_board_file;
  bool name;
  bool interactive;
};

void print_usage(const char* prog_name);
bool parse_arguments(struct Arguments* result, int argc, char* argv[]);

#ifdef __cplusplus
}
#endif
