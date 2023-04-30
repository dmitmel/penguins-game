#include "arguments.h"
#include "autonomous.h"
#include "interactive.h"
#include "penguins-version.h"
#include <stdio.h> // IWYU pragma: keep

int main(int argc, char* argv[]) {
  if (argc <= 1) {
#ifdef INTERACTIVE_MODE
    return run_interactive_mode();
#else
    fprintf(stderr, "The app has been compiled without the interactive mode!\n");
    return EXIT_INTERNAL_ERROR;
#endif
  }

  Arguments args;
  char* prog_name = argc > 0 ? argv[0] : "penguins";
  if (!parse_arguments(&args, argc, argv)) {
    print_usage(prog_name);
    return EXIT_INTERNAL_ERROR;
  }

  if (args.action == ACTION_ARG_PRINT_HELP) {
    print_usage(prog_name);
  } else if (args.action == ACTION_ARG_PRINT_VERSION) {
    fprintf(stderr, "%s v%s\n", prog_name, PENGUINS_VERSION_STRING);
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

  return EXIT_OK;
}
