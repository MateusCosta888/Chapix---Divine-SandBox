#include "AudioManager.h"
#include <cstdlib>
#include <ctime>

void AudioManager::Load() {
  if (isLoaded)
    return;

  // Load ambient tracks
  const char *ambientFiles[] = {
      "assets/SondTrack/Ambient/Pixel 1.mp3",
      "assets/SondTrack/Ambient/Pixel 2.mp3",
      "assets/SondTrack/Ambient/Pixel 3.mp3",
      "assets/SondTrack/Ambient/Pixel 4.mp3",
      "assets/SondTrack/Ambient/Pixel 6.mp3",
      "assets/SondTrack/Ambient/Pixel 8.mp3",
      "assets/SondTrack/Ambient/Pixel 9.mp3",
      "assets/SondTrack/Ambient/Pixel 10.mp3",
      "assets/SondTrack/Ambient/Pixel 11.mp3",
      "assets/SondTrack/Ambient/Pixel 12.mp3",
  };

  for (const char *file : ambientFiles) {
    Music m = LoadMusicStream(file);
    if (m.frameCount > 0) {
      m.looping = false; // We handle looping manually to shuffle
      ambientTracks.push_back(m);
      TraceLog(LOG_INFO, "AUDIO: Loaded ambient track: %s", file);
    } else {
      TraceLog(LOG_WARNING, "AUDIO: Failed to load: %s", file);
    }
  }

  // Load space track
  spaceTrack = LoadMusicStream("assets/SondTrack/Menuspace/02 Space Riddle.mp3");
  if (spaceTrack.frameCount > 0) {
    spaceTrack.looping = true; // Space track loops
    spaceTrackLoaded = true;
    TraceLog(LOG_INFO, "AUDIO: Loaded space track");
  } else {
    TraceLog(LOG_WARNING, "AUDIO: Failed to load space track");
  }

  // Seed random for shuffle
  srand((unsigned int)time(nullptr));

  isLoaded = true;

  // Start playing ambient by default
  SetMusicVolume(musicVolume);
  PlayRandomAmbient();
}

void AudioManager::Unload() {
  if (!isLoaded)
    return;

  StopCurrentMusic();

  for (auto &m : ambientTracks) {
    UnloadMusicStream(m);
  }
  ambientTracks.clear();

  if (spaceTrackLoaded) {
    UnloadMusicStream(spaceTrack);
    spaceTrackLoaded = false;
  }

  isLoaded = false;
  TraceLog(LOG_INFO, "AUDIO: All music unloaded");
}

void AudioManager::Update() {
  if (!isLoaded)
    return;

  if (currentMode == MusicMode::AMBIENT) {
    // Update current ambient track
    if (currentAmbientIndex >= 0 &&
        currentAmbientIndex < (int)ambientTracks.size()) {
      UpdateMusicStream(ambientTracks[currentAmbientIndex]);

      // Check if track finished — play next random
      float timePlayed =
          GetMusicTimePlayed(ambientTracks[currentAmbientIndex]);
      float timeLength =
          GetMusicTimeLength(ambientTracks[currentAmbientIndex]);
      if (timePlayed >= timeLength - 0.1f) {
        PlayRandomAmbient();
      }
    } else if (!ambientTracks.empty()) {
      // No track playing, start one
      PlayRandomAmbient();
    }
  } else if (currentMode == MusicMode::SPACE) {
    if (spaceTrackLoaded) {
      UpdateMusicStream(spaceTrack);
    }
  }
}

void AudioManager::SetMusicMode(MusicMode mode) {
  if (mode == currentMode)
    return;

  StopCurrentMusic();
  currentMode = mode;

  if (mode == MusicMode::AMBIENT) {
    PlayRandomAmbient();
  } else if (mode == MusicMode::SPACE && spaceTrackLoaded) {
    SetMusicVolume(musicVolume);
    PlayMusicStream(spaceTrack);
    isMusicPlaying = true;
    TraceLog(LOG_INFO, "AUDIO: Playing space track");
  }
}

void AudioManager::SetMusicVolume(float volume) {
  musicVolume = volume;
  if (musicVolume < 0.0f)
    musicVolume = 0.0f;
  if (musicVolume > 1.0f)
    musicVolume = 1.0f;

  // Apply to all tracks
  for (auto &m : ambientTracks) {
    ::SetMusicVolume(m, musicVolume);
  }
  if (spaceTrackLoaded) {
    ::SetMusicVolume(spaceTrack, musicVolume);
  }
}

void AudioManager::PlayRandomAmbient() {
  if (ambientTracks.empty())
    return;

  StopCurrentMusic();

  // Pick random track (different from current if possible)
  int newIndex;
  if (ambientTracks.size() > 1) {
    do {
      newIndex = rand() % (int)ambientTracks.size();
    } while (newIndex == currentAmbientIndex);
  } else {
    newIndex = 0;
  }

  currentAmbientIndex = newIndex;
  SetMusicVolume(musicVolume);
  PlayMusicStream(ambientTracks[currentAmbientIndex]);
  isMusicPlaying = true;

  TraceLog(LOG_INFO, "AUDIO: Playing ambient track %d", currentAmbientIndex);
}

void AudioManager::StopCurrentMusic() {
  if (!isMusicPlaying)
    return;

  if (currentMode == MusicMode::AMBIENT && currentAmbientIndex >= 0 &&
      currentAmbientIndex < (int)ambientTracks.size()) {
    StopMusicStream(ambientTracks[currentAmbientIndex]);
  } else if (currentMode == MusicMode::SPACE && spaceTrackLoaded) {
    StopMusicStream(spaceTrack);
  }

  isMusicPlaying = false;
}
