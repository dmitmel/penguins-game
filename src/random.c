#include "random.h"
#include <stdlib.h>
#include <time.h>

void random_init(void) {
  srand(time(NULL));
}

// Taken from <https://stackoverflow.com/a/18386648>
int random_range(int min, int max) {
  return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}
