#pragma once

#include "utils.h"
#include <random>

/// @brief An implementation of #Rng for C++ using the standard functions from
/// @c \<random\>.
///
/// Here I abuse the fact that if class B inherits from class A, given an
/// instance of B a pointer of the type A may be created to it and using the
/// members of the base class A through said pointer afterwards is safe and
/// perfectly fine. This works because the fields of descendant classes are
/// laid out in memory after the fields of the base class, so if we have a
/// pointer of the base type to an object that actually extends that base
/// class, we are going to access just the leading chunk of memory.
///
/// Strictly speaking implementing this struct like this wasn't required, but
/// inheriting from a C struct is nonetheless a cool hack. On the other hand,
/// now #BetterRng can be passed anywhere #Rng can.
struct BetterRng : Rng {
public:
  std::default_random_engine rng_engine;
  /// Seeds the #rng_engine using @c std::random_device.
  BetterRng();

private:
  /// See #Rng::random_range.
  static int random_range_impl(Rng* rng, int min, int max);
};
