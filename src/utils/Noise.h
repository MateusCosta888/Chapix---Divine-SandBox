#pragma once

#include <cstdint>
#include <vector>


class Noise {
public:
  Noise(uint32_t seed);

  // Get noise value at (x, y). Returns value roughly between -1.0 and 1.0
  float GetNoise(float x, float y) const;

  // Get fractal noise (multiple octaves)
  float GetFractal(float x, float y, int octaves, float frequency,
                   float persistence) const;

private:
  std::vector<int> p; // Permutation vector

  float Fade(float t) const;
  float Lerp(float t, float a, float b) const;
  float Grad(int hash, float x, float y) const;
};
