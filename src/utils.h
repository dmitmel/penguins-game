#pragma once

/// @file
/// @brief Various miscellaneous utilities

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// A pair of 2D coordinates, used for addressing the #Game::board_grid.
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

/// A table that maps #Direction variants to relative #Coords.
extern const Coords DIRECTION_TO_COORDS[DIRECTION_MAX];
/// A table that maps #Neighbor variants to relative #Coords.
extern const Coords NEIGHBOR_TO_COORDS[NEIGHBOR_MAX];
/// A table that maps #Direction variants to corresponding #Neighbor variants.
extern const Neighbor DIRECTION_TO_NEIGHBOR[DIRECTION_MAX];

/// @relatedalso Coords
/// @brief Checks if two #Coords pairs are equal.
inline bool coords_same(Coords a, Coords b) {
  return a.x == b.x && a.y == b.y;
}

/// @brief Helper for silencing warnings about unused variables.
///
/// To use it just pass the name of the variable you want to hide the usage
/// warning for, like this:
/// @code{.c}
/// void function(int some_param) {
///   UNUSED(some_param);
///   int some_var = 123;
///   UNUSED(some_var);
/// }
/// @endcode
///
/// One notable use is getting rid of warnings related to variables used in @c
/// assert conditions. When the assertions are disabled, the condition passed
/// to the @c assert macro effectively becomes a useless bit of text and the
/// compiler can't find any usages of variables within it since the condition
/// itself doesn't even get parsed.
///
// Generally several approaches are possible for implementing a similar macro:
// <https://stackoverflow.com/a/3599170/12005228>
// <https://stackoverflow.com/a/3599203/12005228>
// <https://stackoverflow.com/a/12891181/12005228>
#define UNUSED(var) ((void)(var))

/// @brief Strongly suggests to the compiler that a function should be inlined.
///
/// Can be used to ask the compiler to inline a function even when inlining is
/// disabled (in the debug configuration, for example). Note that this is still
/// treated by the compilers as a hint, though an authoritative one.
///
/// @see <https://learn.microsoft.com/en-us/cpp/cpp/inline-functions-cpp?view=msvc-170>
/// @see <https://clang.llvm.org/docs/AttributeReference.html#always-inline-force-inline>
/// @see <https://gcc.gnu.org/onlinedocs/gcc/Inline.html>
#if defined(__GNUC__)
#define ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE
#endif

/// Calls @c free on a pointer and then sets it to @c NULL.
#define free_and_clear(ptr) (free((ptr)), (ptr) = NULL)

/// Computes the number of elements of an array.
#define sizeof_array(ptr) (sizeof((ptr)) / sizeof(*(ptr)))

// Taken from <https://stackoverflow.com/a/21338744/12005228>
/// Compares two numbers and returns the larger one.
#define my_max(x, y) (((x) > (y)) ? (x) : (y))
/// Compares two numbers and returns the smaller one.
#define my_min(x, y) (((x) < (y)) ? (x) : (y))

/// @name Macros for working with bitfields
/// @{
#define set_bit(num, bit) ((num) | (bit))
#define clear_bit(num, bit) ((num) & ~(bit))
#define toggle_bit(num, bit) ((num) ^ (bit))
#define change_bit(num, bit, val) (((num) & ~(bit)) | ((val) ? (bit) : 0))
#define test_bit(num, bit) (((num) & (bit)) != 0)
/// @}

/// @brief Returns a substring with the prefix removed if the given string
/// starts with the prefix, otherwise returns @c NULL.
const char* strip_prefix(const char* str, const char* prefix);

/// @brief Converts a string into a number, returns @c false if the string was
/// invalid.
///
/// A wrapper around the @c strtol function, which, unlike @c atoi, can
/// reliably tell us whether an error has occurred, whereas @c atoi simply
/// returns zero, so there is no way to distinguish between an error or the
/// string legitimately containing a zero.
///
/// @param[in] str
/// @param[out] result
///
/// @see <https://en.cppreference.com/w/c/string/byte/strtol>
bool parse_number(const char* str, long* result);

/// A shorthand for @c malloc + @c memcpy (analogous to @c strdup).
void* memdup(const void* src, size_t size);

/// @brief A wrapper around random number generators.
///
/// We need this to abstract away the different implementations of randomness
/// under different environments:
///
/// 1. The TUI and the CLI use the @c rand function from the standard C library.
/// 2. The GUI has #BetterRng which wraps the standard C++ functions from @c \<random\>.
/// 3. The test suite calls @c munit_rand_int_range (see <https://nemequ.github.io/munit/#prng>).
///
/// The struct itself holds pointers to the actual functions of implementations
/// and works kind of like an interface in OOP terminology.
typedef struct Rng {
  /// @brief Generates and returns a value in the range <tt>[min; max)</tt>
  /// (i.e. from @c min inclusive to @c max exclusive).
  int (*random_range)(struct Rng* self, int min, int max);
} Rng;

/// @brief Returns an RNG implementation based on the @c rand function from @c
/// \<stdlib.h\> (and seeds it with @c srand).
///
/// @warning It is probably not thread-safe though... Then again, it's not like
/// we are using threads in the C program.
Rng init_stdlib_rng(void);

/// A constant for #fnv32_hash.
#define FNV32_INITIAL_STATE ((uint32_t)2166136261)
/// A constant for #fnv32_hash.
#define FNV32_PRIME ((uint32_t)16777619)

/// @brief Computes the 32-bit FNV-1a hash of a given byte sequence.
///
/// This is a fast and simple hash function that shouldn't be used for anything
/// serious. The invocations may be chained together to compute the hash of,
/// say, a struct or an array by passing the returned value to the @c state
/// argument of the next call, however, the first call must use @c
/// #FNV32_INITIAL_STATE as the value of @c state.
///
/// @see <https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function>
inline uint32_t fnv32_hash(uint32_t state, const void* buf, size_t len) {
  const uint8_t* ptr = (const uint8_t*)buf;
  const uint8_t* end = ptr + len;
  for (; ptr != end; ptr++) state = (state ^ *ptr) * FNV32_PRIME;
  return state;
}

#ifdef __cplusplus
}
#endif
