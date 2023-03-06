#include "tui/main.h"
#include "arguments.h"
#include "autonomous.h"
#include "interactive.h"
#include "random.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
  random_init();

  if (argc <= 1) {
#if defined(PSEUDOGRAPHICAL_MODE)
    return run_pseudographical_mode();
#elif defined(INTERACTIVE_MODE)
    return run_interactive_mode();
#else
    fprintf(stderr, "The app has been compiled without the interactive mode!\n");
    return EXIT_INTERNAL_ERROR;
#endif
  }

  Arguments args;
  if (!parse_arguments(&args, argc, argv)) {
    print_usage(argc > 0 ? argv[0] : "");
    return EXIT_INTERNAL_ERROR;
  }

  if (args.action == ACTION_ARG_PSEUDOGRAPHICAL) {
#ifdef PSEUDOGRAPHICAL_MODE
    return run_pseudographical_mode();
#endif
    fprintf(stderr, "The app has been compiled without the pseudographical interface!\n");
    return EXIT_INTERNAL_ERROR;
  } else if (args.action == ACTION_ARG_INTERACTIVE) {
#ifdef INTERACTIVE_MODE
    return run_interactive_mode();
#else
    fprintf(stderr, "The app has been compiled without the interactive mode!\n");
    return EXIT_INTERNAL_ERROR;
#endif
  } else {
#ifdef AUTONOMOUS_MODE
    return run_autonomous_mode(&args);
#else
    fprintf(stderr, "The app has been compiled without the autonomous mode!\n");
    return EXIT_INTERNAL_ERROR;
#endif
  }
}
