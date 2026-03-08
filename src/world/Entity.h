#pragma once
#include "../utils/JsonHelpers.h"
#include "raylib.h"

// Forward declaration for citizen data
struct Citizen;

enum class EntityType {
  HumanUnarmed, // Replaces Generic Human
  HumanArmed,   // New Armed Variant
  HumanWoman,   // Female citizen
  Boar,         // New Enemy
  Cow,
  Chicken,
  Sheep,
  Bull,
  Chicken2,
  Lamb,
  Pig,
  Turkey,
  Slime
};

enum class EntityState { Idle, Walking, Attack, Run, Hurt, Die, Swim, Block };

struct Entity {
  int id;
  EntityType type;
  Vector2
      position; // World Grid Coordinates (e.g., 50.5, 50.5 for center of tile)
  EntityState state;
  int facingDirection; // -1 Left, 1 Right, 0 Down, 2 Up
  float speed;
  float health;

  // Animation
  float animTime;
  int currentFrame;

  // Target for movement
  bool hasTarget;
  Vector2 targetPos;

  // === SIMULATION LINK ===
  // For intelligent entities (Humans), this links to their Citizen data in
  // SimulationManager
  int citizenID = -1; // -1 means not a citizen (animal, etc.)

  // === SURVIVAL SYSTEMS ===
  float bodyTemperature = 37.0f; // Normal = 37, below 30 = cold damage

  // === ANIMAL REPRODUCTION ===
  float reproductionTimer = 0.0f;
  float reproductionCooldown = 60.0f; // Seconds between births

  // Helper to check if this entity is an intelligent creature
  bool IsIntelligent() const {
    return type == EntityType::HumanUnarmed || type == EntityType::HumanArmed ||
           type == EntityType::HumanWoman;
  }

  void TakeDamage(float amount) {
    health -= amount;
    if (health < 0)
      health = 0;

    // Apply Hurt state visual if not dying
    if (health > 0) {
      if (state !=
          EntityState::Attack) { // Don't interrupt attack for slight grazes
        state = EntityState::Hurt;
        animTime = 0.0f;
        currentFrame = 0;
      }
    } else {
      state = EntityState::Die;
      animTime = 0.0f;
      currentFrame = 0;
    }
  }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    Entity, id, type, position, state, facingDirection, speed, health, animTime,
    currentFrame, hasTarget, targetPos, citizenID, bodyTemperature,
    reproductionTimer, reproductionCooldown)
