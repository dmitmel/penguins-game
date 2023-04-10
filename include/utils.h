#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

inline bool coords_same(Coords a, Coords b) {
  return a.x == b.x && a.y == b.y;
}

// Generally several approaches are possible for implementing a similar macro:
// <https://stackoverflow.com/a/3599170/12005228>
// <https://stackoverflow.com/a/3599203/12005228>
// <https://stackoverflow.com/a/12891181/12005228>
#define UNUSED(x) ((void)(x))

#if defined(__GNUC__)
#define ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE
#endif

#define free_and_clear(ptr) (free(ptr), ptr = NULL)

// Taken from <https://stackoverflow.com/a/21338744/12005228>
#define my_max(x, y) (((x) > (y)) ? (x) : (y))
#define my_min(x, y) (((x) < (y)) ? (x) : (y))

#define set_bit(num, bit) ((num) | (bit))
#define clear_bit(num, bit) ((num) & ~(bit))
#define toggle_bit(num, bit) ((num) ^ (bit))
#define change_bit(num, bit, val) (((num) & ~(bit)) | ((val) ? (bit) : 0))
#define test_bit(num, bit) (((num) & (bit)) != 0)

const char* strip_prefix(const char* str, const char* prefix);
bool parse_number(const char* str, long* result);
void* memdup(const void* src, size_t size);

typedef struct Rng {
  int (*random_range)(struct Rng* self, int min, int max);
} Rng;

Rng init_stdlib_rng(void);

#define FNV32_INITIAL_STATE ((uint32_t)2166136261)
#define FNV32_PRIME ((uint32_t)16777619)

// <https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function>
inline uint32_t fnv32_hash(uint32_t state, const void* buf, size_t len) {
  const uint8_t* ptr = (const uint8_t*)buf;
  const uint8_t* end = ptr + len;
  for (; ptr != end; ptr++) state = (state ^ *ptr) * FNV32_PRIME;
  return state;
}

#ifdef __cplusplus
}
#endif
