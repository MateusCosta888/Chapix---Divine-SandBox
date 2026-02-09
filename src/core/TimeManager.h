#pragma once
#include "raylib.h"

// Global time control for the simulation
// Usage: TimeManager::Get().GetDeltaTime() for simulation
// Use raw GetFrameTime() for UI and camera (always responsive)
class TimeManager {
public:
  static TimeManager &Get() {
    static TimeManager instance;
    return instance;
  }

  // Get scaled delta time (0 when paused)
  float GetDeltaTime() const {
    if (isPaused)
      return 0.0f;
    return GetFrameTime() * timeScale;
  }

  // Get raw delta time (for UI/animations that should always run)
  float GetRawDeltaTime() const { return GetFrameTime(); }

  // Pause controls
  bool IsPaused() const { return isPaused; }
  void SetPaused(bool paused) { isPaused = paused; }
  void TogglePause() { isPaused = !isPaused; }

  // Speed controls (1.0 to 5.0)
  float GetTimeScale() const { return timeScale; }
  void SetTimeScale(float scale) {
    if (scale < 1.0f)
      scale = 1.0f;
    if (scale > 5.0f)
      scale = 5.0f;
    timeScale = scale;
  }

  // Cycle through speeds: 1x -> 2x -> 3x -> 4x -> 5x -> 1x
  void CycleTimeScale() {
    if (timeScale >= 5.0f)
      timeScale = 1.0f;
    else
      timeScale += 1.0f;
  }

private:
  TimeManager() : isPaused(false), timeScale(1.0f) {}
  TimeManager(const TimeManager &) = delete;
  TimeManager &operator=(const TimeManager &) = delete;

  bool isPaused;
  float timeScale;
};
