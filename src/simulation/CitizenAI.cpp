#include "../world/Entity.h"
#include "../world/Tile.h"
#include "../world/World.h"
#include "SimulationManager.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include "../utils/GlobalRandom.h"
#include <vector>

// ============================================================================
// UPDATE SUBSYSTEMS - CITIZEN AI
// ============================================================================

// Helper: Try to move towards target, return true if reached
static bool TryMoveToTarget(Entity *entity, float deltaTime) {
  if (!entity || !entity->hasTarget) return false;

  float dist = std::hypot(entity->position.x - entity->targetPos.x,
                          entity->position.y - entity->targetPos.y);

  if (dist < 0.5f) {
    entity->hasTarget = false;
    return true;
  }

  // Simple movement (speed based on stats)
  float speed = 2.0f + entity->speed * 0.5f;
  float moveDist = speed * deltaTime;

  if (moveDist >= dist) {
    entity->position = entity->targetPos;
    entity->hasTarget = false;
    return true;
  }

  Vector2 dir = Vector2Normalize(
      Vector2Subtract(entity->targetPos, entity->position));
  
  // Add tiny jitter to prevent perfect stacking
  dir.x += (GRandom.Float() - 0.5f) * 0.15f;
  dir.y += (GRandom.Float() - 0.5f) * 0.15f;
  dir = Vector2Normalize(dir);

  entity->position = Vector2Add(entity->position, Vector2Scale(dir, moveDist));
  return false;
}

// Helper: Find a random walkable spot nearby
static Vector2 FindRandomWalkableSpot(World &world, int cx, int cy, int radius) {
  for (int attempt = 0; attempt < 5; attempt++) {
    int rx = cx + GRandom.Int(-radius, radius);
    int ry = cy + GRandom.Int(-radius, radius);

    if (rx >= 0 && ry >= 0 && rx < world.GetWidth() && ry < world.GetHeight()) {
      if (world.IsWalkable(rx, ry)) {
        return {(float)rx + 0.5f, (float)ry + 0.5f};
      }
    }
  }
  return {(float)cx + 0.5f, (float)cy + 0.5f};
}

void SimulationManager::UpdateCitizens(World &world, float deltaTime) {

  // Settler check timer (don't check every frame)
  static float settlerCheckTimer = 0.0f;
  settlerCheckTimer += deltaTime;
  bool doSettlerCheck = settlerCheckTimer >= 5.0f; // Check every 5 seconds
  if (doSettlerCheck)
    settlerCheckTimer = 0.0f;

  // Build check timer - check more frequently now (every 2 seconds)
  static float buildCheckTimer = 0.0f;
  buildCheckTimer += deltaTime;
  bool doBuildCheck = buildCheckTimer >= 2.0f;
  if (doBuildCheck)
    buildCheckTimer = 0.0f;

  for (auto &pair : citizens) {
    Citizen &c = pair.second;
    if (!c.isAlive)
      continue;

    // === FIND ENTITY ===
    Entity *myEntity = world.GetEntityByCitizenID(c.id);
    if (!myEntity)
      continue;

    // === STUCK DETECTION ===
    if (myEntity->state == EntityState::Walking || myEntity->state == EntityState::Run) {
        float distMoved = std::hypot(myEntity->position.x - c.lastX, myEntity->position.y - c.lastY);
        if (distMoved < 0.01f) {
            c.stuckTimer += deltaTime;
            if (c.stuckTimer >= 3.0f) {
                c.workState = Citizen::WorkState::Idle;
                myEntity->state = EntityState::Idle;
                myEntity->hasTarget = false;
                c.stuckTimer = 0.0f;
                // Small nudge
                myEntity->position.x += (GRandom.Float() - 0.5f) * 0.2f;
                myEntity->position.y += (GRandom.Float() - 0.5f) * 0.2f;
            }
        } else {
            c.stuckTimer = 0.0f;
        }
    } else {
        c.stuckTimer = 0.0f;
    }
    c.lastX = myEntity->position.x;
    c.lastY = myEntity->position.y;

    // === AGE SYSTEM ===
    float ageMultiplier = c.isChild() ? 5.0f : 1.0f;
    float yearProgress = deltaTime * 0.05f * ageMultiplier;
    c.age += yearProgress;

    // === HOSTILE DETECTION (Flee Logic) ===
    if (c.health > 0) {
      float fleeRadius = 8.0f;
      Entity *threat = nullptr;
      float minThreatDist = 999.0f;

      auto nearbyEntities = world.GetEntitiesInRadius(myEntity->position, fleeRadius);
      for (Entity *ae : nearbyEntities) {
        if (ae->type == EntityType::Boar || ae->type == EntityType::Slime || ae->type == EntityType::Dragon) {
          float d = std::hypot(myEntity->position.x - ae->position.x,
                               myEntity->position.y - ae->position.y);
          if (d < minThreatDist) {
            minThreatDist = d;
            threat = ae;
          }
        }
      }

      if (threat) {
        if (c.profession != Profession::Soldier) {
          // Flee logic: find a spot away from threat that is walkable
          Vector2 fleeDir = Vector2Normalize(Vector2Subtract(myEntity->position, threat->position));
          Vector2 bestFleePos = myEntity->position;
          float bestFleeScore = -1.0f;

          for (int i = 0; i < 8; i++) {
              float angle = ((float)i / 8.0f) * 2.0f * PI;
              Vector2 checkDir = {cosf(angle), sinf(angle)};
              // Favor directions away from threat
              float dot = Vector2DotProduct(checkDir, fleeDir);
              if (dot < 0.2f) continue; // Skip directions towards threat

              Vector2 checkPos = Vector2Add(myEntity->position, Vector2Scale(checkDir, 5.0f));
              if (world.IsWalkable((int)checkPos.x, (int)checkPos.y)) {
                  float score = dot;
                  if (score > bestFleeScore) {
                      bestFleeScore = score;
                      bestFleePos = checkPos;
                  }
              }
          }

          myEntity->targetPos = bestFleePos;
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Run;
          c.workState = Citizen::WorkState::Idle;
          continue;
        }
      }
    }

    // === PASSIVE XP (Base growth + Action bonuses) ===
    // Base XP: 1.0f por segundo (constante para progressão mínima)
    // Bônus por ações: +2.0f por tarefa completa, +5.0f por combate
    // XP de caça: +15.0f por vitória
    float xpGain = deltaTime * 1.0f;

    // Personality XP modifier
    float xpMultiplier = 1.0f;
    switch (c.personality) {
      case PersonalityTrait::Hardworking: xpMultiplier = 1.20f; break;  // +20% XP
      case PersonalityTrait::Brave: xpMultiplier = 1.15f; break;        // +15% XP
      case PersonalityTrait::Coward: xpMultiplier = 0.90f; break;       // -10% XP (evita riscos)
      case PersonalityTrait::Lazy: xpMultiplier = 0.70f; break;         // -30% XP (evita trabalho)
      default: break;
    }
    c.experience += xpGain * xpMultiplier;

    // === LEVEL UP CHECK ===
    // XP necessário para level up: base 50 + 25 por nível (linear, não exponencial)
    // Isso permite evolução mais constante e previsível
    while (c.experience >= c.maxExperience) {
      c.experience -= c.maxExperience;
      c.level++;
      // Stats aumentam de forma mais equilibrada
      c.stats.strength += 1.0f;
      c.stats.speed += 0.5f;
      c.stats.endurance += 1.0f;
      c.stats.intelligence += 0.5f;
      // Health aumenta mais significativamente
      c.maxHealth += 10.0f;
      c.health = c.maxHealth;
      // Capacidade de carga aumenta a cada 2 níveis
      if (c.level % 2 == 0) c.maxCarryCapacity += 1;

      // Bonus para cada 5 níveis
      if (c.level % 5 == 0) {
        c.experience += 50.0f;  // Bônus de XP extra para acelerar progression
      }
    }

    // === AGING & DEATH ===
    if (c.age > c.genes.maxAge) {
      c.isAlive = false;
      myEntity->state = EntityState::Die;
      myEntity->currentFrame = 0;
      myEntity->animTime = 0.0f;
      continue;
    }

    // === HAND OF GOD: Skip all AI while being held ===
    if (myEntity->isGrabbed) {
      continue;
    }

    // === HUNGER SYSTEM ===
    // Hunger grows even slower for better early-game survival
    c.hunger += deltaTime * 0.05f; // Buffed from 0.08f
    if (c.hunger > 100.0f) c.hunger = 100.0f;

    // Starvation threshold
    if (c.hunger >= 98.0f) { // Buffed from 95.0f
      c.health -= deltaTime * 0.2f;  // Buffed from 0.5f (much more time to find food)
    }

    // XP bonus for finding food (hunger reduction = survival skill)
    if (c.hunger <= 0.0f) {
      c.experience += deltaTime * 0.5f;  // Bonus XP for well-fed NPCs
    }

    if (c.health <= 0.0f) {
      c.isAlive = false;
      myEntity->state = EntityState::Die;
      myEntity->currentFrame = 0;
      myEntity->animTime = 0.0f;
      continue;
    }

    // === COLD / HYPOTHERMIA SYSTEM ===
    int tileX = static_cast<int>(myEntity->position.x);
    int tileY = static_cast<int>(myEntity->position.y);
    if (tileX >= 0 && tileX < world.GetWidth() && tileY >= 0 && tileY < world.GetHeight()) {
      const Tile &currentTile = world.GetTileConst(tileX, tileY);
      if (currentTile.biome == BiomeType::Snow) {
        float coolingRate = 2.0f;
        if (c.homeID != -1) {
          Building *home = GetBuilding(c.homeID);
          if (home) {
            float distToHome = std::hypot(myEntity->position.x - home->tileX,
                                          myEntity->position.y - home->tileY);
            if (distToHome < 5.0f) coolingRate *= 0.2f;
          }
        }
        c.bodyTemperature -= deltaTime * coolingRate;
      } else {
        if (c.bodyTemperature < 37.0f) {
          c.bodyTemperature += deltaTime * 1.5f;
          if (c.bodyTemperature > 37.0f) c.bodyTemperature = 37.0f;
        }
      }
      if (c.bodyTemperature < 30.0f) {
        c.health -= deltaTime * 3.0f;
      }
      if (c.bodyTemperature < 0.0f) c.bodyTemperature = 0.0f;
    }

    // === STAMINA & HOME SYSTEM ===
    if (c.isResting) {
      float recoveryRate = (c.homeID != -1) ? 15.0f : 20.0f;
      c.energy += deltaTime * recoveryRate;
      c.health += deltaTime * 5.0f;
      if (c.health > c.maxHealth) c.health = c.maxHealth;

      // XP while resting (learning from experience)
      if (c.isAdult()) {
        c.experience += deltaTime * 0.3f;  // Passive XP while resting
      }

      if (c.energy >= 100.0f) {
        c.energy = 100.0f;
        c.isResting = false;
        c.isGoingHome = false;
        c.workState = Citizen::WorkState::Idle;
      } else {
        if (myEntity) {
          myEntity->hasTarget = false;
        }
        continue;
      }
    }

    // === STAMINA DECAY ===
    // Reduced decay rates - NPCs can work longer before resting
    float decayRate = 0.3f;  // Base decay (was 0.5f)
    if (c.workState == Citizen::WorkState::Working) {
      decayRate = 1.0f;  // Working decay (was 2.0f)
    } else if (c.workState == Citizen::WorkState::GoingToWork ||
               c.workState == Citizen::WorkState::ReturningHome) {
      decayRate = 0.5f;  // Moving decay (was 0.8f)
    }
    c.energy -= deltaTime * decayRate;

    // Personality effect on stamina
    switch (c.personality) {
      case PersonalityTrait::Hardworking: decayRate *= 0.8f; break;  // Tires slower
      case PersonalityTrait::Coward: decayRate *= 1.2f; break;      // Tires faster (avoids work)
      default: break;
    }

    // === GO HOME IF TIRED ===
    float restThreshold = (c.homeID != -1) ? 20.0f : 10.0f;
    float chanceToRest = 0.0f;
    if (c.energy < restThreshold) {
      float tiredness = (restThreshold - c.energy) / restThreshold;
      switch (c.personality) {
        case PersonalityTrait::Lazy: chanceToRest = tiredness * 1.5f; break;
        case PersonalityTrait::Coward: chanceToRest = tiredness * 1.2f; break;
        case PersonalityTrait::Hardworking: chanceToRest = tiredness * 0.5f; break;
        case PersonalityTrait::Brave: chanceToRest = tiredness * 0.8f; break;
        case PersonalityTrait::Normal:
        default: chanceToRest = tiredness; break;
      }
      if (chanceToRest > 1.0f) chanceToRest = 1.0f;
    }

    if (chanceToRest > 0.0f && GRandom.Chance(chanceToRest * 100.0f) && !c.isGoingHome && !c.isResting) {
      c.workState = Citizen::WorkState::Idle;
      c.isWorking = false;
      c.stateTimer = 0.0f;
      if (myEntity) myEntity->hasTarget = false;

      if (c.homeID != -1) {
        c.isGoingHome = true;
      } else {
        c.isResting = true;
      }
    }

    // === GO HOME ===
    if (c.isGoingHome) {
      Building *home = GetBuilding(c.homeID);
      if (home) {
        if (c.stateTimer > 60.0f && myEntity) {
          myEntity->position = {(float)home->tileX, (float)home->tileY};
        }

        float dist = std::hypot(myEntity->position.x - home->tileX,
                                myEntity->position.y - home->tileY);

        if (dist < 1.5f) {
          c.isResting = true;
          c.isGoingHome = false;
        } else if (myEntity) {
          myEntity->targetPos = {(float)home->tileX + 0.5f, (float)home->tileY + 0.5f};
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Walking;
          c.stateTimer += deltaTime;
        }
        continue;
      } else {
        c.homeID = -1;
        c.isGoingHome = false;
      }
    }

    // === STUCK DETECTION ===
    if (c.workState != c.lastWorkState) {
      c.stateTimer = 0.0f;
      c.lastWorkState = c.workState;
    } else {
      c.stateTimer += deltaTime;
    }

    // Timeout for GoingToWork
    if (c.workState == Citizen::WorkState::GoingToWork && c.stateTimer > 30.0f) {
      c.workState = Citizen::WorkState::Wandering;
      c.isWorking = false;
      c.stateTimer = 0.0f;
      if (myEntity) {
        myEntity->hasTarget = false;
        myEntity->state = EntityState::Idle;
      }
    }
    // Timeout for Working
    else if (c.workState == Citizen::WorkState::Working && c.stateTimer > 30.0f) {
      c.workState = Citizen::WorkState::Wandering;
      c.isWorking = false;
      c.stateTimer = 0.0f;
      if (myEntity) {
        myEntity->hasTarget = false;
        myEntity->state = EntityState::Idle;
      }
    }
    // Timeout for ReturningHome
    else if (c.workState == Citizen::WorkState::ReturningHome && c.stateTimer > 20.0f) {
      c.workState = Citizen::WorkState::Wandering;
      c.isWorking = false;
      c.stateTimer = 0.0f;
      if (myEntity) {
        myEntity->hasTarget = false;
        myEntity->state = EntityState::Idle;
      }
    }

    // === IDLE TRAINING (Spontaneous skill practice) ===
    // If idle and conditions met, chance to start training
    // This allows NPCs to improve skills even without assigned work
    if (c.workState == Citizen::WorkState::Idle && c.isAdult()) {
      // Check conditions for training: hunger < 60, energy > 40
      if (c.hunger < 60.0f && c.energy > 40.0f) {
        // Base 5% chance per second to start training
        float trainChance = 0.05f;
        // Hardworking trains more often, Lazy trains less
        if (c.personality == PersonalityTrait::Hardworking) trainChance = 0.12f;
        if (c.personality == PersonalityTrait::Lazy) trainChance = 0.02f;

        if (GRandom.Chance(trainChance * 100.0f)) {
          c.workState = Citizen::WorkState::Training;
          c.workTimer = 0.0f;
          c.targetTileX = static_cast<int>(myEntity->position.x);
          c.targetTileY = static_cast<int>(myEntity->position.y);
        }
      }
    }

    // === IDLE STATES ===
    if (c.workState == Citizen::WorkState::IdleSitting ||
        c.workState == Citizen::WorkState::IdleObserving ||
        c.workState == Citizen::WorkState::IdleSocializing) {
      if (myEntity) {
        myEntity->hasTarget = false;
        myEntity->state = EntityState::Idle;
      }
      c.workTimer -= deltaTime;
      if (c.workTimer <= 0.0f) {
        c.workState = Citizen::WorkState::Idle;
        c.stateTimer = 0.0f;
      }
    }

    // === TRAINING STATE ===
    // NPCs practice skills while training - gaining XP and skill points
    if (c.workState == Citizen::WorkState::Training) {
      if (myEntity) {
        myEntity->hasTarget = false;
        myEntity->state = EntityState::Idle;
      }

      // Training for minimum 10 seconds
      c.workTimer += deltaTime;

      // Gain XP and random skill while training
      c.experience += deltaTime * 0.8f;

      // Randomly improve one skill (0-3 chance per second)
      int skillRoll = GRandom.Int(0, 100);
      if (skillRoll < 2) {
        switch (GRandom.Int(0, 4)) {
          case 0: c.skillWoodcutting += 0.2f; break;
          case 1: c.skillFarming += 0.2f; break;
          case 2: c.skillMining += 0.2f; break;
          case 3: c.skillBuilding += 0.2f; break;
          case 4: c.skillCombat += 0.2f; break;
        }
      }

      // Cap skills at 200 with diminishing returns (soft cap logic)
      float allSkills[] = {c.skillWoodcutting, c.skillFarming, c.skillMining, c.skillBuilding, c.skillCombat};
      for (int i = 0; i < 5; i++) {
        if (allSkills[i] > 200.0f) allSkills[i] = 200.0f;
        // Diminishing returns near cap
        if (allSkills[i] > 100.0f) {
          float overCap = allSkills[i] - 100.0f;
          float reduction = overCap * 0.01f;  // Slows down after 100
          if (GRandom.Chance(reduction)) continue;
        }
      }

      // End training after 10+ seconds, return to idle
      if (c.workTimer >= 10.0f) {
        if (GRandom.Chance(50)) {
          c.workState = Citizen::WorkState::Idle;
          c.workTimer = 0.0f;
          c.stateTimer = 0.0f;
        }
      }
      continue;
    }

    // === WANDERING ===
    if (c.workState == Citizen::WorkState::Wandering) {
      if (!myEntity->hasTarget) {
        int rx = GRandom.Int(-4, 4);
        int ry = GRandom.Int(-4, 4);
        int tx = static_cast<int>(myEntity->position.x) + rx;
        int ty = static_cast<int>(myEntity->position.y) + ry;

        if (tx >= 0 && ty >= 0 && tx < world.GetWidth() &&
            ty < world.GetHeight() && world.IsWalkable(tx, ty)) {
          c.targetTileX = tx;
          c.targetTileY = ty;
          myEntity->targetPos = {tx + 0.5f, ty + 0.5f};
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Walking;
        } else {
          c.workState = Citizen::WorkState::Idle;
        }
      }

      float dist = std::hypot(myEntity->position.x - myEntity->targetPos.x,
                              myEntity->position.y - myEntity->targetPos.y);
      if (dist < 0.5f || !myEntity->hasTarget || c.stateTimer > 8.0f) {
        c.workState = Citizen::WorkState::Idle;
        c.stateTimer = 0.0f;
        myEntity->hasTarget = false;
        myEntity->state = EntityState::Idle;
      }
    }

    // === SETTLER AI ===
    if (doSettlerCheck && c.cityID == -1 && c.isAdult()) {
      int tx = static_cast<int>(myEntity->position.x);
      int ty = static_cast<int>(myEntity->position.y);

      // Find nearest city
      int nearestCityID = -1;
      float nearestDist = 999999.0f;

      for (const auto &cityPair : cities) {
        const City &city = cityPair.second;
        if (!city.isAlive || !city.HasCapacity()) continue;

        float dist = std::hypot(city.center.x - tx, city.center.y - ty);
        if (dist < nearestDist) {
          nearestDist = dist;
          nearestCityID = cityPair.first;
        }
      }

      // Join city if nearby
      if (nearestCityID >= 0 && nearestDist <= 10.0f) {
        c.cityID = nearestCityID;
        cities[nearestCityID].citizenIDs.push_back(c.id);
      }
      // Found city if far away
      else if (nearestDist > 15.0f || nearestCityID < 0) {
        float score = ScoreTileForCity(world, tx, ty);
        if (score >= 50.0f) {
          int newCityID = FoundCity(world, c.id, tx, ty);
          c.cityID = newCityID;
        }
      }
    }

    // === HUNTING AI (Improved) ===
    // Hunting chance is now independent of hunger - always has base chance for practice
    // This provides more XP and makes NPC behavior more dynamic
    float baseHuntChance = 0.05f;  // 5% base chance per check (always active)

    // Hunger increases hunting desire (but doesn't block hunting when full)
    if (c.hunger > 30.0f) {
      baseHuntChance += (c.hunger - 30.0f) * 0.02f;
    }

    // Personality modifiers
    float huntChance = baseHuntChance;
    switch (c.personality) {
      case PersonalityTrait::Brave: huntChance *= 1.5f; break;    // More aggressive hunters
      case PersonalityTrait::Hardworking: huntChance *= 1.3f; break;  // Always look for ways to contribute
      case PersonalityTrait::Lazy: huntChance *= 0.5f; break;     // Avoids hunting
      case PersonalityTrait::Coward: huntChance *= 0.7f; break;   // Hesitates to hunt
      default: break;
    }

    // Cap at reasonable max
    if (huntChance > 1.0f) huntChance = 1.0f;

    if (c.isAdult() && c.workState != Citizen::WorkState::Hunting) {
      // Check if hunting is needed (for food or practice)
      bool needsHunting = (c.cityID < 0);  // Settlers always hunt

      if (c.cityID >= 0) {
        City *myCity = GetCity(c.cityID);
        // City hunts when food is low OR to maintain combat readiness
        if (myCity && (myCity->resources.food < 50 || GRandom.Chance(10))) {
          needsHunting = true;
        }
      }

      if (GRandom.Chance(huntChance * 100.0f) && needsHunting) {
        float bestDist = 999999.0f;
        int bestAnimalEntityID = -1;
        auto nearbyAnimals = world.GetEntitiesInRadius(myEntity->position, 30.0f);

        for (Entity *ae : nearbyAnimals) {
          bool isHuntable = (ae->type == EntityType::Cow || ae->type == EntityType::Chicken ||
                             ae->type == EntityType::Sheep || ae->type == EntityType::Bull ||
                             ae->type == EntityType::Chicken2 || ae->type == EntityType::Lamb ||
                             ae->type == EntityType::Pig || ae->type == EntityType::Turkey);
          if (!isHuntable) continue;
          float d = std::hypot(myEntity->position.x - ae->position.x,
                               myEntity->position.y - ae->position.y);
          if (d < bestDist) {
            bestDist = d;
            bestAnimalEntityID = ae->id;
          }
        }

        if (bestAnimalEntityID >= 0) {
          Entity *prey = world.GetEntityByID(bestAnimalEntityID);
          c.workState = Citizen::WorkState::Hunting;
          c.isWorking = true;
          c.workTimer = 0.0f;
          c.targetEntityID = bestAnimalEntityID;
          myEntity->targetPos = prey->position;
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Run;
        }
      }
    }

    // === HUNTING STATE ===
    if (c.workState == Citizen::WorkState::Hunting && myEntity) {
      Entity *prey = world.GetEntityByID(c.targetEntityID);
      if (prey && (prey->health <= 0 || prey->state == EntityState::Die)) prey = nullptr;

      if (!prey) {
        c.workState = Citizen::WorkState::Idle;
        c.isWorking = false;
        myEntity->hasTarget = false;
        myEntity->state = EntityState::Idle;
      } else {
        float dist = std::hypot(myEntity->position.x - prey->position.x,
                                myEntity->position.y - prey->position.y);
        if (dist < 1.0f) {
          c.workTimer += deltaTime;
          myEntity->state = EntityState::Attack;
          myEntity->hasTarget = false;
          if (c.workTimer >= 0.5f) {
            c.workTimer = 0.0f;
            float damage = 5.0f + c.stats.strength;
            prey->health -= damage;
            prey->state = EntityState::Hurt;
            prey->animTime = 0.0f;
            prey->currentFrame = 0;

            if (prey->health <= 0) {
              prey->state = EntityState::Die;

              // Improved XP system for hunting
              // Base XP + Combat skill bonus + Personality bonus
              float xpGain = 25.0f;  // Base XP for hunting
              xpGain += c.stats.intelligence * 2.0f;  // Intelligence bonus
              if (c.personality == PersonalityTrait::Brave) xpGain *= 1.2f;  // Brave gets bonus
              if (c.personality == PersonalityTrait::Coward) xpGain *= 0.8f; // Coward gets less

              c.experience += xpGain;
              c.skillCombat += 1.0f;  // Increased skill gain

              // Personality effects on skill gain
              if (c.personality == PersonalityTrait::Hardworking) c.skillCombat += 0.5f;
              if (c.personality == PersonalityTrait::Lazy) c.skillCombat -= 0.2f;

              // Cap combat skill at 200
              if (c.skillCombat > 200.0f) c.skillCombat = 200.0f;

              int foodGain = (prey->type == EntityType::Cow || prey->type == EntityType::Bull) ? 5 : 3;
              c.hunger -= (float)foodGain * 8.0f;
              if (c.hunger < 0.0f) c.hunger = 0.0f;
              if (c.cityID >= 0) {
                City *myCity = GetCity(c.cityID);
                if (myCity) myCity->resources.food += foodGain;
              }
              c.workState = Citizen::WorkState::Idle;
              c.isWorking = false;
              myEntity->state = EntityState::Idle;
            }
          }
        } else {
          myEntity->targetPos = prey->position;
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Run;
        }
      }
      continue;
    }

    // === JOB AI: LUMBERJACK ===
    if (c.profession == Profession::Lumberjack && c.cityID >= 0 && c.isAdult()) {
      City *myCity = GetCity(c.cityID);
      if (!myCity) continue;

      switch (c.workState) {
        case Citizen::WorkState::Idle: {
          if (myCity->resources.wood >= myCity->maxStorage) {
            c.workState = Citizen::WorkState::Wandering;
            break;
          }

          if (c.carryingResource > 0) {
            c.workState = Citizen::WorkState::ReturningHome;
            myEntity->targetPos = myCity->center;
            myEntity->hasTarget = true;
            myEntity->state = EntityState::Walking;
            break;
          }

          // Find nearest tree
          int bestTileX = -1, bestTileY = -1;
          float bestDist = 999999.0f;
          const int SEARCH_RADIUS = 120; // Increased from 50

          int centerX = static_cast<int>(myEntity->position.x);
          int centerY = static_cast<int>(myEntity->position.y);

          for (int dy = -SEARCH_RADIUS; dy <= SEARCH_RADIUS; dy++) {
            for (int dx = -SEARCH_RADIUS; dx <= SEARCH_RADIUS; dx++) {
              int tx = centerX + dx;
              int ty = centerY + dy;

              if (tx < 0 || ty < 0 || tx >= world.GetWidth() || ty >= world.GetHeight()) continue;

              const Tile &t = world.GetTileConst(tx, ty);
              if (t.decoration == DecorationType::Tree ||
                  t.decoration == DecorationType::PineTree ||
                  t.decoration == DecorationType::PalmTree) {
                float dist = std::hypot(myEntity->position.x - tx, myEntity->position.y - ty);
                if (dist < bestDist) {
                  bestDist = dist;
                  bestTileX = tx;
                  bestTileY = ty;
                }
              }
            }
          }

          if (bestTileX >= 0) {
            c.targetTileX = bestTileX;
            c.targetTileY = bestTileY;
            c.workState = Citizen::WorkState::GoingToWork;
            c.isWorking = true;
            myEntity->targetPos = {bestTileX + 0.5f, bestTileY + 0.5f};
            myEntity->hasTarget = true;
            myEntity->state = EntityState::Walking;
          } else {
            c.workState = Citizen::WorkState::Wandering;
          }
          break;
        }

        case Citizen::WorkState::GoingToWork: {
          float distToTarget = std::hypot(myEntity->position.x - (c.targetTileX + 0.5f),
                                          myEntity->position.y - (c.targetTileY + 0.5f));
          if (distToTarget < 1.0f) {
            c.workState = Citizen::WorkState::Working;
            c.workTimer = 0.0f;
            myEntity->state = EntityState::Attack;
            myEntity->hasTarget = false;
          }
          break;
        }

        case Citizen::WorkState::Working: {
          c.workTimer += deltaTime;
          float chopTime = 3.0f - (c.skillWoodcutting * 0.03f);
          if (chopTime < 1.5f) chopTime = 1.5f;

          if (c.workTimer >= chopTime) {
            Tile &tile = world.GetTile(c.targetTileX, c.targetTileY);
            tile.originalTree = tile.decoration;
            tile.decoration = DecorationType::None;
            tile.hasStump = true;
            tile.stumpVariant = GRandom.Int(0, 1);
            tile.regrowthTimer = 0.0f;

            c.carryingResource += 10; // Increased from 5

            // Soft cap skill system: Diminishing returns after 100
            // Gain is reduced by 50% when skill > 100, 75% when > 150
            float woodcuttingGain = 0.5f;
            if (c.skillWoodcutting > 100.0f) woodcuttingGain *= 0.5f;
            if (c.skillWoodcutting > 150.0f) woodcuttingGain *= 0.25f;
            c.skillWoodcutting += woodcuttingGain;

            // Hard cap at 200
            if (c.skillWoodcutting > 200.0f) c.skillWoodcutting = 200.0f;

            c.experience += 15.0f;

            if (c.carryingResource >= c.maxCarryCapacity) {
              c.workState = Citizen::WorkState::ReturningHome;
              float jx = GRandom.FloatRange(-1.2f, 1.2f);
              float jy = GRandom.FloatRange(-1.2f, 1.2f);
              myEntity->targetPos = {myCity->center.x + jx, myCity->center.y + jy};
              myEntity->hasTarget = true;
              myEntity->state = EntityState::Walking;
            } else {
              c.workState = Citizen::WorkState::Idle;
            }
          }
          break;
        }

        case Citizen::WorkState::ReturningHome: {
          float distToCity = std::hypot(myEntity->position.x - myCity->center.x,
                                        myEntity->position.y - myCity->center.y);
          if (distToCity < 3.0f) {
            myCity->resources.wood += c.carryingResource;
            c.carryingResource = 0;
            c.workState = Citizen::WorkState::Idle;
            c.isWorking = false;
            myEntity->state = EntityState::Idle;
          } else {
            myEntity->targetPos = myCity->center;
            myEntity->hasTarget = true;
            myEntity->state = EntityState::Walking;
          }
          break;
        }

        default:
          c.workState = Citizen::WorkState::Wandering;
          break;
      }
    }

    // === JOB AI: FARMER ===
    if (c.profession == Profession::Farmer && c.cityID >= 0 && c.isAdult()) {
      City *myCity = GetCity(c.cityID);
      if (!myCity) continue;

      switch (c.workState) {
        case Citizen::WorkState::Idle: {
          // Find crops to harvest or plant
          std::vector<std::pair<float, std::pair<int, int>>> harvestCandidates;
          std::vector<std::pair<float, std::pair<int, int>>> plantCandidates;

          for (const Vector2 &tile : myCity->territory) {
            int tx = static_cast<int>(tile.x);
            int ty = static_cast<int>(tile.y);
            Tile &t = world.GetTile(tx, ty);

            if (t.isPlanted && t.growthProgress >= 100.0f && t.farmOwnerCityID == myCity->id) {
              float dist = std::hypot(myEntity->position.x - tx, myEntity->position.y - ty);
              harvestCandidates.push_back({dist, {tx, ty}});
            }

            bool needsFood = myCity->resources.food < (myCity->maxStorage * 0.8f);
            bool canPlantCrop = needsFood && !t.isPlanted && t.type == TileType::Grass &&
                                t.decoration == DecorationType::None;
            if (canPlantCrop) {
              float dist = std::hypot(myEntity->position.x - tx, myEntity->position.y - ty);
              plantCandidates.push_back({dist, {tx, ty}});
            }
          }

          int harvestX = -1, harvestY = -1;
          if (!harvestCandidates.empty()) {
            std::sort(harvestCandidates.begin(), harvestCandidates.end());
            int pick = GRandom.Int(0, std::min((int)harvestCandidates.size(), 5) - 1);
            harvestX = harvestCandidates[pick].second.first;
            harvestY = harvestCandidates[pick].second.second;
          }

          int plantX = -1, plantY = -1;
          if (!plantCandidates.empty()) {
            std::sort(plantCandidates.begin(), plantCandidates.end());
            int pick = GRandom.Int(0, std::min((int)plantCandidates.size(), 5) - 1);
            plantX = plantCandidates[pick].second.first;
            plantY = plantCandidates[pick].second.second;
          }

          if (harvestX >= 0) {
            c.targetTileX = harvestX;
            c.targetTileY = harvestY;
            c.workState = Citizen::WorkState::GoingToWork;
            c.currentJob = Citizen::JobType::Farming;
            c.isWorking = true;
            c.workTimer = 0.0f;
            myEntity->targetPos = {harvestX + 0.5f, harvestY + 0.5f};
            myEntity->hasTarget = true;
            myEntity->state = EntityState::Walking;
          } else if (plantX >= 0) {
            c.targetTileX = plantX;
            c.targetTileY = plantY;
            c.workState = Citizen::WorkState::GoingToWork;
            c.currentJob = Citizen::JobType::Farming;
            c.isWorking = true;
            c.workTimer = -1.0f;
            myEntity->targetPos = {plantX + 0.5f, plantY + 0.5f};
            myEntity->hasTarget = true;
            myEntity->state = EntityState::Walking;
          } else {
            c.workState = Citizen::WorkState::Wandering;
          }
          break;
        }

        case Citizen::WorkState::GoingToWork: {
          float distToTarget = std::hypot(myEntity->position.x - (c.targetTileX + 0.5f),
                                          myEntity->position.y - (c.targetTileY + 0.5f));
          if (distToTarget < 1.0f) {
            c.workState = Citizen::WorkState::Working;
            myEntity->state = EntityState::Attack;
            myEntity->currentFrame = 0;
            myEntity->hasTarget = false;
            if (c.workTimer < 0) c.workTimer = 0.0f;
          }
          break;
        }

        case Citizen::WorkState::Working: {
          c.workTimer += deltaTime;
          Tile &tile = world.GetTile(c.targetTileX, c.targetTileY);

          // Planting
          if (c.currentJob == Citizen::JobType::PlantingTree) {
            if (c.workTimer >= 2.0f) {
              if (!tile.isOccupied) {
                tile.decoration = DecorationType::Tree;
                tile.hasStump = false;
                tile.decorationVariant = GRandom.Int(0, 2);
              }
              c.workState = Citizen::WorkState::Idle;
              c.isWorking = false;
              c.currentJob = Citizen::JobType::None;
            }
          } else if (!tile.isPlanted && c.currentJob == Citizen::JobType::Farming) {
            if (c.workTimer >= 2.0f) {
              tile.isPlanted = true;
              tile.growthProgress = 0.0f;
              tile.farmOwnerCityID = myCity->id;

              // Soft cap skill system for farming
              float farmingGain = 0.3f;
              if (c.skillFarming > 100.0f) farmingGain *= 0.5f;
              if (c.skillFarming > 150.0f) farmingGain *= 0.25f;
              c.skillFarming += farmingGain;
              if (c.skillFarming > 200.0f) c.skillFarming = 200.0f;

              c.experience += 8.0f;
              c.workState = Citizen::WorkState::Idle;
              c.isWorking = false;
              c.currentJob = Citizen::JobType::None;
            }
          }
          // Harvesting
          else if (tile.growthProgress >= 100.0f) {
            if (c.workTimer >= 1.5f) {
              int harvestAmount = 3 + static_cast<int>(c.skillFarming * 0.05f);
              c.carryingResource += harvestAmount;
              tile.isPlanted = false;
              tile.growthProgress = 0.0f;
              tile.farmOwnerCityID = -1;

              // Soft cap skill system for farming
              float farmingGain = 0.5f;
              if (c.skillFarming > 100.0f) farmingGain *= 0.5f;
              if (c.skillFarming > 150.0f) farmingGain *= 0.25f;
              c.skillFarming += farmingGain;
              if (c.skillFarming > 200.0f) c.skillFarming = 200.0f;

              c.experience += 12.0f;

              if (c.carryingResource >= c.maxCarryCapacity) {
                c.workState = Citizen::WorkState::ReturningHome;
                float jx = GRandom.FloatRange(-1.2f, 1.2f);
                float jy = GRandom.FloatRange(-1.2f, 1.2f);
                myEntity->targetPos = {myCity->center.x + jx, myCity->center.y + jy};
                myEntity->hasTarget = true;
                myEntity->state = EntityState::Walking;
              } else {
                c.workState = Citizen::WorkState::Idle;
              }
            }
          }
          break;
        }

        case Citizen::WorkState::ReturningHome: {
          float distToCity = std::hypot(myEntity->position.x - myCity->center.x,
                                        myEntity->position.y - myCity->center.y);
          if (distToCity < 3.0f) {
            myCity->resources.food += c.carryingResource;
            c.carryingResource = 0;
            c.workState = Citizen::WorkState::Idle;
            c.isWorking = false;
            myEntity->state = EntityState::Idle;
          } else {
            myEntity->targetPos = myCity->center;
            myEntity->hasTarget = true;
            myEntity->state = EntityState::Walking;
          }
          break;
        }

        case Citizen::WorkState::Wandering: {
          c.stateTimer += deltaTime;
          if (!myEntity->hasTarget && !myCity->territory.empty()) {
            int idx = GRandom.Int(0, (int)myCity->territory.size() - 1);
            float tx = myCity->territory[idx].x + 0.5f;
            float ty = myCity->territory[idx].y + 0.5f;
            if (world.IsWalkable((int)tx, (int)ty)) {
              myEntity->targetPos = {tx, ty};
              myEntity->hasTarget = true;
              myEntity->state = EntityState::Walking;
            }
          }
          if (c.stateTimer >= 5.0f) {
            c.workState = Citizen::WorkState::Idle;
            c.stateTimer = 0.0f;
          }
          break;
        }

        default:
          c.workState = Citizen::WorkState::Idle;
          break;
      }
    }

    // === JOB AI: MINER ===
    if (c.profession == Profession::Miner && c.cityID >= 0 && c.isAdult()) {
      City *myCity = GetCity(c.cityID);
      if (!myCity) continue;

      switch (c.workState) {
        case Citizen::WorkState::Idle: {
          if (myCity->resources.stone >= myCity->maxStorage) {
            c.workState = Citizen::WorkState::Wandering;
            break;
          }

          int bestX = -1, bestY = -1;
          float bestDist = 999999.0f;
          int range = 120; // Increased from 60
          int cx = (int)myEntity->position.x, cy = (int)myEntity->position.y;

          for (int dy = -range; dy <= range; dy++) {
            for (int dx = -range; dx <= range; dx++) {
              int tx = cx + dx;
              int ty = cy + dy;
              if (tx < 0 || ty < 0 || tx >= world.GetWidth() || ty >= world.GetHeight()) continue;

              const Tile &t = world.GetTileConst(tx, ty);
              bool isRock = (t.decoration == DecorationType::Rock || t.decoration == DecorationType::SmallRock ||
                             t.decoration == DecorationType::MediumRock || t.decoration == DecorationType::BigRock ||
                             t.decoration == DecorationType::Crystal);
              if (isRock || t.type == TileType::Mountain) {
                float dist = std::hypot(myEntity->position.x - tx, myEntity->position.y - ty);
                if (dist < bestDist) {
                  bestDist = dist;
                  bestX = tx;
                  bestY = ty;
                }
              }
            }
          }

          if (bestX >= 0) {
            c.targetTileX = bestX;
            c.targetTileY = bestY;
            c.workState = Citizen::WorkState::GoingToWork;
            c.isWorking = true;
            myEntity->targetPos = {bestX + 0.5f, bestY + 0.5f};
            myEntity->hasTarget = true;
            myEntity->state = EntityState::Walking;
          } else {
            c.workState = Citizen::WorkState::Wandering;
          }
          break;
        }

        case Citizen::WorkState::GoingToWork: {
          float dist = std::hypot(myEntity->position.x - (c.targetTileX + 0.5f),
                                  myEntity->position.y - (c.targetTileY + 0.5f));
          if (dist < 1.0f) {
            c.workState = Citizen::WorkState::Working;
            c.workTimer = 0.0f;
            myEntity->state = EntityState::Attack;
            myEntity->hasTarget = false;
          }
          break;
        }

        case Citizen::WorkState::Working: {
          c.workTimer += deltaTime;
          if (c.workTimer >= 3.0f) {
            c.workTimer = 0.0f;
            Tile &t = world.GetTile(c.targetTileX, c.targetTileY);

            bool rockExists = (t.decoration != DecorationType::None && t.type != TileType::Mountain);
            if (rockExists) {
              if (t.resourceAmount > 0) {
                t.resourceAmount -= 1.0f;
                c.carryingResource += 5; // Increased from 2

                // Soft cap skill system for mining
                float miningGain = 0.2f;
                if (c.skillMining > 100.0f) miningGain *= 0.5f;
                if (c.skillMining > 150.0f) miningGain *= 0.25f;
                c.skillMining += miningGain;
                if (c.skillMining > 200.0f) c.skillMining = 200.0f;

                c.experience += 5.0f;
                if (t.resourceAmount <= 0) t.decoration = DecorationType::None;
              } else {
                t.decoration = DecorationType::None;
               c.carryingResource += 2; // Increased from 1
              }
            } else if (t.type == TileType::Mountain) {
              c.carryingResource += 2; // Increased from 1

              // Soft cap skill system for mining (harder work)
              float miningGain = 0.1f;
              if (c.skillMining > 100.0f) miningGain *= 0.5f;
              if (c.skillMining > 150.0f) miningGain *= 0.25f;
              c.skillMining += miningGain;
              if (c.skillMining > 200.0f) c.skillMining = 200.0f;

              c.experience += 5.0f;
            } else {
              c.workState = Citizen::WorkState::Idle;
              myEntity->state = EntityState::Idle;
              break;
            }

            if (c.carryingResource >= c.maxCarryCapacity) {
              c.workState = Citizen::WorkState::ReturningHome;
              float jx = GRandom.FloatRange(-1.2f, 1.2f);
              float jy = GRandom.FloatRange(-1.2f, 1.2f);
              myEntity->targetPos = {myCity->center.x + jx, myCity->center.y + jy};
              myEntity->hasTarget = true;
              myEntity->state = EntityState::Walking;
            }
          }
          break;
        }

        case Citizen::WorkState::ReturningHome: {
          float dist = std::hypot(myEntity->position.x - myCity->center.x,
                                  myEntity->position.y - myCity->center.y);
          if (dist < 3.0f) {
            myCity->resources.stone += c.carryingResource;
            if (myCity->resources.stone > myCity->maxStorage) myCity->resources.stone = myCity->maxStorage;
            if (GRandom.Chance(10)) myCity->resources.ore += 1;
            c.carryingResource = 0;
            c.workState = Citizen::WorkState::Idle;
            c.isWorking = false;
            myEntity->state = EntityState::Idle;
          } else {
            myEntity->targetPos = myCity->center;
            myEntity->hasTarget = true;
            myEntity->state = EntityState::Walking;
          }
          break;
        }

        default:
          c.workState = Citizen::WorkState::Wandering;
          break;
      }
    }

    // === JOB AI: BUILDER (REWRITTEN) ===
    if (c.profession == Profession::Builder && c.cityID >= 0 && c.isAdult()) {
      City *myCity = GetCity(c.cityID);
      if (!myCity) continue;

      switch (c.workState) {
        case Citizen::WorkState::Idle: {
          // Priority 1: Complete existing construction sites
          int bestIdx = -1;
          float bestDist = 999999.0f;
          float minProgress = 2.0f; // Need progress > 0

          for (size_t i = 0; i < myCity->buildings.size(); i++) {
            Building &b = myCity->buildings[i];
            if (!b.isComplete && b.constructionProgress > 0 && b.constructionProgress < 1.0f) {
              float dist = std::hypot(myEntity->position.x - b.tileX, myEntity->position.y - b.tileY);
              if (dist < bestDist) {
                bestDist = dist;
                bestIdx = (int)i;
              }
            }
          }

          // Priority 2: Start building new buildings if we have resources
          if (bestIdx < 0 && doBuildCheck) {
            // Check if city should build something
            int totalStorage = 200;
            int plannedCapacity = 0;
            for (const auto &b : myCity->buildings) {
              if (b.isComplete) {
                if (b.type == BuildingType::Recursos || b.type == BuildingType::StockpileStone)
                  totalStorage += 500;
              }
              if (b.IsHousing()) {
                int cap = b.capacity > 0 ? b.capacity : 2;
                plannedCapacity += cap;
              }
            }

            bool needHousing = myCity->GetPopulation() > (plannedCapacity - 2);
            bool needWoodStorage = myCity->resources.wood >= (myCity->maxStorage * 0.8f);
            bool needStoneStorage = myCity->resources.stone >= (myCity->maxStorage * 0.8f);

            BuildingType typeToBuild = BuildingType::None;
            int woodCost = 0;
            int stoneCost = 0;

            if (needHousing && myCity->resources.wood >= 5) {
              if (myCity->buildings.size() < 3) {
                typeToBuild = BuildingType::Cabana;
                woodCost = 2; stoneCost = 0;
              } else if (myCity->buildings.size() < 8) {
                typeToBuild = BuildingType::Casa;
                woodCost = 5; stoneCost = 2;
              } else {
                typeToBuild = BuildingType::Casa2;
                woodCost = 8; stoneCost = 5;
              }
            } else if (needWoodStorage && myCity->resources.wood >= 25) {
              typeToBuild = BuildingType::Recursos;
              woodCost = 20; stoneCost = 0;
            } else if (needStoneStorage && myCity->resources.stone >= 25) {
              typeToBuild = BuildingType::StockpileStone;
              woodCost = 25; stoneCost = 0;
            }

            // If we found a building to construct, do it now
            if (typeToBuild != BuildingType::None && myCity->resources.wood >= woodCost) {
              myCity->resources.wood -= woodCost;
              myCity->resources.stone -= stoneCost;

              Building newBuilding;
              newBuilding.id = GetNextBuildingID();
              newBuilding.cityID = myCity->id;
              newBuilding.tileX = static_cast<int>(myCity->center.x);
              newBuilding.tileY = static_cast<int>(myCity->center.y);
              newBuilding.isComplete = false;
              newBuilding.constructionProgress = 0.0f;
              newBuilding.type = typeToBuild;
              newBuilding.variant = GRandom.Int(0, 2);
              newBuilding.capacity = GetBuildingHousingCapacity(typeToBuild);

              // Find valid placement near center
              bool placed = false;
              for (int dy = -2; dy <= 2 && !placed; dy++) {
                for (int dx = -2; dx <= 2 && !placed; dx++) {
                  int bx = static_cast<int>(myCity->center.x) + dx;
                  int by = static_cast<int>(myCity->center.y) + dy;
                  if (bx >= 0 && by >= 0 && bx < world.GetWidth() && by < world.GetHeight()) {
                    if (world.IsWalkable(bx, by)) {
                      // Check tiles this building will occupy
                      BuildingSize size = GetBuildingSize(typeToBuild);
                      bool canPlace = true;
                      for (int bdy = 0; bdy < size.height && canPlace; bdy++) {
                        for (int bdx = 0; bdx < size.width && canPlace; bdx++) {
                          int tx = bx + bdx, ty = by + bdy;
                          if (tx >= 0 && tx < world.GetWidth() && ty >= 0 && ty < world.GetHeight()) {
                            if (world.GetTileConst(tx, ty).isOccupied) canPlace = false;
                          }
                        }
                      }
                      if (canPlace) {
                        newBuilding.tileX = bx;
                        newBuilding.tileY = by;
                        placed = true;
                        // Mark tiles as occupied
                        for (int bdy = 0; bdy < size.height; bdy++) {
                          for (int bdx = 0; bdx < size.width; bdx++) {
                            int tx = bx + bdx, ty = by + bdy;
                            if (tx >= 0 && tx < world.GetWidth() && ty >= 0 && ty < world.GetHeight()) {
                              world.GetTile(tx, ty).isOccupied = true;
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }

              if (placed) {
                myCity->buildings.push_back(newBuilding);
                TraceLog(LOG_INFO, "CITY %d: Started building %s at (%d,%d)",
                         myCity->id, GetBuildingName(typeToBuild), newBuilding.tileX, newBuilding.tileY);
              }
            }
          }

          // Find the best construction site - Add some randomness to distribute workers
          if (bestIdx < 0) {
            for (size_t i = 0; i < myCity->buildings.size(); i++) {
              Building &b = myCity->buildings[i];
              if (!b.isComplete && b.constructionProgress >= 0 && b.constructionProgress < 1.0f) {
                float dist = std::hypot(myEntity->position.x - b.tileX, myEntity->position.y - b.tileY);
                // Add random bias (0-10 tiles) to distance to distribute workers across different sites
                dist += (GRandom.Float() * 10.0f);
                if (dist < bestDist) {
                  bestDist = dist;
                  bestIdx = (int)i;
                }
              }
            }
          }

          if (bestIdx >= 0) {
            Building &b = myCity->buildings[bestIdx];
            c.targetBuildingIdx = bestIdx;
            c.workState = Citizen::WorkState::GoingToWork;
            c.isWorking = true;
            BuildingSize size = GetBuildingSize(b.type);
            // CLUMPING FIX: Add jitter to building target
            float jx = GRandom.FloatRange(-0.7f, 0.7f);
            float jy = GRandom.FloatRange(-0.7f, 0.7f);
            myEntity->targetPos = {b.tileX + size.width / 2.0f + jx, b.tileY + size.height / 2.0f + jy};
            myEntity->hasTarget = true;
            myEntity->state = EntityState::Walking;
          } else {
            c.workState = Citizen::WorkState::Wandering;
          }
          break;
        }

        case Citizen::WorkState::GoingToWork: {
          Building &b = myCity->buildings[c.targetBuildingIdx];
          BuildingSize size = GetBuildingSize(b.type);
          float dist = std::hypot(myEntity->position.x - (b.tileX + size.width / 2.0f),
                                  myEntity->position.y - (b.tileY + size.height / 2.0f));
          if (dist < 2.0f) { // Increased from 1.5
            c.workState = Citizen::WorkState::Working;
            c.workTimer = 0.0f;
            myEntity->state = EntityState::Attack;
            myEntity->hasTarget = false;
          }
          break;
        }

        case Citizen::WorkState::Working: {
          c.workTimer += deltaTime;
          Building &b = myCity->buildings[c.targetBuildingIdx];

          // Building progress: MUCH faster construction
          float buildTime = 1.0f + (b.type == BuildingType::Casa ? 1.0f : 0) +
                           (b.type == BuildingType::Casa2 ? 2.0f : 0) +
                           (b.type == BuildingType::Recursos ? 1.0f : 0);
          if (buildTime > 8.0f) buildTime = 8.0f;

          if (c.workTimer >= buildTime) {
            c.workTimer = 0.0f;

            // Check if building is already complete (might have been finished by another builder)
            if (!b.isComplete && b.constructionProgress < 1.0f) {
              // Finish faster
              float progressGain = 0.5f;
              if (b.type == BuildingType::Cabana || b.type == BuildingType::Casa) progressGain = 1.0f;
              b.constructionProgress += progressGain;

              // Soft cap skill system for building
              float buildingGain = 0.5f;
              if (c.skillBuilding > 100.0f) buildingGain *= 0.5f;
              if (c.skillBuilding > 150.0f) buildingGain *= 0.25f;
              c.skillBuilding += buildingGain;
              if (c.skillBuilding > 200.0f) c.skillBuilding = 200.0f;

              c.experience += 10.0f;

              if (b.constructionProgress >= 1.0f) {
                b.isComplete = true;
                TraceLog(LOG_INFO, "BUILDER: Citizen %d completed building %s at (%d,%d)",
                         c.id, GetBuildingName(b.type), b.tileX, b.tileY);
              } else {
                TraceLog(LOG_INFO, "BUILDER: Citizen %d worked on %s at (%d,%d) - Progress: %.0f%%",
                         c.id, GetBuildingName(b.type), b.tileX, b.tileY, b.constructionProgress * 100);
              }
            }

            // If still working on building, stay idle to wait
            if (!b.isComplete && b.constructionProgress < 1.0f) {
              c.workState = Citizen::WorkState::Idle;
              // Pick another building if available
              for (size_t i = 0; i < myCity->buildings.size(); i++) {
                Building &other = myCity->buildings[i];
                if (!other.isComplete && other.constructionProgress < 1.0f && i != (size_t)c.targetBuildingIdx) {
                  c.targetBuildingIdx = (int)i;
                  c.workState = Citizen::WorkState::GoingToWork;
                  myEntity->targetPos = {other.tileX + 0.5f, other.tileY + 0.5f};
                  myEntity->hasTarget = true;
                  myEntity->state = EntityState::Walking;
                  break;
                }
              }
            } else {
              c.workState = Citizen::WorkState::Idle;
            }
          }
          break;
        }

        case Citizen::WorkState::ReturningHome:
        case Citizen::WorkState::Wandering:
        default:
          c.workState = Citizen::WorkState::Idle;
          break;
      }
    }

    // === JOB AI: SOLDIER ===
    if (c.profession == Profession::Soldier && c.cityID >= 0 && c.isAdult()) {
      if (myEntity && myEntity->type != EntityType::HumanArmed) {
        myEntity->type = EntityType::HumanArmed;
      }

      City *myCity = GetCity(c.cityID);
      if (!myCity) continue;

      switch (c.workState) {
        case Citizen::WorkState::Idle:
        case Citizen::WorkState::Wandering: {
          // Scan for enemies
          int enemyID = -1;
          float closestDist = 999999.0f;
          float detectRange = 25.0f;

          auto nearbyEntities = world.GetEntitiesInRadius(myEntity->position, detectRange);
          int bestPriority = 0; // 1:Slime, 2:Boar, 3:Human, 4:Dragon

          for (Entity *otherE : nearbyEntities) {
            if (otherE->id == myEntity->id || otherE->health <= 0) continue;
            
            float d = std::hypot(myEntity->position.x - otherE->position.x,
                                 myEntity->position.y - otherE->position.y);
            
            int priority = 0;
            int potentialTargetID = -1;

            if (otherE->type == EntityType::Dragon) {
                priority = 4;
                potentialTargetID = otherE->id; // For monsters, use entity ID (handled in Working state)
            } else if (otherE->IsIntelligent()) {
                Citizen *otherCitizen = GetCitizen(otherE->citizenID);
                if (otherCitizen && otherCitizen->cityID != c.cityID && otherCitizen->cityID >= 0) {
                    bool isEnemy = false;
                    City *otherCity = GetCity(otherCitizen->cityID);
                    Kingdom *myKingdom = myCity->kingdomID >= 0 ? GetKingdom(myCity->kingdomID) : nullptr;
                    Kingdom *otherKingdom = otherCity->kingdomID >= 0 ? GetKingdom(otherCity->kingdomID) : nullptr;

                    if (myKingdom && otherKingdom && myKingdom->IsAtWarWith(otherKingdom->id)) isEnemy = true;
                    if (!isEnemy && myCity) {
                        float distToMyCity = std::hypot(otherE->position.x - myCity->center.x,
                                                        otherE->position.y - myCity->center.y);
                        if (distToMyCity < 20.0f) isEnemy = true;
                    }
                    if (isEnemy) {
                        priority = 3;
                        potentialTargetID = otherCitizen->id;
                    }
                }
            } else if (otherE->type == EntityType::Boar) {
                priority = 2;
                potentialTargetID = otherE->id;
            } else if (otherE->type == EntityType::Slime) {
                priority = 1;
                potentialTargetID = otherE->id;
            }

            if (priority > 0 && (priority > bestPriority || (priority == bestPriority && d < closestDist))) {
                bestPriority = priority;
                closestDist = d;
                enemyID = potentialTargetID;
            }
          }

          if (enemyID != -1) {
            c.workState = Citizen::WorkState::GoingToWork;
            c.targetEntityID = enemyID;
            c.isWorking = true;
            myEntity->hasTarget = true;
            myEntity->state = EntityState::Run;
          } else {
            // Patrol
            if (!myEntity->hasTarget) {
              int r = GRandom.Int(0, 3);
              int dx = 0, dy = 0;
              if (r == 0) dx = 1;
              else if (r == 1) dx = -1;
              else if (r == 2) dy = 1;
              else dy = -1;

              int maxDist = GRandom.Int(4, 11);
              int cx = (int)myEntity->position.x;
              int cy = (int)myEntity->position.y;
              int validDist = 0;

              for (int i = 1; i <= maxDist; i++) {
                int checkX = cx + dx * i;
                int checkY = cy + dy * i;
                if (checkX >= 0 && checkX < world.GetWidth() && checkY >= 0 && checkY < world.GetHeight()) {
                  if (world.IsWalkable(checkX, checkY)) {
                    if (world.GetTileConst(checkX, checkY).ownerCityID == c.cityID) {
                      validDist = i;
                    } else {
                      break;
                    }
                  } else {
                    break;
                  }
                } else {
                  break;
                }
              }

              if (validDist > 0) {
                myEntity->targetPos = {(float)(cx + dx * validDist) + 0.5f,
                                       (float)(cy + dy * validDist) + 0.5f};
                myEntity->hasTarget = true;
                myEntity->state = EntityState::Walking;
              }
            }
          }
          break;
        }

        case Citizen::WorkState::GoingToWork: {
          Citizen *enemy = GetCitizen(c.targetEntityID);
          if (!enemy || !enemy->isAlive) {
            c.workState = Citizen::WorkState::Idle;
            myEntity->hasTarget = false;
            myEntity->state = EntityState::Idle;
            break;
          }

          Entity *enemyEntity = world.GetEntityByCitizenID(enemy->id);
          if (!enemyEntity) {
            c.workState = Citizen::WorkState::Idle;
            break;
          }

          myEntity->targetPos = enemyEntity->position;
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Run;

          float dist = std::hypot(myEntity->position.x - enemyEntity->position.x,
                                  myEntity->position.y - enemyEntity->position.y);
          if (dist < 1.2f) {
            c.workState = Citizen::WorkState::Working;
            c.workTimer = 0.0f;
            myEntity->state = EntityState::Attack;
            myEntity->currentFrame = 0;
            myEntity->hasTarget = false;
          }
          break;
        }

        case Citizen::WorkState::Working: {
          Citizen *enemy = GetCitizen(c.targetEntityID);
          if (!enemy || !enemy->isAlive) {
            c.workState = Citizen::WorkState::Idle;
            myEntity->state = EntityState::Idle;
            break;
          }

          Entity *enemyEntity = world.GetEntityByCitizenID(enemy->id);
          if (!enemyEntity) {
            c.workState = Citizen::WorkState::Idle;
            break;
          }

          float dist = std::hypot(myEntity->position.x - enemyEntity->position.x,
                                  myEntity->position.y - enemyEntity->position.y);
          if (dist > 2.0f) {
            c.workState = Citizen::WorkState::GoingToWork;
            break;
          }

          c.workTimer += deltaTime;
          if (c.workTimer >= 0.5f) {
            c.workTimer = 0.0f;
            float damage = 15.0f;
            enemy->health -= damage;
            enemyEntity->state = EntityState::Hurt;
            enemyEntity->animTime = 0.0f;
            enemyEntity->currentFrame = 0;

            // Combat skill gain with soft cap
            float combatGain = 0.3f;
            if (c.skillCombat > 100.0f) combatGain *= 0.5f;
            if (c.skillCombat > 150.0f) combatGain *= 0.25f;
            c.skillCombat += combatGain;
            if (c.skillCombat > 200.0f) c.skillCombat = 200.0f;
          }
          break;
        }

        default:
          c.workState = Citizen::WorkState::Idle;
          break;
      }
    }

    // === Fallback ===
    if (c.workState == Citizen::WorkState::Idle && c.profession == Profession::None) {
      c.workState = Citizen::WorkState::Wandering;
    }

    // === RICH IDLE & EXPLORATION ===
    if (c.workState == Citizen::WorkState::Wandering && c.stateTimer == 0.0f) {
      // Chance to SCOUT (Long range wander) to find new city spots
      if (GRandom.Chance(5)) { // 5% chance to scout far away
         int scoutX = GRandom.Int(20, world.GetWidth() - 20);
         int scoutY = GRandom.Int(20, world.GetHeight() - 20);
         if (world.IsWalkable(scoutX, scoutY)) {
            myEntity->targetPos = {(float)scoutX + 0.5f, (float)scoutY + 0.5f};
            myEntity->hasTarget = true;
            myEntity->state = EntityState::Walking;
            c.workTimer = (float)GRandom.Int(20, 40); // Long scout mission
         }
      }
      else if (GRandom.Chance(25)) {
        int r = GRandom.Int(0, 2);
        if (r == 0) c.workState = Citizen::WorkState::IdleSitting;
        else if (r == 1) c.workState = Citizen::WorkState::IdleObserving;
        else c.workState = Citizen::WorkState::IdleSocializing;
        c.workTimer = (float)GRandom.Int(3, 6);
      }
    }
  }
}
