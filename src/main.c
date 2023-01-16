#include "arguments.h"
#include "autonomous.h"
#include "interactive.h"
#include "random.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
  random_init();

  if (argc <= 1) {
    return run_interactive_mode();
  }

  Arguments args;
  if (!parse_arguments(&args, argc, argv)) {
    print_usage(argc > 0 ? argv[0] : "");
    return EXIT_INTERNAL_ERROR;
  }

  if (args.name) {
    printf("%s\n", MY_AUTONOMOUS_PLAYER_NAME);
    return EXIT_OK;
  } else if (args.interactive) {
    return run_interactive_mode();
  } else {
    return run_autonomous_mode(&args);
  }
}
