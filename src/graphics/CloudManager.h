#pragma once

#include "raylib.h"
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>

struct Cloud {
  float worldX;         // World-space X position (pixels)
  float worldY;         // World-space Y position (pixels)
  float speed;          // Horizontal speed in world pixels per second
  float alpha;          // Transparency (0.0 - 1.0)
  float scale;          // Scale factor
  int textureIndex;     // 0-5 for cloud1-6
};

class CloudManager {
public:
  static const int NUM_TEXTURES = 6;
  static const int MAX_CLOUDS = 8; // Limitado para evitar poluição no mapa

  void Load() {
    textures[0] = LoadTexture("assets/Clouds/cloud1.png");
    textures[1] = LoadTexture("assets/Clouds/cloud2.png");
    textures[2] = LoadTexture("assets/Clouds/cloud3.png");
    textures[3] = LoadTexture("assets/Clouds/cloud4.png");
    textures[4] = LoadTexture("assets/Clouds/cloud5.png");
    textures[5] = LoadTexture("assets/Clouds/cloud6.png");
  }

  void Unload() {
    for (int i = 0; i < NUM_TEXTURES; i++) {
      if (textures[i].id > 0) UnloadTexture(textures[i]);
    }
  }

  void Init(float mapWidth, float mapHeight) {
    worldWidth = mapWidth;
    worldHeight = mapHeight;
    clouds.clear();
    
    // Começa com céu limpo e espera apenas 5 segundos para a PRIMEIRA vez 
    // (pra você conseguir testar rápido agora). Depois será 2 a 4 minutos.
    isCloudy = false;
    weatherTimer = 5.0f; 
  }

  void Update(float dt, const Camera2D& camera) {
    weatherTimer -= dt;

    if (weatherTimer <= 0.0f) {
      if (isCloudy) {
        // Acaba a passagem de nuvens, tempo limpo de 2 a 4 minutos (120 - 240 segundos)
        isCloudy = false;
        weatherTimer = 120.0f + (float)(rand() % 120); 
      } else {
        // Começa a transição de nuvens durante 30 a 60 segundos
        isCloudy = true;
        weatherTimer = 30.0f + (float)(rand() % 30);
        spawnTimer = 0.0f;
      }
    }

    // Move existing clouds left to right
    for (auto &c : clouds) {
      c.worldX += c.speed * dt;
    }

    float screenWWorld = GetScreenWidth() / camera.zoom;
    float screenHWorld = GetScreenHeight() / camera.zoom;
    
    // Limites de remoção BEM distantes da borda da câmera
    // Assim o jogador não vê a nuvem sumindo do nada
    float rightBound = camera.target.x + (screenWWorld * 0.5f) + 2000.0f;
    float leftBound = camera.target.x - (screenWWorld * 0.5f) - 1500.0f;

    // Remove clouds that went past the right camera bound
    clouds.erase(
      std::remove_if(clouds.begin(), clouds.end(),
        [rightBound](const Cloud &c) {
          return c.worldX > rightBound;
        }),
      clouds.end()
    );

    // Spawn new clouds only during the cloudy phase
    if (isCloudy) {
      spawnTimer += dt;
      if (spawnTimer >= spawnInterval) {
        int batch = 1 + rand() % 2; // Spawn 1-2 clouds at a time
        for (int i = 0; i < batch && (int)clouds.size() < MAX_CLOUDS; i++) {
          SpawnCloudNearCamera(leftBound, camera.target.y, screenHWorld);
        }
        spawnTimer = 0.0f;
        // Tempo curto entre aparições durante a fase de nuvens
        spawnInterval = 3.0f + (float)(rand() % 5);
      }
    }
  }

  void Draw() {
    for (const auto &c : clouds) {
      Texture2D &tex = textures[c.textureIndex];
      if (tex.id <= 0) continue;

      float w = tex.width * c.scale;
      float h = tex.height * c.scale;

      Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
      Rectangle dst = {c.worldX - w / 2.0f, c.worldY - h / 2.0f, w, h};

      DrawTexturePro(tex, src, dst, {0, 0}, 0.0f,
                     ColorAlpha(WHITE, c.alpha));
    }
  }

private:
  Texture2D textures[NUM_TEXTURES] = {};
  std::vector<Cloud> clouds;
  float spawnTimer = 0.0f;
  float spawnInterval = 3.0f;
  float worldWidth = 3000.0f;
  float worldHeight = 3000.0f;
  
  bool isCloudy = false;
  float weatherTimer = 0.0f;

  void SpawnCloudNearCamera(float leftBound, float targetY, float screenHWorld) {
    Cloud c;
    c.textureIndex = rand() % NUM_TEXTURES;
    c.scale = 2.0f + (float)(rand() % 30) / 10.0f; // 2.0 - 5.0 (tamanho reduzido a pedido do user)
    c.speed = 10.0f + (float)(rand() % 30);         // 10 - 40 px/s
    c.alpha = 0.5f + (float)(rand() % 30) / 100.0f; // 0.5 - 0.8 

    // Aparece BEM distante à esquerda da câmera, invisível ao jogador
    c.worldX = leftBound - (float)(rand() % 500);

    // Y espalhado perto do que o jogador está olhando, pra garantir que passe por ele
    float rangeY = screenHWorld + 1000.0f; 
    c.worldY = targetY - (rangeY * 0.5f) + (float)(rand() % (int)rangeY);

    clouds.push_back(c);
  }
};
