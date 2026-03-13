#pragma once

#include "raylib.h"
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>

struct Cloud {
  Vector2 position;
  float speed;
  float scale;
  int textureIndex; // 0-5 for cloud1-6
};

class CloudManager {
public:
  static const int MAX_CLOUDS = 6;
  static const int NUM_TEXTURES = 6;

  void Load() {
    textures[0] = LoadTexture("assets/Clouds/cloud1.png");
    textures[1] = LoadTexture("assets/Clouds/cloud2.png");
    textures[2] = LoadTexture("assets/Clouds/cloud3.png");
    textures[3] = LoadTexture("assets/Clouds/cloud4.png");
    textures[4] = LoadTexture("assets/Clouds/cloud5.png");
    textures[5] = LoadTexture("assets/Clouds/cloud6.png");

    // Spawn initial clouds spread across screen
    for (int i = 0; i < 3; i++) {
      SpawnCloud(true);
    }
  }

  void Unload() {
    for (int i = 0; i < NUM_TEXTURES; i++) {
      if (textures[i].id > 0) UnloadTexture(textures[i]);
    }
  }

  void Update(float dt) {
    spawnTimer += dt;

    // Move existing clouds
    for (auto &c : clouds) {
      c.position.x += c.speed * dt;
    }

    // Remove clouds that left the screen
    float sw = (float)GetScreenWidth();
    clouds.erase(
        std::remove_if(clouds.begin(), clouds.end(),
                        [sw](const Cloud &c) { return c.position.x > sw + 200; }),
        clouds.end());

    // Spawn new clouds periodically
    if (spawnTimer >= spawnInterval && (int)clouds.size() < MAX_CLOUDS) {
      SpawnCloud(false);
      spawnTimer = 0.0f;
      // Randomize next spawn interval (8-15 seconds)
      spawnInterval = 8.0f + (float)(rand() % 8);
    }
  }

  void Draw() {
    for (auto &c : clouds) {
      Texture2D &tex = textures[c.textureIndex];
      if (tex.id <= 0) continue;

      float w = tex.width * c.scale;
      float h = tex.height * c.scale;
      Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
      Rectangle dst = {c.position.x, c.position.y, w, h};

      DrawTexturePro(tex, src, dst, {0, 0}, 0.0f, ColorAlpha(WHITE, 0.35f));
    }
  }

private:
  Texture2D textures[NUM_TEXTURES] = {};
  std::vector<Cloud> clouds;
  float spawnTimer = 0.0f;
  float spawnInterval = 5.0f;

  void SpawnCloud(bool randomX) {
    Cloud c;
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    c.textureIndex = rand() % NUM_TEXTURES;
    c.scale = 0.5f + (float)(rand() % 50) / 100.0f; // 0.5 - 1.0
    c.speed = 10.0f + (float)(rand() % 20);           // 10 - 30 px/s
    c.position.y = (float)(rand() % (int)(sh * 0.35f)); // Top 35% of screen

    if (randomX) {
      c.position.x = (float)(rand() % (int)sw); // Spread across screen
    } else {
      c.position.x = -200.0f; // Start off-screen left
    }

    clouds.push_back(c);
  }
};
