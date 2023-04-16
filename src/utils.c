#include "utils.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

extern bool coords_same(Coords a, Coords b);
extern uint32_t fnv32_hash(uint32_t state, const void* buf, size_t len);

const Coords DIRECTION_TO_COORDS[DIRECTION_MAX] = {
  [DIRECTION_RIGHT] = { 1, 0 },
  [DIRECTION_DOWN] = { 0, 1 },
  [DIRECTION_LEFT] = { -1, 0 },
  [DIRECTION_UP] = { 0, -1 },
};

const Coords NEIGHBOR_TO_COORDS[NEIGHBOR_MAX] = {
  [NEIGHBOR_RIGHT] = { 1, 0 },  [NEIGHBOR_BOTTOM_RIGHT] = { 1, 1 },
  [NEIGHBOR_BOTTOM] = { 0, 1 }, [NEIGHBOR_BOTTOM_LEFT] = { -1, 1 },
  [NEIGHBOR_LEFT] = { -1, 0 },  [NEIGHBOR_TOP_LEFT] = { -1, -1 },
  [NEIGHBOR_TOP] = { 0, -1 },   [NEIGHBOR_TOP_RIGHT] = { 1, -1 },
};

const Neighbor DIRECTION_TO_NEIGHBOR[DIRECTION_MAX] = {
  [DIRECTION_RIGHT] = NEIGHBOR_RIGHT,
  [DIRECTION_DOWN] = NEIGHBOR_BOTTOM,
  [DIRECTION_LEFT] = NEIGHBOR_LEFT,
  [DIRECTION_UP] = NEIGHBOR_TOP,
};

const char* strip_prefix(const char* str, const char* prefix) {
  // Based on <https://stackoverflow.com/a/4770992>
  size_t prefix_len = strlen(prefix);
  return strncmp(prefix, str, prefix_len) == 0 ? str + prefix_len : NULL;
}

bool parse_number(const char* str, long* result) {
  char* end;
  while (isspace(*str)) str++; // Skip the leading whitespace
  errno = 0;
  *result = strtol(str, &end, 10);
  while (isspace(*end)) end++; // Skip the trailing whitespace
  return !(
    // The parsing fails if:
    errno == ERANGE || // The value falls outside the range of the `long` type
    end == str ||      // The string is empty or nothing could be parsed at all
    *end != '\0'       // There is junk at the end of the string
  );
}

void* memdup(const void* src, size_t size) {
  // Taken from <https://stackoverflow.com/a/29035370>.
  void* dest = malloc(size);
  if (dest != NULL) memcpy(dest, src, size);
  return dest;
}

// Taken from <https://cplusplus.com/faq/beginners/random-numbers/#seeding>.
static void random_init(void) {
#ifdef _WIN32
  FILETIME t;
  ULARGE_INTEGER i;
  GetSystemTimeAsFileTime(&t);
  i.u.LowPart = t.dwLowDateTime;
  i.u.HighPart = t.dwHighDateTime;
  ULONGLONG seed = i.QuadPart / 1000;
#else
  struct timespec ts, res;
  clock_getres(CLOCK_REALTIME, &res);
  clock_gettime(CLOCK_REALTIME, &ts);
  unsigned long seed = ts.tv_nsec / res.tv_nsec;
#endif
  srand((unsigned int)seed);
}

// Taken from <https://cplusplus.com/faq/beginners/random-numbers/#random_int_in_range>.
static int random_range(Rng* rng, int min, int max) {
  UNUSED(rng);
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

Rng init_stdlib_rng(void) {
  random_init();
  Rng rng;
  rng.random_range = &random_range;
  return rng;
}
