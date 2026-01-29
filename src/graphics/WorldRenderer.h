#pragma once
#include "../world/World.h"
#include "raylib.h"

class WorldRenderer {
public:
  WorldRenderer(World &world);
  void Draw();

private:
  void DrawEntities();
  World &world;
};
