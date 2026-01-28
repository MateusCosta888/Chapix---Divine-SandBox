#pragma once

#include <cstdint>
#include <random>


class Random {
public:
  explicit Random(uint32_t seed) : engine(seed) {}
  // Returns a float in [0.0, 1.0)
  float Float() {
    return std::uniform_real_distribution<float>(0.0f, 1.0f)(engine);
  }
  // Returns an int in [min, max]
  int Int(int min, int max) {
    return std::uniform_int_distribution<int>(min, max)(engine);
  }

private:
  std::mt19937 engine;
};
