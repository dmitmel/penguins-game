#include "random.h"
#include <assert.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

// Taken from <https://cplusplus.com/faq/beginners/random-numbers/#seeding>.
void random_init(void) {
  unsigned long seed;
#ifdef _WIN32
  FILETIME t;
  ULARGE_INTEGER i;
  GetSystemTimeAsFileTime(&t);
  i.u.LowPart = t.dwLowDateTime;
  i.u.HighPart = t.dwHighDateTime;
  seed = i.QuadPart / 1000;
#else
  struct timespec ts, res;
  clock_getres(CLOCK_REALTIME, &res);
  clock_gettime(CLOCK_REALTIME, &ts);
  seed = ts.tv_nsec / res.tv_nsec;
#endif
  srand(seed);
}

// Taken from <https://cplusplus.com/faq/beginners/random-numbers/#random_int_in_range>.
int random_range(int min, int max) {
  assert(min <= max);
  unsigned int n = (max - min <= RAND_MAX) ? (max - min + 1U) : (RAND_MAX + 1U);
  unsigned int x = (RAND_MAX + 1U) / n;
  unsigned int y = x * n;
  unsigned int r;
  do {
    r = rand();
  } while (r >= y);
  return r / x + min;
}
