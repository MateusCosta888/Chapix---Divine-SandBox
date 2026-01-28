#include "Noise.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>


Noise::Noise(uint32_t seed) {
  p.resize(256);
  std::iota(p.begin(), p.end(), 0);

  std::mt19937 engine(seed);
  std::shuffle(p.begin(), p.end(), engine);

  // Duplicate for wrapping
  p.insert(p.end(), p.begin(), p.end());
}

float Noise::GetNoise(float x, float y) const {
  // Find unit square
  int X = (int)std::floor(x) & 255;
  int Y = (int)std::floor(y) & 255;

  // Relative coordinates
  x -= std::floor(x);
  y -= std::floor(y);

  // Fade curves
  float u = Fade(x);
  float v = Fade(y);

  // Hash coordinates
  int A = p[X] + Y;
  int AA = p[A];
  int AB = p[A + 1];
  int B = p[X + 1] + Y;
  int BA = p[B];
  int BB = p[B + 1];

  // Blend results
  return Lerp(v, Lerp(u, Grad(p[AA], x, y), Grad(p[BA], x - 1, y)),
              Lerp(u, Grad(p[AB], x, y - 1), Grad(p[BB], x - 1, y - 1)));
}

float Noise::GetFractal(float x, float y, int octaves, float frequency,
                        float persistence) const {
  float total = 0.0f;
  float amplitude = 1.0f;
  float maxValue = 0.0f; // Used for normalizing result to 0.0 - 1.0

  for (int i = 0; i < octaves; i++) {
    total += GetNoise(x * frequency, y * frequency) * amplitude;

    maxValue += amplitude;

    amplitude *= persistence;
    frequency *= 2.0f;
  }

  // Normalize to roughly 0.0 - 1.0
  // GetNoise returns approx -1 to 1, so total is in range [-maxValue, maxValue]
  // (total / maxValue) is -1 to 1
  // ((total / maxValue) + 1) / 2 is 0 to 1

  return ((total / maxValue) + 1.0f) / 2.0f;
}

float Noise::Fade(float t) const { return t * t * t * (t * (t * 6 - 15) + 10); }

float Noise::Lerp(float t, float a, float b) const { return a + t * (b - a); }

float Noise::Grad(int hash, float x, float y) const {
  int h = hash & 15;
  float u = h < 8 ? x : y;
  float v = h < 4 ? y : h == 12 || h == 14 ? x : 0;
  return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}
