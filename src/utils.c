#include "utils.h"
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

// A wrapper around the strtol function. Unlike atoi it can reliably tell us
// whether an error has occurred, whereas atoi simply returns zero, and there
// is no way to distinguish between an error or the string legitimately
// containing a zero. See <https://en.cppreference.com/w/c/string/byte/strtol>.
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

// Taken from <https://stackoverflow.com/a/29035370>.
void* memdup(const void* src, size_t size) {
  void* dest = malloc(size);
  if (dest != NULL) memcpy(dest, src, size);
  return dest;
}
