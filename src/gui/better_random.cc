#include "random.h"
#include <random>

static std::default_random_engine& get_random_engine() {
  static thread_local std::default_random_engine rng{ std::random_device()() };
  return rng;
}

extern "C" void random_init(void) {
  // Ensure that the RNG has been initialized.
  get_random_engine();
}

extern "C" int random_range(int min, int max) {
  std::uniform_int_distribution<int> distribution(min, max);
  return distribution(get_random_engine());
}
