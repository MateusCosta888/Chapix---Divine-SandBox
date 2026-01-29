#pragma once
#include "raylib.h"

enum class EntityType { Human };

enum class EntityState { Idle, Walking };

struct Entity {
  int id;
  EntityType type;
  Vector2
      position; // World Grid Coordinates (e.g., 50.5, 50.5 for center of tile)
  EntityState state;
  int facingDirection; // -1 Left, 1 Right
  float speed;
  float health;

  // Animation
  float animTime;
  int currentFrame;

  // Target for movement
  bool hasTarget;
  Vector2 targetPos;
};
