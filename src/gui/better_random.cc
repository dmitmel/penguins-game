#include "gui/better_random.hh"
#include <memory>
#include <random>

BetterRng::BetterRng() {
  // std::random_device has really large internal state, something like 5
  // kilobytes (on GCC/Linux), therefore I think its better to allocate it on
  // the heap.
  std::unique_ptr<std::random_device> rng_dev(new std::random_device());
  this->rng_engine.seed((*rng_dev)());
  this->random_range = &random_range_impl;
}

int BetterRng::random_range_impl(Rng* rng, int min, int max) {
  auto self = (BetterRng*)rng;
  std::uniform_int_distribution<int> distribution(min, max);
  return distribution(self->rng_engine);
}
