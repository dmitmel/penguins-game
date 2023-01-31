#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Coords {
  int x;
  int y;
} Coords;

typedef enum Direction {
  DIRECTION_RIGHT,
  DIRECTION_DOWN,
  DIRECTION_LEFT,
  DIRECTION_UP,
  DIRECTION_MAX,
} Direction;

typedef enum Neighbor {
  NEIGHBOR_RIGHT,
  NEIGHBOR_BOTTOM_RIGHT,
  NEIGHBOR_BOTTOM,
  NEIGHBOR_BOTTOM_LEFT,
  NEIGHBOR_LEFT,
  NEIGHBOR_TOP_LEFT,
  NEIGHBOR_TOP,
  NEIGHBOR_TOP_RIGHT,
  NEIGHBOR_MAX,
} Neighbor;

extern const Coords DIRECTION_TO_COORDS[DIRECTION_MAX];
extern const Coords NEIGHBOR_TO_COORDS[NEIGHBOR_MAX];
extern const Neighbor DIRECTION_TO_NEIGHBOR[DIRECTION_MAX];

// Generally several approaches are possible for implementing this macro:
// <https://stackoverflow.com/a/3599170/12005228>
// <https://stackoverflow.com/a/3599203/12005228>
// <https://stackoverflow.com/a/12891181/12005228>
#ifdef __GNUC__
#define UNUSED_ATTR __attribute__((unused))
#define UNUSED(x) UNUSED_##x UNUSED_ATTR
#else
#define UNUSED_ATTR
#define UNUSED(x) UNUSED_##x
#endif

#if defined(__GNUC__)
#define ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
#define ALWAYS_INLINE __forceinline
#endif

#define free_and_clear(ptr) (free(ptr), ptr = NULL)

// Taken from <https://stackoverflow.com/a/21338744/12005228>
#define my_max(x, y) (((x) > (y)) ? (x) : (y))
#define my_min(x, y) (((x) < (y)) ? (x) : (y))

const char* strip_prefix(const char* str, const char* prefix);
bool parse_number(const char* str, long* result);

#ifdef __cplusplus
}
#endif
