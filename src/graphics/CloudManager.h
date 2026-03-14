#pragma once

#include "raylib.h"
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>

struct Cloud {
  Vector2 position;      // World coordinates (pixels)
  float speed;           // Horizontal speed in pixels per second
  float parallaxFactor;  // Parallax strength (0.0 = static, 1.0 = same as world)
  float alpha;           // Transparency
  float scale;           // Base scale (multiplied by zoom)
  int textureIndex;      // 0-5 for cloud1-6
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

    // Spawn initial clouds spread across a default width
    float initialWidth = (float)GetScreenWidth();
    for (int i = 0; i < 3; i++) {
      SpawnCloud(true, initialWidth);
    }
  }

  void Unload() {
    for (int i = 0; i < NUM_TEXTURES; i++) {
      if (textures[i].id > 0) UnloadTexture(textures[i]);
    }
  }

  void Update(float worldWidth, float dt) {
    spawnTimer += dt;

    // Move existing clouds (world coordinates)
    for (auto &c : clouds) {
      c.position.x += c.speed * dt;

      // Wrap around world boundaries
      float margin = 200.0f;
      if (c.position.x > worldWidth + margin) {
        c.position.x = -margin;
      }
    }

    // Spawn new clouds periodically
    if (spawnTimer >= spawnInterval && (int)clouds.size() < MAX_CLOUDS) {
      SpawnCloud(true, worldWidth);
      spawnTimer = 0.0f;
      // Randomize next spawn interval (8-15 seconds)
      spawnInterval = 8.0f + (float)(rand() % 8);
    }
  }

  void Draw(const Camera2D &camera) {
    for (auto &c : clouds) {
      Texture2D &tex = textures[c.textureIndex];
      if (tex.id <= 0)
        continue;

      // Transform world position into screen space with parallax + zoom
      float screenX = (c.position.x - camera.target.x) * c.parallaxFactor *
                      camera.zoom + camera.offset.x;
      float screenY = (c.position.y - camera.target.y) * c.parallaxFactor *
                      camera.zoom + camera.offset.y;

      float totalScale = c.scale * camera.zoom;
      float w = tex.width * totalScale;
      float h = tex.height * totalScale;

      Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
      Rectangle dst = {screenX, screenY, w, h};

      DrawTexturePro(tex, src, dst, {0, 0}, 0.0f,
                     ColorAlpha(WHITE, c.alpha));
    }
  }

private:
  Texture2D textures[NUM_TEXTURES] = {};
  std::vector<Cloud> clouds;
  float spawnTimer = 0.0f;
  float spawnInterval = 5.0f;

  void SpawnCloud(bool randomX, float worldWidth) {
    Cloud c;
    float sh = (float)GetScreenHeight();

    c.textureIndex = rand() % NUM_TEXTURES;
    c.scale = 0.4f + (float)(rand() % 80) / 100.0f; // 0.4 - 1.2
    c.speed = 8.0f + (float)(rand() % 24);          // 8 - 32 px/s
    c.parallaxFactor = 0.3f + (float)(rand() % 60) / 100.0f; // 0.3 - 0.9
    c.alpha = 0.3f + (float)(rand() % 50) / 100.0f; // 0.3 - 0.8

    c.position.y = (float)(rand() % (int)(sh * 0.35f)); // Top 35% of screen

    if (randomX) {
      c.position.x = (float)(rand() % (int)worldWidth);
    } else {
      c.position.x = -200.0f; // Start off-screen left
    }

    clouds.push_back(c);
  }
};
