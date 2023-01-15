#include "interactive.h"
#include "random.h"
#include "utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int UNUSED(argc), char* argv[] UNUSED_ATTR) {
  random_init();

  return run_interactive_mode();
}
