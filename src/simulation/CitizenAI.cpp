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
void SimulationManager::UpdateCitizens(World &world, float deltaTime) {

  // Settler check timer (don't check every frame)
  static float settlerCheckTimer = 0.0f;
  settlerCheckTimer += deltaTime;
  bool doSettlerCheck = settlerCheckTimer >= 3.0f; // Check every 3 seconds
  if (doSettlerCheck)
    settlerCheckTimer = 0.0f;

  for (auto &pair : citizens) {
    Citizen &c = pair.second;
    if (!c.isAlive)
      continue;

    // === FIND ENTITY ===
    Entity *myEntity = world.GetEntityByCitizenID(c.id);

    // Age - much faster for gameplay!
    // Children age faster to become adults sooner (game-years per second)
    float ageMultiplier = c.isChild() ? 5.0f : 1.0f; // Children grow 5x faster
    float yearProgress =
        deltaTime * 0.05f *
        ageMultiplier; // ~0.2 years per second base (Slower aging)
    c.age += yearProgress;

    // === HOSTILE DETECTION (Flee Logic) ===
    if (myEntity && c.isAlive && c.health > 0) {
      float fleeRadius = 8.0f; // Detect hostiles within 8 tiles
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
        // Unarmed citizens (or non-soldiers) FLEE
        if (c.profession != Profession::Soldier) {
          Vector2 fleeDir = Vector2Normalize(
              Vector2Subtract(myEntity->position, threat->position));
          myEntity->targetPos =
              Vector2Add(myEntity->position, Vector2Scale(fleeDir, 3.0f));
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Run;
          c.workState = Citizen::WorkState::Idle; // Interrupt current job
          continue; // Skip rest of AI logic for this citizen this frame
        }
      }
    }

    // === PASSIVE XP (survival experience) ===
    c.experience += deltaTime * 0.3f; // Small passive XP gain just for living

    // === LEVEL UP CHECK (universal - catches all XP sources) ===
    while (c.experience >= c.maxExperience) {
      c.experience -= c.maxExperience;
      c.level++;
      c.maxExperience = 50.0f + (c.level * 25.0f); // Scales per level
      // Stat boosts on level up
      c.stats.strength += 0.5f;
      c.stats.speed += 0.3f;
      c.stats.endurance += 0.5f;
      c.stats.intelligence += 0.3f;
      c.maxHealth += 5.0f;
      c.health = c.maxHealth;  // Full heal on level up
      c.maxCarryCapacity += 1; // Can carry more
      TraceLog(LOG_INFO,
               "LEVEL UP: Citizen %d reached level %d! (next: %.0f XP)", c.id,
               c.level, c.maxExperience);
    }

    if (c.age > c.genes.maxAge) {
      c.isAlive = false; // Died of old age
      if (myEntity) {
        myEntity->state = EntityState::Die;
        myEntity->currentFrame = 0;
        myEntity->animTime = 0.0f;
      }
      continue;
    }

    // Hunger increases over time
    c.hunger += deltaTime * 0.1f; // Gets hungry much slower
    if (c.hunger > 100.0f)
      c.hunger = 100.0f;

    // Starving damages health
    if (c.hunger >= 80.0f) {
      c.health -= deltaTime * 1.0f; // Lose 1 HP per second when starving
    }

    // Death from starvation
    if (c.health <= 0.0f) {
      c.isAlive = false;
      if (myEntity) {
        myEntity->state = EntityState::Die;
        myEntity->currentFrame = 0;
        myEntity->animTime = 0.0f;
      }
      continue;
    }

    // === COLD / HYPOTHERMIA SYSTEM ===
    if (myEntity) {
      int tileX = static_cast<int>(myEntity->position.x);
      int tileY = static_cast<int>(myEntity->position.y);
      if (tileX >= 0 && tileX < world.GetWidth() && tileY >= 0 &&
          tileY < world.GetHeight()) {
        const Tile &currentTile = world.GetTileConst(tileX, tileY);
        if (currentTile.biome == BiomeType::Snow) {
          // Cooling rate — reduced if citizen has shelter nearby
          float coolingRate = 2.0f;
          if (c.homeID != -1) {
            Building *home = GetBuilding(c.homeID);
            if (home) {
              float distToHome = std::hypot(myEntity->position.x - home->tileX,
                                            myEntity->position.y - home->tileY);
              if (distToHome < 5.0f) {
                coolingRate *= 0.2f; // 80% reduction near home
              }
            }
          }
          c.bodyTemperature -= deltaTime * coolingRate;
        } else {
          // Warm up when not in snow
          if (c.bodyTemperature < 37.0f) {
            c.bodyTemperature += deltaTime * 1.5f;
            if (c.bodyTemperature > 37.0f)
              c.bodyTemperature = 37.0f;
          }
        }
        // Cold damage
        if (c.bodyTemperature < 30.0f) {
          c.health -= deltaTime * 3.0f;
        }
        // Clamp
        if (c.bodyTemperature < 0.0f)
          c.bodyTemperature = 0.0f;
      }
    }

    // === STAMINA & HOME SYSTEM ===
    // 1. Recovery at Home (or Homeless Rest)
    if (c.isResting) {
      float recoveryRate = (c.homeID != -1)
                               ? 15.0f
                               : 20.0f; // Slower if homeless (was 8.0f, boosted
                                        // to 20.0f for faster recovery)
      c.energy += deltaTime * recoveryRate;
      c.health += deltaTime * 5.0f; // Recover health
      if (c.health > c.maxHealth)
        c.health = c.maxHealth;

      // Homeless recover only to 60% if they sleep on the ground?
      // For now let them recover fully but slowly
      if (c.energy >= 100.0f) {
        c.energy = 100.0f;
        c.isResting = false;
        c.isGoingHome = false;
        c.workState = Citizen::WorkState::Idle;
      } else {
        // Stay resting
        if (myEntity) {
          myEntity->hasTarget = false;
          // Ideally hide sprite, but for now just stand there
        }
        continue; // Skip other AI
      }
    }

    // 2. Stamina Decay
    float decayRate = 0.5f; // Base metabolic rate
    if (c.workState == Citizen::WorkState::Working) {
      decayRate = 2.0f; // Working tires faster
    } else if (c.workState == Citizen::WorkState::GoingToWork ||
               c.workState == Citizen::WorkState::ReturningHome) {
      decayRate = 0.8f; // Walking tires less than working
    }
    c.energy -= deltaTime * decayRate;

    // 3. Go Home if Tired (now probabilistic and influenced by personality)
    // Homeless push themselves harder (need < 10% energy to stop)
    float restThreshold = (c.homeID != -1) ? 20.0f : 10.0f;
    float chanceToRest = 0.0f;
    if (c.energy < restThreshold) {
        // How tired are we? (0 to 1)
        float tiredness = (restThreshold - c.energy) / restThreshold;
        // Base chance is tiredness, then adjust by personality
        switch (c.personality) {
            case PersonalityTrait::Lazy:
                chanceToRest = tiredness * 1.5f;
                break;
            case PersonalityTrait::Coward:
                chanceToRest = tiredness * 1.2f;
                break;
            case PersonalityTrait::Hardworking:
                chanceToRest = tiredness * 0.5f;
                break;
            case PersonalityTrait::Brave:
                chanceToRest = tiredness * 0.8f;
                break;
            case PersonalityTrait::Normal:
            default:
                chanceToRest = tiredness;
                break;
        }
        // Clamp to 1.0
        if (chanceToRest > 1.0f) chanceToRest = 1.0f;
    }

    if (chanceToRest > 0.0f && GRandom.Chance(chanceToRest * 100.0f) && !c.isGoingHome && !c.isResting) {
      // Stop working immediately
      c.workState = Citizen::WorkState::Idle;
      c.isWorking = false;
      c.stateTimer = 0.0f;
      if (myEntity)
        myEntity->hasTarget = false;

      if (c.homeID != -1) {
        c.isGoingHome = true;
      } else {
        // Homeless? Just rest here immediately
        c.isResting = true;
        TraceLog(LOG_INFO,
                 "STAMINA: Citizen %d homeless and tired -> Resting on ground.",
                 c.id);
      }
    }

    // 4. Executing "Go Home"
    if (c.isGoingHome) {
      Building *home = GetBuilding(c.homeID);
      if (home) {
        // Teleport if stuck too long going home
        if (c.stateTimer > 60.0f && myEntity) {
          myEntity->position = {static_cast<float>(home->tileX),
                                static_cast<float>(home->tileY)};
        }

        float dist = 9999.0f;
        if (myEntity) {
          dist = std::hypot(myEntity->position.x - home->tileX,
                            myEntity->position.y - home->tileY);
        }

        if (dist < 1.5f) {
          // Arrived
          c.isResting = true;
          c.isGoingHome = false;
        } else if (myEntity) {
          // Move to home
          myEntity->targetPos = {static_cast<float>(home->tileX) + 0.5f,
                                 static_cast<float>(home->tileY) + 0.5f};
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Walking;

          c.stateTimer += deltaTime;
        }
        continue; // Skip other AI while going home
      } else {
        // Home destroyed?
        c.homeID = -1;
        c.isGoingHome = false;
      }
    }

    // === STUCK DETECTION ===
    // Track time in current state and reset if stuck
    if (c.workState != c.lastWorkState) {
      c.stateTimer = 0.0f;
      c.lastWorkState = c.workState;
    } else {
      c.stateTimer += deltaTime;
    }

    // Timeout for GoingToWork (stuck pathfinding or unreachable target)
    if (c.workState == Citizen::WorkState::GoingToWork &&
        c.stateTimer > 40.0f) {
      c.workState = Citizen::WorkState::Wandering;
      c.isWorking = false;
      c.stateTimer = 0.0f;
      if (myEntity) {
        myEntity->hasTarget = false;
        myEntity->state = EntityState::Idle;
      }
      TraceLog(LOG_INFO,
               "SIMULATION: Citizen %d stuck GoingToWork (>40s) - WANDERING",
               c.id);
    }
    // Timeout for Working (stuck animation or missing logic)
    else if (c.workState == Citizen::WorkState::Working &&
             c.stateTimer > 60.0f) {
      c.workState = Citizen::WorkState::Wandering;
      c.isWorking = false;
      c.stateTimer = 0.0f;
      if (myEntity) {
        myEntity->hasTarget = false;
        myEntity->state = EntityState::Idle;
      }
      TraceLog(LOG_INFO,
               "SIMULATION: Citizen %d stuck Working (>60s) - WANDERING", c.id);
    }
    // Timeout for ReturningHome
    else if (c.workState == Citizen::WorkState::ReturningHome &&
             c.stateTimer > 40.0f) {
      c.workState = Citizen::WorkState::Wandering;
      c.isWorking = false;
      c.stateTimer = 0.0f;
      if (myEntity) {
        myEntity->hasTarget = false;
        myEntity->state = EntityState::Idle;
      }
      TraceLog(LOG_INFO,
               "SIMULATION: Citizen %d stuck ReturningHome (>40s) - WANDERING",
               c.id);
    }

    if (!myEntity)
      continue; // CRITICAL FIX: Prevent massive crashes when iterating over
                // entities inside Jobs

    // === GENERIC IDLE STATES (SITTING/OBSERVING/SOCIALIZING) ===
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

    // === GENERIC WANDERING ===
    // If wandering, pick a random spot and move there
    if (c.workState == Citizen::WorkState::Wandering) {
      if (!myEntity->hasTarget) {
        int rx = GRandom.Int(-5, 5);
        int ry = GRandom.Int(-5, 5);
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
          // Invalid target, try again next frame or go idle
          c.workState = Citizen::WorkState::Idle;
        }
      }

      // Check if reached wander target or timeout (stuck against wall)
      float dist = std::hypot(myEntity->position.x - myEntity->targetPos.x,
                              myEntity->position.y - myEntity->targetPos.y);
      if (dist < 0.5f || !myEntity->hasTarget || c.stateTimer > 10.0f) {
        c.workState = Citizen::WorkState::Idle;
        c.stateTimer = 0.0f; // Reset timer for the next state
        myEntity->hasTarget = false;
        myEntity->state = EntityState::Idle;
      }
    }

    // === SETTLER AI ===
    // Homeless adults periodically check if they should join or found a city

    // === HUNTING AI (any profession, triggered by hunger - now probabilistic) ===
    // Base hunger chance increases as hunger grows
    float baseHuntChance = 0.0f;
    if (c.hunger > 30.0f) { // Start considering hunt at 30 hunger
        baseHuntChance = (c.hunger - 30.0f) * 0.015f; // 0% at 30, 100% at 96+ hunger
        if (baseHuntChance > 1.0f) baseHuntChance = 1.0f;
    }

    // Modify chance by personality
    float huntChance = baseHuntChance;
    switch (c.personality) {
        case PersonalityTrait::Brave:
            huntChance *= 1.3f; // Braver citizens hunt more eagerly
            break;
        case PersonalityTrait::Hardworking:
            huntChance *= 1.1f; // Hardworking citizens slightly more likely to hunt
            break;
        case PersonalityTrait::Lazy:
            huntChance *= 0.7f; // Lazy citizens hunt less
            break;
        case PersonalityTrait::Coward:
            huntChance *= 0.5f; // Cowards avoid hunting
            break;
        case PersonalityTrait::Normal:
        default:
            // Normal personality uses base chance
            break;
    }

    // Clamp chance
    if (huntChance > 1.0f) huntChance = 1.0f;

    if (huntChance > 0.0f && GRandom.Chance(huntChance * 100.0f) && c.isAdult() && myEntity &&
        c.workState != Citizen::WorkState::Hunting) {
      // Check if city has low food or citizen is cityless
      bool needsHunting = (c.cityID < 0);
      if (!needsHunting && c.cityID >= 0) {
        City *myCity = GetCity(c.cityID);
        if (myCity && myCity->resources.food < 10) {
          needsHunting = true;
        }
      }
      if (needsHunting) {
        // Scan for nearest animal
        float bestDist = 999999.0f;
        int bestAnimalEntityID = -1;
        auto nearbyAnimals = world.GetEntitiesInRadius(myEntity->position, 30.0f);

        for (Entity *ae : nearbyAnimals) {
          // Only hunt non-hostile animals (not Boars or Humans)
          bool isHuntable =
              (ae->type == EntityType::Cow || ae->type == EntityType::Chicken ||
               ae->type == EntityType::Sheep || ae->type == EntityType::Bull ||
               ae->type == EntityType::Chicken2 || ae->type == EntityType::Lamb ||
               ae->type == EntityType::Pig || ae->type == EntityType::Turkey);
          if (!isHuntable)
            continue;
          float d = std::hypot(myEntity->position.x - ae->position.x,
                               myEntity->position.y - ae->position.y);
          if (d < bestDist) {
            bestDist = d;
            bestAnimalEntityID = ae->id;
          }
        }
        if (bestAnimalEntityID >= 0) {
          Entity *prey = world.GetEntityByID(bestAnimalEntityID);
          // Enter hunting mode
          c.workState = Citizen::WorkState::Hunting;
          c.isWorking = true;
          c.workTimer = 0.0f;
          c.targetEntityID = bestAnimalEntityID;
          myEntity->targetPos = prey->position;
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Run;
          TraceLog(LOG_INFO,
                   "HUNT: Citizen %d started hunting animal (entity %d)", c.id,
                   c.targetEntityID);
        }
      }
    }

    // === HUNTING STATE MACHINE ===
    if (c.workState == Citizen::WorkState::Hunting && myEntity) {
      // Find the target animal entity by ID
      Entity *prey = world.GetEntityByID(c.targetEntityID);
      if (prey && (prey->health <= 0 || prey->state == EntityState::Die)) {
        prey = nullptr;
      }
      if (!prey) {
        // Prey died or disappeared
        c.workState = Citizen::WorkState::Idle;
        c.isWorking = false;
        myEntity->hasTarget = false;
        myEntity->state = EntityState::Idle;
      } else {
        float dist = std::hypot(myEntity->position.x - prey->position.x,
                                myEntity->position.y - prey->position.y);
        if (dist < 1.2f) {
          // Attack prey
          c.workTimer += deltaTime;
          myEntity->state = EntityState::Attack;
          myEntity->hasTarget = false;
          // Attack once per second
          if (c.workTimer >= 1.0f) {
            c.workTimer = 0.0f;
            float damage =
                5.0f + c.stats.strength; // Nerfed from 10.0f for balance
            prey->health -= damage;
            prey->state = EntityState::Hurt;
            prey->animTime = 0.0f;
            prey->currentFrame = 0;
            TraceLog(LOG_INFO,
                     "HUNT: Citizen %d hit animal for %.0f dmg (HP: %.0f)",
                     c.id, damage, prey->health);
            if (prey->health <= 0) {
              prey->state = EntityState::Die;
              // Harvest food from kill
              bool isLarge = (prey->type == EntityType::Cow ||
                              prey->type == EntityType::Bull);
              int foodGain = isLarge ? 5 : 3;
              c.hunger -= (float)foodGain * 8.0f;
              if (c.hunger < 0.0f)
                c.hunger = 0.0f;
              // Deposit food to city
              if (c.cityID >= 0) {
                City *myCity = GetCity(c.cityID);
                if (myCity)
                  myCity->resources.food += foodGain;
              }
              c.experience += 20.0f;
              c.skillCombat += 0.5f;
              TraceLog(LOG_INFO,
                       "HUNT: Citizen %d killed animal, gained %d food", c.id,
                       foodGain);
              c.workState = Citizen::WorkState::Idle;
              c.isWorking = false;
              myEntity->state = EntityState::Idle;
            }
          }
        } else {
          // Chase prey
          myEntity->targetPos = prey->position;
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Run;
        }
      }
      continue; // Skip profession AI while hunting
    }

    if (doSettlerCheck && c.cityID == -1 && c.isAdult()) {
      int tx = static_cast<int>(myEntity->position.x);
      int ty = static_cast<int>(myEntity->position.y);

      // FIRST: Check if near an existing city - join it instead of founding
      int nearestCityID = -1;
      float nearestDist = 999999.0f;

      for (const auto &cityPair : cities) {
        const City &city = cityPair.second;
        if (!city.isAlive || !city.HasCapacity())
          continue;

        float dist = std::hypot(city.center.x - tx, city.center.y - ty);
        if (dist < nearestDist) {
          nearestDist = dist;
          nearestCityID = cityPair.first;
        }
      }

      // If close to a city (within 10 tiles), join it
      if (nearestCityID >= 0 && nearestDist <= 10.0f) {
        c.cityID = nearestCityID;
        cities[nearestCityID].citizenIDs.push_back(c.id);
        TraceLog(LOG_INFO, "SIMULATION: Citizen %d joined City %d (dist: %.1f)",
                 c.id, nearestCityID, nearestDist);
      }
      // ELSE: If far from cities, try to found a new one
      else if (nearestDist > 15.0f || nearestCityID < 0) {
        float score = ScoreTileForCity(world, tx, ty);

        // If score is good enough, found a city!
        if (score >= 50.0f) {
          int newCityID = FoundCity(world, c.id, tx, ty);
          c.cityID = newCityID; // Explicitly update current reference
          TraceLog(LOG_INFO,
                   "SIMULATION: Citizen %d founded city %d at (%d, %d) with "
                   "score %.1f!",
                   c.id, newCityID, tx, ty, score);
        }
      }
      break;
    }

    // === JOB AI: LUMBERJACK ===
    // Lumberjacks search for trees, chop them, and return wood to city
    if (c.profession == Profession::Lumberjack && c.cityID >= 0 &&
        c.isAdult()) {
      City *myCity = GetCity(c.cityID);
      if (!myCity)
        continue;

      switch (c.workState) {
      case Citizen::WorkState::Idle: {
        // Check if city storage is full
        if (myCity->resources.wood >= myCity->maxStorage) {
          c.workState = Citizen::WorkState::Wandering;
          break;
        }

        // Check if we are carrying resources to deposit
        if (c.carryingResource > 0) {
          c.workState = Citizen::WorkState::ReturningHome;
          // Find nearest building to deposit
          Vector2 depositTarget = myCity->center;
          float bestDist = 999999.0f;
          for (const auto &b : myCity->buildings) {
            float d = std::hypot(myEntity->position.x - b.tileX,
                                 myEntity->position.y - b.tileY);
            if (d < bestDist) {
              bestDist = d;
              depositTarget = {(float)b.tileX + 0.5f, (float)b.tileY + 0.5f};
            }
          }
          myEntity->targetPos = depositTarget;
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Walking;
          break;
        }

        // Find nearest tree - search in radius around citizen (can leave
        // territory)
        int bestTileX = -1, bestTileY = -1;
        float bestDist = 999999.0f;
        const int SEARCH_RADIUS =
            50; // Search 50 tiles around position (can go far!)

        int centerX = static_cast<int>(myEntity->position.x);
        int centerY = static_cast<int>(myEntity->position.y);

        for (int dy = -SEARCH_RADIUS; dy <= SEARCH_RADIUS; dy++) {
          for (int dx = -SEARCH_RADIUS; dx <= SEARCH_RADIUS; dx++) {
            int tx = centerX + dx;
            int ty = centerY + dy;

            // Bounds check
            if (tx < 0 || ty < 0 || tx >= world.GetWidth() ||
                ty >= world.GetHeight())
              continue;

            const Tile &t = world.GetTileConst(tx, ty);

            // Check if this tile has a tree decoration
            if (t.decoration == DecorationType::Tree ||
                t.decoration == DecorationType::PineTree ||
                t.decoration == DecorationType::PalmTree) {
              float dist = std::hypot(myEntity->position.x - tx,
                                      myEntity->position.y - ty);
              if (dist < bestDist) {
                bestDist = dist;
                bestTileX = tx;
                bestTileY = ty;
              }
            }
          }
        }

        if (bestTileX >= 0) {
          // Found a tree! Start moving towards it
          c.targetTileX = bestTileX;
          c.targetTileY = bestTileY;
          c.workState = Citizen::WorkState::GoingToWork;
          c.isWorking = true;

          // Set entity target
          myEntity->targetPos = {bestTileX + 0.5f, bestTileY + 0.5f};
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Walking;

          TraceLog(LOG_INFO,
                   "LUMBERJACK: Citizen %d going to tree at (%d,%d), dist=%.1f",
                   c.id, bestTileX, bestTileY, bestDist);
        } else {
          // No tree found - log this for debugging
          TraceLog(LOG_INFO,
                   "LUMBERJACK: Citizen %d at (%.1f,%.1f) found NO trees in "
                   "radius %d",
                   c.id, myEntity->position.x, myEntity->position.y,
                   SEARCH_RADIUS);
        }
        break;
      }

      case Citizen::WorkState::GoingToWork: {
        // Check if we reached the target
        float distToTarget =
            std::hypot(myEntity->position.x - (c.targetTileX + 0.5f),
                       myEntity->position.y - (c.targetTileY + 0.5f));

        if (distToTarget < 1.5f) {
          // Arrived at tree! Start chopping
          c.workState = Citizen::WorkState::Working;
          c.workTimer = 0.0f;
          myEntity->state = EntityState::Attack; // Use attack anim for chopping
          myEntity->hasTarget = false;
        }
        break;
      }

      case Citizen::WorkState::Working: {
        // Chopping the tree
        c.workTimer += deltaTime;

        // Chopping takes 5 seconds (faster with skill)
        float chopTime = 5.0f - (c.skillWoodcutting * 0.03f);
        if (chopTime < 2.0f)
          chopTime = 2.0f;

        if (c.workTimer >= chopTime) {
          // Tree chopped! Remove decoration and get wood
          Tile &tile = world.GetTile(c.targetTileX, c.targetTileY);
          tile.originalTree = tile.decoration; // Remember what tree was here
          tile.decoration = DecorationType::None;
          tile.hasStump = true; // Mark for visual stump + reforestation
          tile.stumpVariant = GRandom.Int(0, 1); // Random stump art (0 or 1)
          tile.regrowthTimer = 0.0f;      // Start regrowth countdown

          c.carryingResource += 3;    // Each tree gives 3 wood
          c.skillWoodcutting += 0.5f; // Gain skill
          if (c.skillWoodcutting > 100.0f)
            c.skillWoodcutting = 100.0f;
          c.experience += 15.0f; // XP for chopping a tree

          TraceLog(LOG_INFO,
                   "LUMBERJACK: Citizen %d chopped tree at (%d,%d), carrying "
                   "%d wood",
                   c.id, c.targetTileX, c.targetTileY, c.carryingResource);

          // If carrying max, return home. Otherwise find another tree.
          if (c.carryingResource >= c.maxCarryCapacity) {
            c.workState = Citizen::WorkState::ReturningHome;
            myEntity->targetPos = myCity->center;
            myEntity->hasTarget = true;
            myEntity->state = EntityState::Walking;
          } else {
            c.workState = Citizen::WorkState::Idle; // Look for another tree
          }
        }
        break;
      }

      case Citizen::WorkState::ReturningHome: {
        // Deposit wood when near any building in city or near center
        bool deposited = false;

        // Check buildings first
        for (const auto &b : myCity->buildings) {
          float distToBld = std::hypot(myEntity->position.x - b.tileX,
                                       myEntity->position.y - b.tileY);
          if (distToBld < 3.0f) {
            myCity->resources.wood += c.carryingResource;
            TraceLog(LOG_INFO,
                     "LUMBERJACK: Citizen %d deposited %d wood. City now has "
                     "%d wood.",
                     c.id, c.carryingResource, myCity->resources.wood);
            c.carryingResource = 0;
            c.workState = Citizen::WorkState::Idle;
            c.isWorking = false;
            myEntity->state = EntityState::Idle;
            deposited = true;
            break;
          }
        }

        if (!deposited) {
          // Check if we reached the city center
          float distToCity =
              std::hypot(myEntity->position.x - myCity->center.x,
                         myEntity->position.y - myCity->center.y);

          if (distToCity < 3.0f) {
            // Deposit resources
            myCity->resources.wood += c.carryingResource;
            TraceLog(LOG_INFO,
                     "LUMBERJACK: Citizen %d deposited %d wood. City now has "
                     "%d wood.",
                     c.id, c.carryingResource, myCity->resources.wood);
            c.carryingResource = 0;
            c.workState = Citizen::WorkState::Idle;
            c.isWorking = false;
            myEntity->state = EntityState::Idle;
          }
        }
        break;
      }

      default:
        if (c.workState == Citizen::WorkState::Idle)
          c.workState = Citizen::WorkState::Wandering;
        break;
      }
    }

    // === JOB AI: FARMER ===
    // Farmers plant crops, wait for growth, and harvest food
    if (c.profession == Profession::Farmer && c.cityID >= 0 && c.isAdult()) {
      City *myCity = GetCity(c.cityID);
      if (!myCity)
        continue;

      switch (c.workState) {
      case Citizen::WorkState::Idle: {
        // Priority 1: Check for mature crops to harvest
        std::vector<std::pair<float, std::pair<int, int>>> harvestCandidates;

        // Priority 2: Find a fertile tile to plant
        std::vector<std::pair<float, std::pair<int, int>>> plantCandidates;

        // Priority 3: Plant Trees (Reforestation) - Only if no food work
        int treePlantX = -1, treePlantY = -1;
        std::vector<std::pair<int, int>> treeCandidates;

        for (const Vector2 &tile : myCity->territory) {
          int tx = static_cast<int>(tile.x);
          int ty = static_cast<int>(tile.y);
          Tile &t = world.GetTile(tx, ty);

          // Check if planted and ready to harvest
          if (t.isPlanted && t.growthProgress >= 100.0f &&
              t.farmOwnerCityID == myCity->id) {
            float dist = std::hypot(myEntity->position.x - tx,
                                    myEntity->position.y - ty);
            harvestCandidates.push_back({dist, {tx, ty}});
          }

          // Check if can plant CROPS (grass tile, not planted, no decoration)
          bool needsFood = myCity->resources.food < (myCity->maxStorage * 0.9f);
          bool canPlantCrop = needsFood && !t.isPlanted &&
                              t.type == TileType::Grass &&
                              t.decoration == DecorationType::None;

          if (canPlantCrop) {
            float dist = std::hypot(myEntity->position.x - tx,
                                    myEntity->position.y - ty);
            plantCandidates.push_back({dist, {tx, ty}});
          }
        }

        // Sort by distance and pick randomly from top 5 closest
        // This distributes farmers across territory
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

        // Priority 3: Plant Trees (Reforestation)
        // Only 30% chance per idle cycle to attempt tree planting (reduce spam)
        if (harvestX == -1 && plantX == -1 && GRandom.Chance(30)) {
          int SEARCH_RADIUS = 50;
          int cx = static_cast<int>(myEntity->position.x);
          int cy = static_cast<int>(myEntity->position.y);

          // Find city center for distance check
          float cityCenterX = 0, cityCenterY = 0;
          if (myCity && !myCity->territory.empty()) {
            for (const auto &tt : myCity->territory) {
              cityCenterX += tt.x;
              cityCenterY += tt.y;
            }
            cityCenterX /= myCity->territory.size();
            cityCenterY /= myCity->territory.size();
          }

          for (int dy = -SEARCH_RADIUS; dy <= SEARCH_RADIUS; dy++) {
            for (int dx = -SEARCH_RADIUS; dx <= SEARCH_RADIUS; dx++) {
              int tx = cx + dx;
              int ty = cy + dy;

              if (tx < 0 || ty < 0 || tx >= world.GetWidth() ||
                  ty >= world.GetHeight())
                continue;

              Tile &t = world.GetTile(tx, ty);

              // NEW RULES:
              // - Must be grass tile
              // - No decoration, no stump, not planted
              // - NOT inside any city territory (ownerCityID == -1)
              // - At least 15 tiles from city center
              float distToCity = std::hypot(tx - cityCenterX, ty - cityCenterY);
              bool canPlantTree =
                  !t.isPlanted && t.decoration == DecorationType::None &&
                  !t.hasStump && !t.isOccupied && t.ownerCityID == -1 &&
                  t.type == TileType::Grass && distToCity >= 15.0f;

              if (canPlantTree) {
                treeCandidates.push_back({tx, ty});
              }
            }
          }
          // Pick a random candidate from the list for organic placement
          if (!treeCandidates.empty()) {
            int idx = GRandom.Int(0, (int)treeCandidates.size() - 1);
            treePlantX = treeCandidates[idx].first;
            treePlantY = treeCandidates[idx].second;
          }
        }

        // Prioritize harvesting over planting food over planting trees
        if (harvestX >= 0) {
          c.targetTileX = harvestX;
          c.targetTileY = harvestY;
          c.workState = Citizen::WorkState::GoingToWork;
          c.currentJob = Citizen::JobType::Farming;
          c.isWorking = true;
          c.workTimer = 0.0f; // Will use work timer to track if harvesting
          myEntity->targetPos = {harvestX + 0.5f, harvestY + 0.5f};
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Walking;
        } else if (plantX >= 0) {
          c.targetTileX = plantX;
          c.targetTileY = plantY;
          c.workState = Citizen::WorkState::GoingToWork;
          c.currentJob = Citizen::JobType::Farming;
          c.isWorking = true;
          c.workTimer = -1.0f; // Negative = planting mode (legacy/backup check)
          myEntity->targetPos = {plantX + 0.5f, plantY + 0.5f};
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Walking;
        } else if (treePlantX >= 0) {
          c.targetTileX = treePlantX;
          c.targetTileY = treePlantY;
          c.workState = Citizen::WorkState::GoingToWork;
          c.currentJob = Citizen::JobType::PlantingTree;
          c.isWorking = true;
          c.workTimer = 0.0f;
          myEntity->targetPos = {treePlantX + 0.5f, treePlantY + 0.5f};
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Walking;
        } else {
          // No work found -> Wander
          c.workState = Citizen::WorkState::Wandering;
          c.stateTimer = 0.0f;
        }
        break;
      }

      case Citizen::WorkState::GoingToWork: {
        float distToTarget =
            std::hypot(myEntity->position.x - (c.targetTileX + 0.5f),
                       myEntity->position.y - (c.targetTileY + 0.5f));

        if (distToTarget < 1.5f) {
          c.workState = Citizen::WorkState::Working;
          myEntity->state = EntityState::Attack; // Shows farming animation
          myEntity->currentFrame = 0;
          myEntity->hasTarget = false;
          if (c.workTimer < 0) {
            c.workTimer = 0.0f; // Start planting timer
          }
        }
        break;
      }

      case Citizen::WorkState::Working: {
        c.workTimer += deltaTime;
        Tile &tile = world.GetTile(c.targetTileX, c.targetTileY);

        // Planting takes 2 seconds
        if (c.currentJob == Citizen::JobType::PlantingTree) {
          if (c.workTimer >= 2.0f) {
            if (!tile.isOccupied) { // Prevent planting under buildings
              tile.decoration = DecorationType::Tree;
              tile.hasStump = false;               // Reset stump flag
              tile.decorationVariant = GRandom.Int(0, 2); // Random variant

              TraceLog(LOG_INFO, "FARMER: Citizen %d planted a TREE at (%d,%d)",
                       c.id, c.targetTileX, c.targetTileY);
            }

            c.workState = Citizen::WorkState::Idle;
            c.isWorking = false;
            c.currentJob = Citizen::JobType::None;
          }
        } else if (!tile.isPlanted &&
                   c.currentJob == Citizen::JobType::Farming) {
          if (c.workTimer >= 2.0f) {
            tile.isPlanted = true;
            tile.growthProgress = 0.0f;
            tile.farmOwnerCityID = myCity->id;

            c.skillFarming += 0.3f;
            if (c.skillFarming > 100.0f)
              c.skillFarming = 100.0f;
            c.experience += 8.0f; // XP for planting crops

            TraceLog(LOG_INFO, "FARMER: Citizen %d planted crops at (%d,%d)",
                     c.id, c.targetTileX, c.targetTileY);

            c.workState = Citizen::WorkState::Idle;
            c.isWorking = false;
            c.currentJob = Citizen::JobType::None;
          }
        }
        // Harvesting takes 1.5 seconds
        else if (tile.growthProgress >= 100.0f) {
          if (c.workTimer >= 1.5f) {
            int harvestAmount = 3 + static_cast<int>(c.skillFarming * 0.05f);
            c.carryingResource += harvestAmount;

            tile.isPlanted = false;
            tile.growthProgress = 0.0f;
            tile.farmOwnerCityID = -1;

            c.skillFarming += 0.5f;
            if (c.skillFarming > 100.0f)
              c.skillFarming = 100.0f;
            c.experience += 12.0f; // XP for harvesting

            TraceLog(
                LOG_INFO,
                "FARMER: Citizen %d harvested %d food at (%d,%d), carrying %d",
                c.id, harvestAmount, c.targetTileX, c.targetTileY,
                c.carryingResource);

            // Return home if carrying enough, or continue farming
            if (c.carryingResource >= c.maxCarryCapacity) {
              c.workState = Citizen::WorkState::ReturningHome;
              // Walk to nearest building instead of center
              Vector2 depositTarget = myCity->center;
              float bestDist = 999999.0f;
              for (const auto &b : myCity->buildings) {
                float d = std::hypot(myEntity->position.x - b.tileX,
                                     myEntity->position.y - b.tileY);
                if (d < bestDist) {
                  bestDist = d;
                  depositTarget = {(float)b.tileX + 0.5f,
                                   (float)b.tileY + 0.5f};
                }
              }
              myEntity->targetPos = depositTarget;
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
        // Deposit food when near any building in city or near center
        bool deposited = false;
        for (const auto &b : myCity->buildings) {
          float distToBld = std::hypot(myEntity->position.x - b.tileX,
                                       myEntity->position.y - b.tileY);
          if (distToBld < 3.0f) {
            myCity->resources.food += c.carryingResource;
            TraceLog(
                LOG_INFO,
                "FARMER: Citizen %d deposited %d food. City now has %d food.",
                c.id, c.carryingResource, myCity->resources.food);
            c.carryingResource = 0;
            c.workState = Citizen::WorkState::Idle;
            c.isWorking = false;
            myEntity->state = EntityState::Idle;
            deposited = true;
            break;
          }
        }
        // Fallback: deposit at city center
        if (!deposited) {
          float distToCity =
              std::hypot(myEntity->position.x - myCity->center.x,
                         myEntity->position.y - myCity->center.y);
          if (distToCity < 3.0f) {
            myCity->resources.food += c.carryingResource;
            TraceLog(
                LOG_INFO,
                "FARMER: Citizen %d deposited %d food. City now has %d food.",
                c.id, c.carryingResource, myCity->resources.food);
            c.carryingResource = 0;
            c.workState = Citizen::WorkState::Idle;
            c.isWorking = false;
            myEntity->state = EntityState::Idle;
          }
        }
        break;
      }

      case Citizen::WorkState::Wandering: {
        // Farmer has no work — wander around city territory
        c.stateTimer += deltaTime;

        if (!myEntity->hasTarget) {
          // Pick a random tile within city territory to walk to
          if (!myCity->territory.empty()) {
            int idx = GRandom.Int(0, (int)myCity->territory.size() - 1);
            float tx = myCity->territory[idx].x + 0.5f;
            float ty = myCity->territory[idx].y + 0.5f;
            if (world.IsWalkable((int)tx, (int)ty)) {
              myEntity->targetPos = {tx, ty};
              myEntity->hasTarget = true;
              myEntity->state = EntityState::Walking;
            }
          }
        }

        // Re-check for work every 8 seconds
        if (c.stateTimer >= 8.0f) {
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
      if (!myCity)
        continue;

      switch (c.workState) {
      case Citizen::WorkState::Idle: {

        // Check if city stone storage is full
        if (myCity->resources.stone >= myCity->maxStorage) {
          c.workState = Citizen::WorkState::Wandering;
          break;
        }

        // Scan for rocks
        int bestX = -1, bestY = -1;
        float bestDist = 999999.0f;
        int range = 60; // Increased range
        int cx = (int)myEntity->position.x, cy = (int)myEntity->position.y;

        for (int dy = -range; dy <= range; dy++) {
          for (int dx = -range; dx <= range; dx++) {
            int tx = cx + dx;
            int ty = cy + dy;
            if (tx < 0 || ty < 0 || tx >= world.GetWidth() ||
                ty >= world.GetHeight())
              continue;

            const Tile &t = world.GetTileConst(tx, ty);
            bool isRock = (t.decoration == DecorationType::Rock ||
                           t.decoration == DecorationType::SmallRock ||
                           t.decoration == DecorationType::MediumRock ||
                           t.decoration == DecorationType::BigRock ||
                           t.decoration == DecorationType::Crystal);

            if (isRock || t.type == TileType::Mountain) {
              float dist = std::hypot(myEntity->position.x - tx,
                                      myEntity->position.y - ty);
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
          TraceLog(LOG_INFO, "MINER: Citizen %d going to mine at (%d,%d)", c.id,
                   bestX, bestY);
        } else {
          c.workState = Citizen::WorkState::Wandering;
        }
        break;
      }
      case Citizen::WorkState::GoingToWork: {
        float dist = std::hypot(myEntity->position.x - (c.targetTileX + 0.5f),
                                myEntity->position.y - (c.targetTileY + 0.5f));
        if (dist < 1.5f) {
          c.workState = Citizen::WorkState::Working;
          c.workTimer = 0.0f;
          myEntity->state = EntityState::Attack; // Mining animation
          myEntity->hasTarget = false;
        }
        break;
      }
      case Citizen::WorkState::Working: {
        c.workTimer += deltaTime;
        if (c.workTimer >= 4.0f) { // Mining takes 4s (hit)

          c.workTimer = 0.0f; // Reset timer for next hit if not full

          Tile &t = world.GetTile(c.targetTileX, c.targetTileY);

          bool rockExists = (t.decoration != DecorationType::None &&
                             t.type != TileType::Mountain);

          if (rockExists) {
            if (t.resourceAmount > 0) {
              t.resourceAmount -= 1.0f;
              c.carryingResource += 1;
              c.skillMining += 0.2f;
              c.experience += 5.0f; // XP for mining stone

              if (t.resourceAmount <= 0) {
                t.decoration = DecorationType::None; // Destroyed
              }
            } else {
              // Fallback for old rocks without resource amount
              t.decoration = DecorationType::None;
              c.carryingResource += 1;
            }
          } else if (t.type == TileType::Mountain) {
            // Infinite mining on mountain tiles?
            c.carryingResource += 1;
            c.skillMining += 0.1f;
            c.experience += 5.0f; // XP for mining mountain
          } else {
            // Rock disappeared
            c.workState = Citizen::WorkState::Idle;
            myEntity->state = EntityState::Idle;
            break;
          }

          if (c.skillMining > 100.0f)
            c.skillMining = 100.0f;

          // Visual feedback (particle?) in future

          TraceLog(LOG_INFO, "MINER: Citizen %d mined stone (Carrying: %d/%d)",
                   c.id, c.carryingResource, c.maxCarryCapacity);

          if (c.carryingResource >= c.maxCarryCapacity) {
            c.workState = Citizen::WorkState::ReturningHome;
            myEntity->targetPos = myCity->center;
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
          if (myCity->resources.stone > myCity->maxStorage)
            myCity->resources.stone = myCity->maxStorage;

          // Small chance for ore/gold?
          if (GRandom.Chance(10))
            myCity->resources.ore += 1;

          TraceLog(LOG_INFO, "MINER: Citizen %d deposited %d stone.", c.id,
                   c.carryingResource);
          c.carryingResource = 0;
          c.workState = Citizen::WorkState::Idle;
          c.isWorking = false;
          myEntity->state = EntityState::Idle;
        }
        break;
      }
      default:
        if (c.workState == Citizen::WorkState::Idle)
          c.workState = Citizen::WorkState::Wandering;
        break;
      }
    }

    // === JOB AI: BUILDER ===
    if (c.profession == Profession::Builder && c.cityID >= 0 && c.isAdult()) {
      City *myCity = GetCity(c.cityID);
      if (!myCity)
        continue;

      switch (c.workState) {
      case Citizen::WorkState::Idle: {
        // Scan for incomplete buildings
        int bestIdx = -1;
        float bestDist = 999999.0f;

        for (size_t i = 0; i < myCity->buildings.size(); i++) {
          if (!myCity->buildings[i].isComplete) {
            float dist =
                std::hypot(myEntity->position.x - myCity->buildings[i].tileX,
                           myEntity->position.y - myCity->buildings[i].tileY);
            if (dist < bestDist) {
              bestDist = dist;
              bestIdx = (int)i;
            }
          }
        }

        if (bestIdx >= 0) {
          // Find a walkable spot adjacent to the building site
          int bx = myCity->buildings[bestIdx].tileX;
          int by = myCity->buildings[bestIdx].tileY;
          int tx = bx, ty = by;

          // Try 4 directions
          if (world.IsWalkable(bx + 1, by) &&
              !world.GetTileConst(bx + 1, by).isOccupied) {
            tx = bx + 1;
          } else if (world.IsWalkable(bx - 1, by) &&
                     !world.GetTileConst(bx - 1, by).isOccupied) {
            tx = bx - 1;
          } else if (world.IsWalkable(bx, by + 1) &&
                     !world.GetTileConst(bx, by + 1).isOccupied) {
            ty = by + 1;
          } else if (world.IsWalkable(bx, by - 1) &&
                     !world.GetTileConst(bx, by - 1).isOccupied) {
            ty = by - 1;
          }

          c.targetTileX = bx; // Job target (building)
          c.targetTileY = by;
          c.workState = Citizen::WorkState::GoingToWork;
          c.isWorking = true;
          myEntity->targetPos = {tx + 0.5f,
                                 ty + 0.5f}; // Move target (next to building)
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Walking;
          TraceLog(LOG_INFO,
                   "BUILDER: Citizen %d going to build at (%d,%d), standing at "
                   "(%d,%d)",
                   c.id, c.targetTileX, c.targetTileY, tx, ty);
        } else {
          c.workState = Citizen::WorkState::Wandering;
        }
        break;
      }
      case Citizen::WorkState::GoingToWork: {
        float dist = std::hypot(myEntity->position.x - (c.targetTileX + 0.5f),
                                myEntity->position.y - (c.targetTileY + 0.5f));
        if (dist < 2.0f) { // Increased distance to 2.0f for adjacent working
          c.workState = Citizen::WorkState::Working;
          c.workTimer = 0.0f;
          myEntity->state = EntityState::Attack; // Building animation
          myEntity->hasTarget = false;
        }
        break;
      }
      case Citizen::WorkState::Working: {
        c.workTimer += deltaTime;
        if (c.workTimer >= 5.0f) { // Building takes 5s per session
          // Find the building again
          for (auto &b : myCity->buildings) {
            if (b.tileX == c.targetTileX && b.tileY == c.targetTileY &&
                !b.isComplete) {
              b.isComplete = true; // For now instant complete after work
              TraceLog(LOG_INFO,
                       "BUILDER: Citizen %d completed building at (%d,%d)",
                       c.id, c.targetTileX, c.targetTileY);
              break;
            }
          }
          c.skillBuilding += 0.5f;
          if (c.skillBuilding > 100.0f)
            c.skillBuilding = 100.0f;

          c.workState = Citizen::WorkState::Idle;
          c.isWorking = false;
          myEntity->state = EntityState::Idle;
        }
        break;
      }
      default:
        if (c.workState == Citizen::WorkState::Idle)
          c.workState = Citizen::WorkState::Wandering;
        break;
      }
    }

    // === JOB AI: SOLDIER ===
    // Soldiers patrol their city and engage enemy soldiers/citizens
    if (c.profession == Profession::Soldier && c.cityID >= 0 && c.isAdult()) {
      // Ensure visual representation updates to Armed
      if (myEntity && myEntity->type != EntityType::HumanArmed) {
        myEntity->type = EntityType::HumanArmed;
      }

      City *myCity = GetCity(c.cityID);
      if (!myCity)
        continue;

      switch (c.workState) {
      case Citizen::WorkState::Idle:
      case Citizen::WorkState::Wandering: {
        // SCAN FOR ENEMIES — only attack citizens from hostile kingdoms
        int enemyID = -1;
        float closestDist = 999999.0f;
        float detectRange = 25.0f;

        // Get my kingdom
        Kingdom *myKingdom = nullptr;
        if (myCity->kingdomID >= 0)
          myKingdom = GetKingdom(myCity->kingdomID);

        auto nearbyEntities = world.GetEntitiesInRadius(myEntity->position, detectRange);
        for (Entity *otherE : nearbyEntities) {
          if (otherE->id == myEntity->id)
            continue;

          if (otherE->IsIntelligent()) {
            Citizen *otherCitizen = GetCitizen(otherE->citizenID);
            if (otherCitizen && otherCitizen->cityID != c.cityID &&
                otherCitizen->cityID != -1) {
              // Check if we are at war with their kingdom
              bool isEnemy = false;
              City *otherCity = GetCity(otherCitizen->cityID);
              if (myKingdom && otherCity && otherCity->kingdomID >= 0) {
                isEnemy = myKingdom->IsAtWarWith(otherCity->kingdomID);
              }
              // Also attack anyone near our city who belongs to another city
              if (!isEnemy && myCity) {
                float distToMyCity =
                    std::hypot(otherE->position.x - myCity->center.x,
                               otherE->position.y - myCity->center.y);
                if (distToMyCity < 20.0f) {
                  isEnemy = true; // Trespasser in our territory
                }
              }

              if (isEnemy) {
                float d = std::hypot(myEntity->position.x - otherE->position.x,
                                     myEntity->position.y - otherE->position.y);
                if (d <= detectRange && d < closestDist) {
                  closestDist = d;
                  enemyID = otherCitizen->id;
                }
              }
            }
          }
        }

        if (enemyID != -1) {
          // Enemy spotted, transition to engage
          c.workState = Citizen::WorkState::GoingToWork;
          c.targetEntityID = enemyID;
          c.isWorking = true;
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Run;
          TraceLog(LOG_INFO, "SOLDIER: Citizen %d locked onto Enemy %d", c.id,
                   enemyID);
        } else if (myKingdom && !myKingdom->activeWarKingdoms.empty()) {
          // AT WAR but no nearby enemies — march toward enemy city
          float bestDist = 999999.0f;
          Vector2 raidTarget = myEntity->position;
          bool foundTarget = false;

          for (int enemyKingID : myKingdom->activeWarKingdoms) {
            Kingdom *enemyKingdom = GetKingdom(enemyKingID);
            if (!enemyKingdom)
              continue;
            for (int eCityID : enemyKingdom->cityIDs) {
              City *eCity = GetCity(eCityID);
              if (!eCity || !eCity->isAlive)
                continue;
              float d = std::hypot(myEntity->position.x - eCity->center.x,
                                   myEntity->position.y - eCity->center.y);
              if (d < bestDist) {
                bestDist = d;
                raidTarget = {eCity->center.x + 0.5f, eCity->center.y + 0.5f};
                foundTarget = true;
              }
            }
          }

          if (foundTarget) {
            myEntity->targetPos = raidTarget;
            myEntity->hasTarget = true;
            myEntity->state = EntityState::Walking;
          }
        } else {
          // Patrol logic: Walk in straight lines through the city streets
          if (!myEntity->hasTarget) {
            // Pick a random cardinal direction
            int r = GRandom.Int(0, 3);
            int dx = 0, dy = 0;
            if (r == 0)
              dx = 1;
            else if (r == 1)
              dx = -1;
            else if (r == 2)
              dy = 1;
            else
              dy = -1;

            int maxDist = GRandom.Int(4, 11); // Patrol 4 to 11 tiles
            int cx = (int)myEntity->position.x;
            int cy = (int)myEntity->position.y;
            int validDist = 0;

            for (int i = 1; i <= maxDist; i++) {
              int checkX = cx + dx * i;
              int checkY = cy + dy * i;

              // Check bounds and walkability
              if (checkX >= 0 && checkX < world.GetWidth() && checkY >= 0 &&
                  checkY < world.GetHeight()) {
                if (world.IsWalkable(checkX, checkY)) {
                  // Stay within own city if possible to defend it
                  if (world.GetTileConst(checkX, checkY).ownerCityID ==
                      c.cityID) {
                    validDist = i;
                  } else {
                    break; // Reached border
                  }
                } else {
                  break; // Hit wall/building
                }
              } else {
                break; // Map edge
              }
            }

            if (validDist > 0) {
              myEntity->targetPos = {(float)(cx + dx * validDist) + 0.5f,
                                     (float)(cy + dy * validDist) + 0.5f};
              myEntity->hasTarget = true;
              myEntity->state = EntityState::Walking;
            } else {
              // Stuck or at border, will pick a new direction next frame
              // Minimal fallback to turn visually
              myEntity->facingDirection = (myEntity->facingDirection + 1) % 4;
            }
          }
        }
        break;
      }

      case Citizen::WorkState::GoingToWork: {
        // Move towards enemy
        Citizen *enemy = GetCitizen(c.targetEntityID);
        if (!enemy || !enemy->isAlive) {
          // Enemy died or vanished
          c.workState = Citizen::WorkState::Idle;
          myEntity->hasTarget = false;
          myEntity->state = EntityState::Idle;
          break;
        }

        // Find enemy entity to get exact position
        Entity *enemyEntity = world.GetEntityByCitizenID(enemy->id);

        if (!enemyEntity) {
          c.workState = Citizen::WorkState::Idle;
          break;
        }

        // Update target position tracking
        myEntity->targetPos = enemyEntity->position;
        myEntity->hasTarget = true;
        myEntity->state = EntityState::Run;

        float dist = std::hypot(myEntity->position.x - enemyEntity->position.x,
                                myEntity->position.y - enemyEntity->position.y);
        if (dist < 1.2f) { // Attack range
          c.workState = Citizen::WorkState::Working;
          c.workTimer = 0.0f;
          myEntity->state = EntityState::Attack;
          myEntity->currentFrame = 0;
          myEntity->hasTarget = false;
        }
        break;
      }

      case Citizen::WorkState::Working: {
        // Attacking the enemy
        Citizen *enemy = GetCitizen(c.targetEntityID);
        if (!enemy || !enemy->isAlive) {
          // Enemy died
          c.workState = Citizen::WorkState::Idle;
          myEntity->state = EntityState::Idle;
          break;
        }

        // Make sure they are still close
        Entity *enemyEntity = world.GetEntityByCitizenID(enemy->id);

        if (!enemyEntity) {
          c.workState = Citizen::WorkState::Idle;
          break;
        }

        float dist = std::hypot(myEntity->position.x - enemyEntity->position.x,
                                myEntity->position.y - enemyEntity->position.y);
        if (dist > 1.8f) {
          // Enemy moved away, chase them again
          c.workState = Citizen::WorkState::GoingToWork;
          break;
        }

        // Deal damage periodically
        c.workTimer += deltaTime;
        if (c.workTimer >= 1.0f) { // 1 hit per second
          c.workTimer = 0.0f;

          // Damage formula
          float damage = 15.0f;
          enemy->health -= damage;
          enemyEntity->state = EntityState::Hurt;
          enemyEntity->animTime = 0.0f;
          enemyEntity->currentFrame = 0;

          TraceLog(
              LOG_INFO,
              "COMBAT: Soldier %d hit Enemy %d for %f damage (Enemy HP: %f)",
              c.id, enemy->id, damage, enemy->health);
        }
        break;
      }

      default:
        c.workState = Citizen::WorkState::Idle;
        break;
      }
    }

    // Fallback for unemployed or stuck Idle
    if (c.workState == Citizen::WorkState::Idle &&
        c.profession == Profession::None) {
      c.workState = Citizen::WorkState::Wandering;
    }

    // Interceptor for rich idle states
    if (c.workState == Citizen::WorkState::Wandering && c.stateTimer == 0.0f) { // Acabou de decidir vagar
        if (GRandom.Chance(25)) { // 25% de chance de fazer um idle rico
            int r = GRandom.Int(0, 2);
            if (r == 0) c.workState = Citizen::WorkState::IdleSitting;
            else if (r == 1) c.workState = Citizen::WorkState::IdleObserving;
            else c.workState = Citizen::WorkState::IdleSocializing;

            c.workTimer = (float)GRandom.Int(3, 6); // Fica de 3 a 6 segundos parado
        }
    }
  }
}
