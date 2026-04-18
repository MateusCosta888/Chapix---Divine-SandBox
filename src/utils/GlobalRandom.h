#pragma once

#include <cstdint>
#include <random>
#include <mutex>

// ============================================================================
// GLOBAL RANDOM - Thread-safe RNG accessor
// ============================================================================
// This provides a centralized, seeded random number generator that replaces
// the global rand() function for reproducibility and thread safety.
// ============================================================================

class GlobalRandom {
public:
  // Get the single instance (singleton pattern)
  static GlobalRandom& Get() {
    static GlobalRandom instance;
    return instance;
  }

  // Seed the RNG (call once at startup)
  void Seed(uint32_t seed) {
    std::lock_guard<std::mutex> lock(mutex_);
    engine_ = std::mt19937(seed);
  }

  // Returns a float in [0.0, 1.0)
  float Float() {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::uniform_real_distribution<float>(0.0f, 1.0f)(engine_);
  }

  // Returns an int in [min, max]
  int Int(int min, int max) {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::uniform_int_distribution<int>(min, max)(engine_);
  }

  // Returns a float in [min, max)
  float FloatRange(float min, float max) {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::uniform_real_distribution<float>(min, max)(engine_);
  }

  // Returns an int with probability (percentage out of 100)
  bool Chance(int percentage) {
    return Int(1, 100) <= percentage;
  }

private:
  GlobalRandom() : engine_(std::random_device{}()) {}
  GlobalRandom(const GlobalRandom&) = delete;
  GlobalRandom& operator=(const GlobalRandom&) = delete;

  std::mt19937 engine_;
  std::mutex mutex_;
};

// Convenience macros (use Get().Int/Get().Float/Get().Chance)
// These provide a cleaner syntax while maintaining the benefits
#define GRandom GlobalRandom::Get()
