#include "arguments.h"
#include "bot.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void init_arguments(Arguments* self) {
  self->action = ACTION_ARG_INTERACTIVE;
  self->penguins = 0;
  self->input_board_file = NULL;
  self->output_board_file = NULL;
  self->set_name = NULL;
  self->board_gen_width = 0;
  self->board_gen_height = 0;
  self->board_gen_type = GENERATE_ARG_NONE;
  init_bot_parameters(&self->bot);
}

void print_usage(const char* prog_name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s help", prog_name);
  fprintf(stderr, "%s version", prog_name);
#ifdef AUTONOMOUS_MODE
  fprintf(stderr, "%s phase=placement penguins=N inputboard.txt outpuboard.txt\n", prog_name);
  fprintf(stderr, "%s phase=movement board.txt board.txt\n", prog_name);
  fprintf(stderr, "%s generate <island|random> <WIDTH> <HEIGHT> board.txt\n", prog_name);
  fprintf(stderr, "%s view board.txt\n", prog_name);
  fprintf(stderr, "%s name\n", prog_name);
#endif
#ifdef INTERACTIVE_MODE
  fprintf(stderr, "%s interactive\n", prog_name);
#endif
}

bool parse_arguments(Arguments* result, int argc, char* argv[]) {
  init_arguments(result);

  bool ok = true;
  int file_arg = 0;
  for (int i = 1; i < argc; i++) {
    const char* arg = argv[i];
    const char* arg_value = arg;
    bool is_placement_or_movement =
      result->action == ACTION_ARG_PLACEMENT || result->action == ACTION_ARG_MOVEMENT;
    bool is_board_gen = result->action == ACTION_ARG_GENERATE;
    long num = 0;
    if ((arg_value = strip_prefix(arg, "phase="))) {
      if (strcmp(arg_value, "placement") == 0) {
        result->action = ACTION_ARG_PLACEMENT;
      } else if (strcmp(arg_value, "movement") == 0) {
        result->action = ACTION_ARG_MOVEMENT;
      } else {
        fprintf(stderr, "Invalid value for the 'phase' option: '%s'\n", arg_value);
        ok = false;
      }
    } else if ((arg_value = strip_prefix(arg, "penguins="))) {
      if (parse_number(arg_value, &num) && num > 0) {
        result->penguins = (int)num;
      } else {
        fprintf(stderr, "Invalid value for the 'penguins' option: '%s'\n", arg_value);
        ok = false;
      }
    } else if (strcmp(arg, "version") == 0) {
      result->action = ACTION_ARG_PRINT_VERSION;
    } else if (strcmp(arg, "help") == 0) {
      result->action = ACTION_ARG_PRINT_HELP;
    } else if (strcmp(arg, "name") == 0) {
      result->action = ACTION_ARG_PRINT_NAME;
    } else if (strcmp(arg, "interactive") == 0) {
      result->action = ACTION_ARG_INTERACTIVE;
    } else if (strcmp(arg, "generate") == 0) {
      result->action = ACTION_ARG_GENERATE;
    } else if (strcmp(arg, "view") == 0) {
      result->action = ACTION_ARG_VIEW;
    } else if ((arg_value = strip_prefix(arg, "name="))) {
      if (*arg_value != '\0') {
        result->set_name = arg_value;
      } else {
        ok = false;
        fprintf(stderr, "Invalid value for the 'name' option: '%s'\n", arg_value);
      }
    } else if ((arg_value = strip_prefix(arg, "bot-placement="))) {
      if (strcmp(arg_value, "smart") == 0) {
        result->bot.placement_strategy = BOT_PLACEMENT_SMART;
      } else if (strcmp(arg_value, "random") == 0) {
        result->bot.placement_strategy = BOT_PLACEMENT_RANDOM;
      } else if (strcmp(arg_value, "first") == 0) {
        result->bot.placement_strategy = BOT_PLACEMENT_FIRST_POSSIBLE;
      } else if (strcmp(arg_value, "fish") == 0) {
        result->bot.placement_strategy = BOT_PLACEMENT_MOST_FISH;
      } else {
        ok = false;
        fprintf(stderr, "Invalid value for the 'bot-placement' option: '%s'\n", arg_value);
      }
    } else if ((arg_value = strip_prefix(arg, "bot-placement-scan-area="))) {
      if (parse_number(arg_value, &num) && num >= 0) {
        result->bot.placement_scan_area = (int)num;
      } else {
        fprintf(
          stderr, "Invalid value for the 'bot-placement-scan-area' option: '%s'\n", arg_value
        );
        ok = false;
      }
    } else if ((arg_value = strip_prefix(arg, "bot-movement="))) {
      if (strcmp(arg_value, "smart") == 0) {
        result->bot.movement_strategy = BOT_MOVEMENT_SMART;
      } else if (strcmp(arg_value, "random") == 0) {
        result->bot.movement_strategy = BOT_MOVEMENT_RANDOM;
      } else if (strcmp(arg_value, "first") == 0) {
        result->bot.movement_strategy = BOT_MOVEMENT_FIRST_POSSIBLE;
      } else {
        ok = false;
        fprintf(stderr, "Invalid value for the 'bot-movement' option: '%s'\n", arg_value);
      }
    } else if ((arg_value = strip_prefix(arg, "bot-max-move-steps="))) {
      if (parse_number(arg_value, &num) && num >= 0) {
        result->bot.max_move_length = (int)num;
      } else {
        fprintf(stderr, "Invalid value for the 'bot-max-move-steps' option: '%s'\n", arg_value);
        ok = false;
      }
    } else if ((arg_value = strip_prefix(arg, "bot-recursion="))) {
      if (parse_number(arg_value, &num) && num >= 0) {
        result->bot.recursion_limit = (int)num;
      } else {
        fprintf(stderr, "Invalid value for the 'bot-recursion' option: '%s'\n", arg_value);
        ok = false;
      }
    } else if ((arg_value = strip_prefix(arg, "bot-junction-check-recursion="))) {
      if (parse_number(arg_value, &num) && num >= -1) {
        result->bot.junction_check_recursion_limit = (int)num;
      } else {
        fprintf(
          stderr, "Invalid value for the 'bot-junction-check-recursion' option: '%s'\n", arg_value
        );
        ok = false;
      }
    } else if (is_board_gen && file_arg == 0) {
      if (strcmp(arg, "island") == 0) {
        result->board_gen_type = GENERATE_ARG_ISLAND;
      } else if (strcmp(arg, "random") == 0) {
        result->board_gen_type = GENERATE_ARG_RANDOM;
      } else {
        fprintf(stderr, "Invalid value for the 'board_gen_type' option: '%s'\n", arg);
        ok = false;
      }
      file_arg++;
    } else if (is_board_gen && file_arg == 1) {
      if (parse_number(arg, &num) && num >= 1) {
        result->board_gen_width = (int)num;
      } else {
        fprintf(stderr, "Invalid value for the 'WIDTH' option: '%s'\n", arg);
        ok = false;
      }
      file_arg++;
    } else if (is_board_gen && file_arg == 2) {
      if (parse_number(arg, &num) && num >= 1) {
        result->board_gen_height = (int)num;
      } else {
        fprintf(stderr, "Invalid value for the 'HEIGHT' option: '%s'\n", arg);
        ok = false;
      }
      file_arg++;
    } else if (is_board_gen && file_arg == 3) {
      result->output_board_file = arg;
      file_arg++;
    } else if (result->action == ACTION_ARG_VIEW && file_arg == 0) {
      result->input_board_file = arg;
      file_arg++;
    } else if (is_placement_or_movement && file_arg == 0) {
      result->input_board_file = arg;
      file_arg++;
    } else if (is_placement_or_movement && file_arg == 1) {
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
      fprintf(stderr, "Expected a value for the 'penguins' option\n");
      ok = false;
    }
  }

  if (result->action == ACTION_ARG_GENERATE) {
    if (result->board_gen_type == GENERATE_ARG_NONE) {
      fprintf(stderr, "Expected a value for the required argument 'board_gen_type'\n");
      ok = false;
    }
    if (result->board_gen_width <= 0) {
      fprintf(stderr, "Expected a value for the required argument 'WIDTH'\n");
      ok = false;
    }
    if (result->board_gen_height <= 0) {
      fprintf(stderr, "Expected a value for the required argument 'HEIGHT'\n");
      ok = false;
    }
    if (result->output_board_file == NULL) {
      fprintf(stderr, "Expected a value for the required argument 'output_board_file'\n");
      ok = false;
    }
  }

  if (result->action == ACTION_ARG_VIEW) {
    if (result->input_board_file == NULL) {
      fprintf(stderr, "Expected a value for the required argument 'input_board_file'\n");
      ok = false;
    }
  }

  return ok;
}
