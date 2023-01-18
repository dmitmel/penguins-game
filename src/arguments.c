#include "arguments.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char* prog_name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s phase=placement penguins=N inputboard.txt outpuboard.txt\n", prog_name);
  fprintf(stderr, "%s phase=movement board.txt board.txt\n", prog_name);
  fprintf(stderr, "%s name\n", prog_name);
  fprintf(stderr, "%s interactive\n", prog_name);
}

static const char* strip_prefix(const char* str, const char* prefix) {
  // Based on <https://stackoverflow.com/a/4770992>
  int prefix_len = strlen(prefix);
  return strncmp(prefix, str, prefix_len) == 0 ? str + prefix_len : NULL;
}

bool parse_arguments(Arguments* result, int argc, char* argv[]) {
  result->action = ACTION_ARG_INTERACTIVE;
  result->penguins = 0;
  result->input_board_file = NULL;
  result->output_board_file = NULL;
  result->set_name = NULL;

  bool ok = true;
  int file_arg = 0;
  for (int i = 1; i < argc; i++) {
    const char* arg = argv[i];
    const char* arg_value;
    if ((arg_value = strip_prefix(arg, "phase="))) {
      if (strcmp(arg_value, "placement") == 0) {
        result->action = ACTION_ARG_PLACEMENT;
      } else if (strcmp(arg_value, "movement") == 0) {
        result->action = ACTION_ARG_MOVEMENT;
      } else {
        fprintf(stderr, "Invalid value for the 'phase' argument: '%s'\n", arg_value);
        ok = false;
      }
    } else if ((arg_value = strip_prefix(arg, "penguins="))) {
      result->penguins = atoi(arg_value);
      if (result->penguins <= 0) {
        fprintf(stderr, "Invalid value for the 'penguins' argument: '%s'\n", arg_value);
        ok = false;
      }
    } else if (strcmp(arg, "name") == 0) {
      result->action = ACTION_ARG_PRINT_NAME;
    } else if (strcmp(arg, "interactive") == 0) {
      result->action = ACTION_ARG_INTERACTIVE;
    } else if ((arg_value = strip_prefix(arg, "name="))) {
      if (arg_value[0] == '\0') {
        ok = false;
        fprintf(stderr, "Invalid value for the 'name' argument: '%s'\n", arg_value);
      } else {
        result->set_name = arg_value;
      }
    } else if (file_arg == 0) {
      result->input_board_file = arg;
      file_arg++;
    } else if (file_arg == 1) {
      result->output_board_file = arg;
      file_arg++;
    } else {
      fprintf(stderr, "Unexpected argument: '%s'\n", arg);
      ok = false;
    }
  }

  if (result->action == ACTION_ARG_PLACEMENT || result->action == ACTION_ARG_MOVEMENT) {
    if (result->input_board_file == NULL) {
      fprintf(stderr, "Expected a value for the required argument 'input_board_file'\n");
      ok = false;
    }
    if (result->output_board_file == NULL) {
      fprintf(stderr, "Expected a value for the required argument 'output_board_file'\n");
      ok = false;
    }
  }

  if (result->action == ACTION_ARG_PLACEMENT) {
    if (result->penguins <= 0) {
      fprintf(stderr, "Expected a value for the 'penguins' argument\n");
      ok = false;
    }
  }

  return ok;
}
