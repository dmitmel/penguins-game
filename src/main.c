#include "arguments.h"
#include "interactive.h"
#include "random.h"
#include <stdio.h>

static const char* MY_PLAYER_NAME = "team-d-provisional";

int main(int argc, char* argv[]) {
  random_init();

  if (argc <= 1) {
    return run_interactive_mode();
  }

  struct Arguments args;
  if (!parse_arguments(&args, argc, argv)) {
    print_usage(argc > 0 ? argv[0] : "");
    return 3;
  }

  if (args.name) {
    printf("%s\n", MY_PLAYER_NAME);
    return 0;
  } else if (args.interactive) {
    return run_interactive_mode();
  } else if (args.phase == PHASE_ARG_PLACEMENT) {
    return 3;
  } else if (args.phase == PHASE_ARG_MOVEMENT) {
    return 3;
  } else {
    return 3;
  }
}
