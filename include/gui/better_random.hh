#pragma once

#include "utils.h"
#include <random>

struct BetterRng : Rng {
public:
  std::default_random_engine rng_engine;
  BetterRng();

private:
  static int random_range_impl(Rng* rng, int min, int max);
};

int better_random_range(int min, int max);
