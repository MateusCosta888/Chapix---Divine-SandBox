#pragma once
#include "raylib.h"
#include <string>
#include <vector>

enum class MusicMode { AMBIENT, SPACE };

class AudioManager {
public:
  static AudioManager &Get() {
    static AudioManager instance;
    return instance;
  }

  void Load();
  void Unload();
  void Update(); // Call every frame to keep music streaming

  // Switch between ambient (game/menu day) and space (menu night)
  void SetMusicMode(MusicMode mode);
  MusicMode GetMusicMode() const { return currentMode; }

  // Volume control (0.0 - 1.0)
  void SetMusicVolume(float volume);
  float GetMusicVolume() const { return musicVolume; }

private:
  AudioManager() = default;
  ~AudioManager() = default;
  AudioManager(const AudioManager &) = delete;
  AudioManager &operator=(const AudioManager &) = delete;

  void PlayRandomAmbient();
  void StopCurrentMusic();

  // Ambient tracks
  std::vector<Music> ambientTracks;
  int currentAmbientIndex = -1;

  // Space track
  Music spaceTrack = {0};
  bool spaceTrackLoaded = false;

  MusicMode currentMode = MusicMode::AMBIENT;
  float musicVolume = 0.7f;
  bool isLoaded = false;
  bool isMusicPlaying = false;
};
