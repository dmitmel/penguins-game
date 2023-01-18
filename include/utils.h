#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Coords {
  int x;
  int y;
} Coords;

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

#define free_and_clear(ptr) (free(ptr), ptr = NULL)

// Taken from <https://stackoverflow.com/a/21338744/12005228>
#define my_max(x, y) (((x) > (y)) ? (x) : (y))
#define my_min(x, y) (((x) < (y)) ? (x) : (y))

#ifdef __cplusplus
}
#endif
