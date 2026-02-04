#pragma once
#include "../world/World.h"
#include "raylib.h"

#include <vector>

struct RenderItem {
  Texture2D texture;
  Rectangle src;
  Rectangle dest;
  Vector2 origin;
  Color tint;
  float sortY;
};

class WorldRenderer {
public:
  WorldRenderer(World &world);
  void Draw();

private:
  void DrawEntities();

  // Water Effects
  void DrawWaterEffects(int tileX, int tileY, TileType type, int screenX,
                        int screenY, int tileSize, float time);
  void DrawWaterWaves(int screenX, int screenY, int tileSize, float time,
                      TileType type);
  void DrawWaterSparkles(int screenX, int screenY, int tileSize, float time,
                         unsigned int seed);
  void DrawWaterFoam(int tileX, int tileY, int screenX, int screenY,
                     int tileSize, float time);
  bool IsWaterTile(TileType type) const;

  World &world;
};
