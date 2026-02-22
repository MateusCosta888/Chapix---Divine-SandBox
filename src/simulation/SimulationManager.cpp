#include "SimulationManager.h"
#include "../world/Entity.h"
#include "../world/Tile.h"
#include "../world/World.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

SimulationManager::SimulationManager() {}

SimulationManager::~SimulationManager() {}

// ============================================================================
// MAIN UPDATE LOOP
// ============================================================================
void SimulationManager::Update(World &world, float deltaTime) {
  gameTime += deltaTime;
  yearTimer += deltaTime;

  // Year cycle
  if (yearTimer >= secondsPerYear) {
    yearTimer -= secondsPerYear;
    currentYear++;
  }

  // Update all subsystems

  // Static flag to run collision rebuild once after load / start
  // This ensures existing buildings in save files get their isOccupied flag set
  static bool hasRebuiltCollision = false;
  if (!hasRebuiltCollision) {
    RebuildOccupationMap(world);
    hasRebuiltCollision = true;
  }

  // === STUMP REGROWTH SYSTEM ===
  // Simple timer: every 5 seconds, scan all stumps and add 5 seconds to their
  // timer
  {
    static float regrowthCheckTimer = 0.0f;
    regrowthCheckTimer += deltaTime;
    float checkInterval = 5.0f; // Check every 5 seconds
    float regrowthTime = 90.0f; // Seconds until stump regrows into tree

    if (regrowthCheckTimer >= checkInterval) {
      regrowthCheckTimer = 0.0f;
      int mapW = world.GetWidth();
      int mapH = world.GetHeight();

      for (int y = 0; y < mapH; y++) {
        for (int x = 0; x < mapW; x++) {
          Tile &tile = world.GetTile(x, y);
          if (tile.hasStump && tile.decoration == DecorationType::None) {
            // Cancel regrowth if someone built over the stump
            if (tile.isOccupied) {
              tile.hasStump = false;
              continue;
            }

            tile.regrowthTimer += checkInterval;
            if (tile.regrowthTimer >= regrowthTime) {
              // Regrow tree! Use the original tree type if saved
              DecorationType treeType = tile.originalTree;
              if (treeType == DecorationType::None) {
                // Fallback: choose tree by biome
                if (tile.biome == BiomeType::Snow)
                  treeType = DecorationType::PineTree;
                else if (tile.biome == BiomeType::Desert)
                  treeType = DecorationType::PalmTree;
                else
                  treeType = DecorationType::Tree;
              }
              tile.decoration = treeType;
              tile.decorationVariant = rand() % 9;
              tile.hasStump = false;
              tile.regrowthTimer = 0.0f;
              tile.originalTree = DecorationType::None;
            }
          }
        }
      }
    }
  }

  UpdateCitizens(world, deltaTime);
  UpdateCities(world, deltaTime);
  UpdateKingdoms(world, deltaTime);
}

void SimulationManager::RebuildOccupationMap(World &world) {
  int count = 0;
  for (const auto &cityPair : cities) {
    const City &city = cityPair.second;
    for (const auto &building : city.buildings) {
      if (building.isComplete || building.constructionProgress >= 0.0f) {
        // Mark tile as occupied
        // Check bounds just in case
        BuildingSize size = GetBuildingSize(building.type);
        for (int dy = 0; dy < size.height; dy++) {
          for (int dx = 0; dx < size.width; dx++) {
            int tx = building.tileX + dx;
            int ty = building.tileY + dy;
            if (tx >= 0 && tx < world.GetWidth() && ty >= 0 &&
                ty < world.GetHeight()) {
              world.GetTile(tx, ty).isOccupied = true;
              count++;
            }
          }
        }
      }
    }
  }
  TraceLog(LOG_INFO,
           "SIMULATION: Rebuilt occupation map. Marked %d tiles as occupied.",
           count);
}

// ============================================================================
// CITIZEN MANAGEMENT
// ============================================================================
int SimulationManager::AddCitizen(const Citizen &citizen) {
  Citizen c = citizen;
  if (c.id < 0)
    c.id = GetNextCitizenID();
  citizens[c.id] = c;
  return c.id;
}

Citizen *SimulationManager::GetCitizen(int id) {
  auto it = citizens.find(id);
  return it != citizens.end() ? &it->second : nullptr;
}

const Citizen *SimulationManager::GetCitizen(int id) const {
  auto it = citizens.find(id);
  return it != citizens.end() ? &it->second : nullptr;
}

void SimulationManager::RemoveCitizen(int id) { citizens.erase(id); }

// ============================================================================
// CITY MANAGEMENT
// ============================================================================
int SimulationManager::AddCity(const City &city) {
  City c = city;
  if (c.id < 0)
    c.id = GetNextCityID();
  cities[c.id] = c;
  return c.id;
}

City *SimulationManager::GetCity(int id) {
  auto it = cities.find(id);
  return it != cities.end() ? &it->second : nullptr;
}

const City *SimulationManager::GetCity(int id) const {
  auto it = cities.find(id);
  return it != cities.end() ? &it->second : nullptr;
}

void SimulationManager::RemoveCity(int id) { cities.erase(id); }

// ============================================================================
// KINGDOM MANAGEMENT
// ============================================================================
int SimulationManager::AddKingdom(const Kingdom &kingdom) {
  Kingdom k = kingdom;
  if (k.id < 0)
    k.id = GetNextKingdomID();
  kingdoms[k.id] = k;
  return k.id;
}

Kingdom *SimulationManager::GetKingdom(int id) {
  auto it = kingdoms.find(id);
  return it != kingdoms.end() ? &it->second : nullptr;
}

const Kingdom *SimulationManager::GetKingdom(int id) const {
  auto it = kingdoms.find(id);
  return it != kingdoms.end() ? &it->second : nullptr;
}

void SimulationManager::RemoveKingdom(int id) { kingdoms.erase(id); }

// ============================================================================
// BUILDING MANAGEMENT
// ============================================================================
int SimulationManager::AddBuilding(const Building &building) {
  Building b = building;
  if (b.id < 0)
    b.id = GetNextBuildingID();
  buildings[b.id] = b;
  return b.id;
}

Building *SimulationManager::GetBuilding(int id) {
  auto it = buildings.find(id);
  return it != buildings.end() ? &it->second : nullptr;
}

const Building *SimulationManager::GetBuilding(int id) const {
  auto it = buildings.find(id);
  return it != buildings.end() ? &it->second : nullptr;
}

void SimulationManager::RemoveBuilding(int id) { buildings.erase(id); }

// ============================================================================
// CITY FOUNDING - SCORE ALGORITHM
// ============================================================================
float SimulationManager::ScoreTileForCity(World &world, int x, int y) const {
  float score = 0.0f;

  // Base score: Must be walkable land
  if (!world.IsWalkable(x, y))
    return -1000.0f;

  // Check if already part of a city territory
  for (const auto &pair : cities) {
    for (const auto &tile : pair.second.territory) {
      if (static_cast<int>(tile.x) == x && static_cast<int>(tile.y) == y) {
        return -1000.0f; // Already claimed
      }
    }
  }

  // Check surrounding area (5x5)
  int waterCount = 0;
  int treeCount = 0;
  int grassCount = 0;
  int oreCount = 0;

  for (int dy = -2; dy <= 2; dy++) {
    for (int dx = -2; dx <= 2; dx++) {
      int nx = x + dx;
      int ny = y + dy;

      if (nx < 0 || ny < 0 || nx >= world.GetWidth() || ny >= world.GetHeight())
        continue;

      Tile &tile = world.GetTile(nx, ny);

      // Water nearby is good
      if (tile.type == TileType::ShallowOcean || tile.type == TileType::Ocean)
        waterCount++;

      // Trees are resources
      if (tile.decoration == DecorationType::Tree)
        treeCount++;

      // Grass is fertile
      if (tile.type == TileType::Grass)
        grassCount++;
    }
  }

  // Score calculation
  // Water nearby is no longer required
  score += treeCount * 5.0f;  // Trees for wood
  score += grassCount * 3.0f; // Fertile land

  // Penalty for being too close to other cities
  for (const auto &pair : cities) {
    float dist = std::hypot(pair.second.center.x - x, pair.second.center.y - y);
    if (dist < 15.0f) {
      score -= (15.0f - dist) * 20.0f; // Heavy penalty for proximity
    }
  }

  return score;
}

// ============================================================================
// CITY FOUNDING
// ============================================================================
int SimulationManager::FoundCity(World &world, int founderCitizenID, int x,
                                 int y) {
  // Create the city
  Color cityColor = {static_cast<unsigned char>(50 + (nextCityID * 37) % 156),
                     static_cast<unsigned char>(50 + (nextCityID * 73) % 156),
                     static_cast<unsigned char>(50 + (nextCityID * 113) % 156),
                     255};

  City city = CreateCity(-1, {static_cast<float>(x), static_cast<float>(y)},
                         "Settlement " + std::to_string(nextCityID), cityColor);
  int cityID = AddCity(city);

  // Assign founder to city
  Citizen *founder = GetCitizen(founderCitizenID);
  if (founder) {
    founder->cityID = cityID;
    founder->profession = Profession::Leader;
    cities[cityID].citizenIDs.push_back(founderCitizenID);
  }

  // Claim initial territory (radius 3)
  for (int dy = -3; dy <= 3; dy++) {
    for (int dx = -3; dx <= 3; dx++) {
      int nx = x + dx;
      int ny = y + dy;
      if (nx >= 0 && ny >= 0 && nx < world.GetWidth() &&
          ny < world.GetHeight()) {
        if (std::hypot(dx, dy) <= 3.5f) {
          cities[cityID].territory.push_back(
              {static_cast<float>(nx), static_cast<float>(ny)});
          world.GetTile(nx, ny).ownerCityID = cityID; // Mark ownership
        }
      }
    }
  }

  // Add a FREE starting cabana at city center
  Building cabana;
  cabana.id = static_cast<int>(cities[cityID].buildings.size());
  cabana.type = BuildingType::Cabana;
  cabana.tileX = x;
  cabana.tileY = y;
  cabana.cityID = cityID;
  cabana.variant = 0; // First cabana variant
  cabana.isComplete = true;
  cabana.isComplete = true;
  cities[cityID].buildings.push_back(cabana);
  world.GetTile(x, y).isOccupied = true; // Mark as occupied

  // Update housing capacity
  cities[cityID].populationCap =
      2 + GetBuildingHousingCapacity(BuildingType::Cabana);

  TraceLog(LOG_INFO, "CITY: Founded with free Cabana at (%d,%d), popCap = %d",
           x, y, cities[cityID].populationCap);

  return cityID;
}

void SimulationManager::AddCitizenToCity(int cityID, int citizenID) {
  auto it = cities.find(cityID);
  if (it != cities.end()) {
    it->second.citizenIDs.push_back(citizenID);
  }
}

// ============================================================================
// UPDATE SUBSYSTEMS
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
    Entity *myEntity = nullptr;
    std::vector<Entity> &entities = world.GetEntitiesMutable();
    for (Entity &e : entities) {
      if (e.citizenID == c.id) {
        myEntity = &e;
        break;
      }
    }

    // Age - much faster for gameplay!
    // Children age faster to become adults sooner (game-years per second)
    float ageMultiplier = c.isChild() ? 5.0f : 1.0f; // Children grow 5x faster
    float yearProgress =
        deltaTime * 0.05f *
        ageMultiplier; // ~0.2 years per second base (Slower aging)
    c.age += yearProgress;
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

    // 3. Go Home if Tired
    // Homeless push themselves harder (need < 10% energy to stop)
    float restThreshold = (c.homeID != -1) ? 20.0f : 10.0f;
    bool needsRest = c.energy < restThreshold;
    if (needsRest && !c.isGoingHome && !c.isResting) {
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
      c.workState = Citizen::WorkState::Idle;
      c.isWorking = false;
      c.stateTimer = 0.0f;
      // Force a random wander to break loops
      c.workState = Citizen::WorkState::Wandering;
      TraceLog(LOG_INFO,
               "SIMULATION: Citizen %d stuck GoingToWork (>40s) - WANDERING",
               c.id);
      c.isWorking = false;
      c.stateTimer = 0.0f;
      TraceLog(LOG_INFO,
               "SIMULATION: Citizen %d stuck GoingToWork (>40s) - RESETTING",
               c.id);
    }
    // Timeout for Working (stuck animation or missing logic)
    else if (c.workState == Citizen::WorkState::Working &&
             c.stateTimer > 60.0f) {
      c.workState = Citizen::WorkState::Idle;
      c.isWorking = false;
      c.stateTimer = 0.0f;
      c.workState = Citizen::WorkState::Wandering;
      TraceLog(LOG_INFO,
               "SIMULATION: Citizen %d stuck Working (>60s) - WANDERING", c.id);
      c.isWorking = false;
      c.stateTimer = 0.0f;
      TraceLog(LOG_INFO,
               "SIMULATION: Citizen %d stuck Working (>60s) - RESETTING", c.id);
    }
    // Timeout for ReturningHome
    else if (c.workState == Citizen::WorkState::ReturningHome &&
             c.stateTimer > 40.0f) {
      c.workState = Citizen::WorkState::Idle;
      c.isWorking = false;
      c.stateTimer = 0.0f;
      c.workState = Citizen::WorkState::Wandering;
      TraceLog(LOG_INFO,
               "SIMULATION: Citizen %d stuck ReturningHome (>40s) - WANDERING",
               c.id);
      c.isWorking = false;
      c.stateTimer = 0.0f;
      TraceLog(LOG_INFO,
               "SIMULATION: Citizen %d stuck ReturningHome (>40s) - RESETTING",
               c.id);
    }

    // === GENERIC WANDERING ===
    // If wandering, pick a random spot and move there
    if (c.workState == Citizen::WorkState::Wandering) {
      if (!myEntity->hasTarget) {
        int rx = (rand() % 11) - 5;
        int ry = (rand() % 11) - 5;
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

      // Check if reached wander target
      float dist = std::hypot(myEntity->position.x - myEntity->targetPos.x,
                              myEntity->position.y - myEntity->targetPos.y);
      if (dist < 0.5f || !myEntity->hasTarget) {
        c.workState = Citizen::WorkState::Idle;
        myEntity->hasTarget = false;
        myEntity->state = EntityState::Idle;
      }
    }

    // === SETTLER AI ===
    // Homeless adults periodically check if they should join or found a city
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
          tile.stumpVariant = rand() % 2; // Random stump art (0 or 1)
          tile.regrowthTimer = 0.0f;      // Start regrowth countdown

          c.carryingResource += 3;    // Each tree gives 3 wood
          c.skillWoodcutting += 0.5f; // Gain skill
          if (c.skillWoodcutting > 100.0f)
            c.skillWoodcutting = 100.0f;

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
          int pick = rand() % std::min((int)harvestCandidates.size(), 5);
          harvestX = harvestCandidates[pick].second.first;
          harvestY = harvestCandidates[pick].second.second;
        }

        int plantX = -1, plantY = -1;
        if (!plantCandidates.empty()) {
          std::sort(plantCandidates.begin(), plantCandidates.end());
          int pick = rand() % std::min((int)plantCandidates.size(), 5);
          plantX = plantCandidates[pick].second.first;
          plantY = plantCandidates[pick].second.second;
        }

        // Priority 3: Plant Trees (Reforestation)
        // Only 30% chance per idle cycle to attempt tree planting (reduce spam)
        if (harvestX == -1 && plantX == -1 && (rand() % 100) < 30) {
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
            int idx = rand() % treeCandidates.size();
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
              tile.decorationVariant = rand() % 3; // Random variant

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
            int idx = rand() % myCity->territory.size();
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
          if (rand() % 10 == 0)
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
        // SCAN FOR ENEMIES
        int enemyID = -1;
        float closestDist = 999999.0f;
        float detectRange = 25.0f; // High visual range

        for (Entity &otherE : world.GetEntitiesMutable()) {
          if (otherE.id == myEntity->id)
            continue;

          // Check if it's an intelligent entity
          if (otherE.IsIntelligent()) {
            Citizen *otherCitizen = GetCitizen(otherE.citizenID);
            if (otherCitizen && otherCitizen->cityID != c.cityID &&
                otherCitizen->cityID != -1) {
              // Hostile found!
              float d = std::hypot(myEntity->position.x - otherE.position.x,
                                   myEntity->position.y - otherE.position.y);
              if (d <= detectRange && d < closestDist) {
                closestDist = d;
                enemyID = otherCitizen->id;
              }
            }
          }
        }

        if (enemyID != -1) {
          // Enemy spotted, transition to engage
          c.workState = Citizen::WorkState::GoingToWork;
          c.targetEntityID = enemyID; // New field to track target
          c.isWorking = true;
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Run; // Soldiers run to combat
          TraceLog(LOG_INFO, "SOLDIER: Citizen %d locked onto Enemy %d", c.id,
                   enemyID);
        } else {
          // Patrol logic: Walk in straight lines through the city streets
          if (!myEntity->hasTarget) {
            // Pick a random cardinal direction
            int r = rand() % 4;
            int dx = 0, dy = 0;
            if (r == 0)
              dx = 1;
            else if (r == 1)
              dx = -1;
            else if (r == 2)
              dy = 1;
            else
              dy = -1;

            int maxDist = 4 + rand() % 8; // Patrol 4 to 11 tiles
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
        Entity *enemyEntity = nullptr;
        for (Entity &otherE : world.GetEntitiesMutable()) {
          if (otherE.citizenID == enemy->id) {
            enemyEntity = &otherE;
            break;
          }
        }

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
        Entity *enemyEntity = nullptr;
        for (Entity &otherE : world.GetEntitiesMutable()) {
          if (otherE.citizenID == enemy->id) {
            enemyEntity = &otherE;
            break;
          }
        }

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
  }
}

void SimulationManager::UpdateCities(World &world, float deltaTime) {
  // Birth timer (don't check every frame)
  static float birthCheckTimer = 0.0f;
  birthCheckTimer += deltaTime;
  bool doBirthCheck =
      birthCheckTimer >= 5.0f; // Check for births every 5 seconds
  if (doBirthCheck)
    birthCheckTimer = 0.0f;

  for (auto &pair : cities) {
    City &city = pair.second;
    if (!city.isAlive)
      continue;

    city.age += deltaTime / secondsPerYear; // Age in years

    // === FOOD PRODUCTION ===
    // Passive food generation based on territory (farms, foraging, etc.)
    // Every 10 seconds of real time, add some food based on population
    static float foodProductionTimer = 0.0f;
    foodProductionTimer += deltaTime;
    if (foodProductionTimer >= 10.0f) {
      foodProductionTimer = 0.0f;
      // Each city produces some food passively (simulating gathering/farming)
      int baseProduction = 5 + city.GetPopulation() / 2;
      city.resources.food += baseProduction;

      // Cap resources at max storage
      if (city.resources.food > city.maxStorage)
        city.resources.food = city.maxStorage;
    }

    // === FOOD CONSUMPTION ===
    // Every citizen needs food periodically
    int hungryCount = 0;
    for (int citizenID : city.citizenIDs) {
      Citizen *c = GetCitizen(citizenID);
      if (c && c->isAlive && c->hunger > 50.0f) {
        hungryCount++;
      }
    }

    // Use food to feed hungry citizens
    int toFeed = std::min(hungryCount, city.resources.food);
    city.resources.food -= toFeed;

    // Reduce hunger for fed citizens
    int fed = 0;
    for (int citizenID : city.citizenIDs) {
      if (fed >= toFeed)
        break;
      Citizen *c = GetCitizen(citizenID);
      if (c && c->isAlive && c->hunger > 50.0f) {
        c->hunger -= 50.0f;
        if (c->hunger < 0)
          c->hunger = 0;
        fed++;
      }
    }

    // === POPULATION GROWTH (BIRTHS) ===
    // Slower reproduction: check every 30s, need energy/food, soft population
    // cap
    static float birthCheckTimer = 0.0f;
    birthCheckTimer += deltaTime;

    // Soft cap: max 5 citizens per building (housing limit)
    int housingCap = std::max(10, static_cast<int>(city.buildings.size()) * 5);

    if (birthCheckTimer >= 30.0f &&
        city.resources.food > city.GetPopulation() * 5 &&
        city.GetPopulation() < housingCap) {
      birthCheckTimer = 0.0f;

      // Find a female and male adult citizen for parents
      Citizen *mother = nullptr;
      Citizen *father = nullptr;

      for (int citizenID : city.citizenIDs) {
        Citizen *c = GetCitizen(citizenID);
        // Stricter requirements for parents
        if (c && c->isAlive && c->isAdult() && !c->isElder() &&
            c->energy > 30.0f && c->hunger < 50.0f) {
          if (!mother && c->isFemale) {
            mother = c;
          } else if (!father && !c->isFemale &&
                     (!mother || c->id != mother->id)) {
            father = c;
          }
          if (mother && father)
            break;
        }
      }

      // If we have two adults, create a baby
      if (mother && father) {
        // Create child citizen
        Citizen child =
            CreateChildCitizen(GetNextCitizenID(), *mother, *father, city.id);
        int childID = AddCitizen(child);
        city.citizenIDs.push_back(childID);

        // Spawn child entity
        float spawnX = city.center.x + (rand() % 3 - 1);
        float spawnY = city.center.y + (rand() % 3 - 1);

        std::vector<Entity> &entities = world.GetEntitiesMutable();
        Entity e;
        e.id = entities.size();
        e.type =
            child.isFemale ? EntityType::HumanWoman : EntityType::HumanUnarmed;
        e.position = {spawnX, spawnY};
        e.targetPos = e.position;
        e.state = EntityState::Idle;
        e.health = 100.0f;
        e.currentFrame = 0;
        e.animTime = 0.0f;
        e.facingDirection = 0;
        e.hasTarget = false;
        e.speed = 2.0f;
        e.citizenID = childID;
        entities.push_back(e);

        // Cost for birth
        city.resources.food -= 20;

        TraceLog(
            LOG_INFO,
            "SIMULATION: Birth! Child %d born in City %d (Parents: %d, %d)",
            childID, city.id, mother->id, father->id);
      } else {
        // TRACE why no birth?
        // static float logTimer = 0.0f;
        // logTimer += deltaTime;
        // if (logTimer > 10.0f) {
        //    TraceLog(LOG_INFO, "BIRTH CHECK: No eligible parents found in City
        //    %d. Needs Energy > 30, Hunger < 50.", city.id); logTimer = 0.0f;
        // }
      }
    }

    // === REMOVE DEAD CITIZENS ===
    city.citizenIDs.erase(std::remove_if(city.citizenIDs.begin(),
                                         city.citizenIDs.end(),
                                         [this](int id) {
                                           const Citizen *c = GetCitizen(id);
                                           return !c || !c->isAlive;
                                         }),
                          city.citizenIDs.end());

    // City dies if no citizens left
    if (city.citizenIDs.empty()) {
      city.isAlive = false;
      TraceLog(LOG_INFO, "SIMULATION: City %d has fallen! (No citizens remain)",
               city.id);
    }

    // === GOVERNMENT SYSTEM: JOB ASSIGNMENT ===
    AssignJobs(city);

    // === CROP GROWTH ===
    // Update growth of planted tiles in territory
    for (const Vector2 &tile : city.territory) {
      int tx = static_cast<int>(tile.x);
      int ty = static_cast<int>(tile.y);
      Tile &t = world.GetTile(tx, ty);

      if (t.isPlanted && t.farmOwnerCityID == city.id &&
          t.growthProgress < 100.0f) {
        // Crops grow over time (about 30 seconds to full growth)
        t.growthProgress += deltaTime * 3.3f;
        if (t.growthProgress > 100.0f)
          t.growthProgress = 100.0f;
      }
    }

    // === AUTOMATIC BUILDING CONSTRUCTION ===
    static float buildTimer = 0.0f;
    buildTimer += deltaTime;
    if (buildTimer >= 5.0f) { // Check every 5 seconds
      buildTimer = 0.0f;
      AttemptConstruction(city, world);
    }

    // === TERRITORY EXPANSION ===
    AttemptTerritoryExpansion(city, world);

    // === PASSIVE STONE GENERATION (MINES) ===
    static float mineTimer = 0.0f;
    mineTimer += deltaTime;
    if (mineTimer >= 3.0f) { // Every 3 seconds
      mineTimer = 0.0f;
      int stoneGen = 0;
      int workshopCount = 0;
      for (const auto &b : city.buildings) {
        if (b.isComplete && b.type == BuildingType::Mina) {
          // Tier 1 (Var 0) -> 2 stone
          // Tier 6 (Var 5) -> 12 stone
          stoneGen += (b.variant + 1) * 2;
        }
        if (b.isComplete && b.type == BuildingType::Workshop)
          workshopCount++;
      }
      // Workshop bonus: +20% per workshop (additive)
      if (workshopCount > 0 && stoneGen > 0) {
        stoneGen = (int)(stoneGen * (1.0f + workshopCount * 0.2f));
      }
      if (stoneGen > 0) {
        city.resources.stone += stoneGen;
        if (city.resources.stone > city.maxStorage)
          city.resources.stone = city.maxStorage;
      }
    }

    // Update Building Evolution
    UpdateBuildingUpgrade(city);
  }
} // Close UpdateCities

// ============================================================================
// NEW: ADAPTIVE CONSTRUCTION
// ============================================================================
void SimulationManager::AttemptConstruction(City &city, World &world) {
  // 1. Calculate Storage & Needs
  int totalStorage = 200;
  int plannedCapacity = 0;

  for (const auto &b : city.buildings) {
    if (b.isComplete) {
      if (b.type == BuildingType::Recursos)
        totalStorage += 500;
      else if (b.type == BuildingType::StockpileStone)
        totalStorage += 500;
    }

    if (b.IsHousing()) {
      int cap = b.capacity;
      if (cap == 0)
        cap = GetBuildingHousingCapacity(b.type);
      if (cap == 0)
        cap = 2; // Fallback
      plannedCapacity += cap;
    }
  }
  city.maxStorage = totalStorage;

  int population = city.GetPopulation();

  // 2. Decide what to build
  bool needWoodStorage = city.resources.wood >= (city.maxStorage * 0.8f);
  bool needStoneStorage = city.resources.stone >= (city.maxStorage * 0.8f);
  bool needHousing = population > (plannedCapacity - 2);

  // Mine Logic: Build if stone is critically low OR if we have few mines for
  // our population
  int mineCount = 0;
  for (const auto &b : city.buildings) {
    if (b.type == BuildingType::Mina)
      mineCount++;
  }
  // Target: 1 mine per 20 people, minimum 1 if stone < 20
  bool needMine = (city.resources.stone < 20 && mineCount == 0) ||
                  (mineCount < (population / 20) + 1);

  BuildingType typeToBuild = BuildingType::None;
  int woodCost = 0;

  if (needHousing && city.resources.wood >= 5) {
    if (city.buildings.size() < 3) {
      typeToBuild = BuildingType::Cabana;
      woodCost = 5;
    } else if (city.buildings.size() < 8) {
      typeToBuild = BuildingType::Casa;
      woodCost = 10;
    } else {
      typeToBuild = BuildingType::Casa2;
      woodCost = 15;
    }
  } else if (needMine && city.resources.wood >= 50) {
    typeToBuild = BuildingType::Mina;
    woodCost = 50;
  } else if (needStoneStorage && city.resources.wood >= 50) {
    typeToBuild = BuildingType::StockpileStone;
    woodCost = 50;
  } else if (needWoodStorage && city.resources.wood >= 50) {
    typeToBuild = BuildingType::Recursos;
    woodCost = 30;
  }

  // === NEW BUILDINGS (population-gated) ===
  if (typeToBuild == BuildingType::None) {
    // Count existing new buildings
    int quartelCount = 0, mercadoCount = 0, workshopCount = 0;
    int tavernaCount = 0, casteloCount = 0;
    for (const auto &b : city.buildings) {
      if (b.type == BuildingType::Quartel)
        quartelCount++;
      else if (b.type == BuildingType::Mercado)
        mercadoCount++;
      else if (b.type == BuildingType::Workshop)
        workshopCount++;
      else if (b.type == BuildingType::Taverna)
        tavernaCount++;
      else if (b.type == BuildingType::Castelo)
        casteloCount++;
    }

    BuildingCost cost = {0, 0, 0};

    // Barracks: 1 when pop >= 15
    if (quartelCount == 0 && population >= 15) {
      cost = GetBuildingCost(BuildingType::Quartel);
      if (city.resources.wood >= cost.wood &&
          city.resources.stone >= cost.stone) {
        typeToBuild = BuildingType::Quartel;
        woodCost = cost.wood;
        city.resources.stone -= cost.stone;
      }
    }
    // Market: 1 per 30 pop
    if (typeToBuild == BuildingType::None &&
        mercadoCount < (population / 30) + 1 && population >= 10) {
      cost = GetBuildingCost(BuildingType::Mercado);
      if (city.resources.wood >= cost.wood &&
          city.resources.stone >= cost.stone) {
        typeToBuild = BuildingType::Mercado;
        woodCost = cost.wood;
        city.resources.stone -= cost.stone;
      }
    }
    // Workshop: 1 per 25 pop
    if (typeToBuild == BuildingType::None &&
        workshopCount < (population / 25) + 1 && population >= 12) {
      cost = GetBuildingCost(BuildingType::Workshop);
      if (city.resources.wood >= cost.wood &&
          city.resources.stone >= cost.stone) {
        typeToBuild = BuildingType::Workshop;
        woodCost = cost.wood;
        city.resources.stone -= cost.stone;
      }
    }
    // Tavern: 1 per 20 pop
    if (typeToBuild == BuildingType::None &&
        tavernaCount < (population / 20) + 1 && population >= 8) {
      cost = GetBuildingCost(BuildingType::Taverna);
      if (city.resources.wood >= cost.wood &&
          city.resources.stone >= cost.stone) {
        typeToBuild = BuildingType::Taverna;
        woodCost = cost.wood;
        city.resources.stone -= cost.stone;
      }
    }
    // Castle: max 1, pop >= 40
    if (typeToBuild == BuildingType::None && casteloCount == 0 &&
        population >= 40) {
      cost = GetBuildingCost(BuildingType::Castelo);
      if (city.resources.wood >= cost.wood &&
          city.resources.stone >= cost.stone) {
        typeToBuild = BuildingType::Castelo;
        woodCost = cost.wood;
        city.resources.stone -= cost.stone;
      }
    }
  }

  if (typeToBuild == BuildingType::None)
    return;
  if (city.resources.wood < woodCost)
    return;

  // 3. Find Placement (Adaptive Spacing)
  int bestBx = -1, bestBy = -1;
  float bestScore = 999999.0f;

  int spacingLevels[] = {
      3, 2, 1}; // Reduced spacing slightly for denser growth, allowing 3/2/1

  for (int spacing : spacingLevels) {
    if (bestBx != -1)
      break;

    int startIdx = rand() % city.territory.size();

    for (size_t i = 0; i < city.territory.size(); i++) {
      size_t idx = (startIdx + i) % city.territory.size();
      int bx = (int)city.territory[idx].x;
      int by = (int)city.territory[idx].y;

      BuildingSize size = GetBuildingSize(typeToBuild);

      bool placeable = true;
      for (int bdy = 0; bdy < size.height; bdy++) {
        for (int bdx = 0; bdx < size.width; bdx++) {
          int tx = bx + bdx;
          int ty = by + bdy;

          if (tx < 0 || tx >= world.GetWidth() || ty < 0 ||
              ty >= world.GetHeight()) {
            placeable = false;
            break;
          }
          const Tile &tile = world.GetTileConst(tx, ty);
          if (tile.isOccupied) { // Removed !world.IsWalkable(tx, ty) so it can
                                 // build over trees/rocks
            placeable = false;
            break;
          }
          if (tile.type == TileType::DeepOcean ||
              tile.type == TileType::Ocean ||
              tile.type == TileType::ShallowOcean ||
              tile.type == TileType::Mountain) {
            placeable = false;
            break;
          }
        }
        if (!placeable)
          break;
      }
      if (!placeable)
        continue;

      // Check neighbors
      bool tooClose = false;
      for (int dy = -spacing; dy < size.height + spacing; dy++) {
        for (int dx = -spacing; dx < size.width + spacing; dx++) {
          if (dx >= 0 && dx < size.width && dy >= 0 && dy < size.height)
            continue;

          int nx = bx + dx;
          int ny = by + dy;
          if (nx >= 0 && nx < world.GetWidth() && ny >= 0 &&
              ny < world.GetHeight()) {
            if (world.GetTileConst(nx, ny).isOccupied) {
              tooClose = true;
              break;
            }
          }
        }
        if (tooClose)
          break;
      }
      if (tooClose)
        continue;

      float dist = std::hypot(bx - city.center.x, by - city.center.y);
      float score = dist + (rand() % 100) / 100.0f;

      if (score < bestScore) {
        bestScore = score;
        bestBx = bx;
        bestBy = by;
      }
    }
  }

  if (bestBx != -1) {
    Building newBuilding;
    newBuilding.id = GetNextBuildingID();
    newBuilding.cityID = city.id;
    newBuilding.tileX = bestBx;
    newBuilding.tileY = bestBy;
    newBuilding.isComplete = false;
    newBuilding.constructionProgress = 0.0f;
    newBuilding.type = typeToBuild;
    newBuilding.variant = rand() % 3;
    newBuilding.capacity = GetBuildingHousingCapacity(typeToBuild);

    city.resources.wood -= woodCost;
    city.buildings.push_back(newBuilding);

    BuildingSize size = GetBuildingSize(typeToBuild);
    for (int dy = 0; dy < size.height; dy++) {
      for (int dx = 0; dx < size.width; dx++) {
        int tx = bestBx + dx;
        int ty = bestBy + dy;
        if (tx >= 0 && tx < world.GetWidth() && ty >= 0 &&
            ty < world.GetHeight()) {
          Tile &bdTile = world.GetTile(tx, ty);
          bdTile.isOccupied = true;
          bdTile.hasStump = false;                  // Clear stumps
          bdTile.isPlanted = false;                 // Clear crops
          bdTile.decoration = DecorationType::None; // Clear trees/rocks
        }
      }
    }

    TraceLog(LOG_INFO,
             "CITY %d: Started construction of %s at (%d,%d). Storage: %d/%d",
             city.id, GetBuildingName(typeToBuild), bestBx, bestBy,
             city.resources.wood, city.maxStorage);
  } else {
    TraceLog(LOG_INFO,
             "CITY %d: Failed to place %s! No valid spot found in territory. "
             "Size: %d",
             city.id, GetBuildingName(typeToBuild), (int)city.territory.size());
  }
}

// ============================================================================
// NEW: ROBUST TERRITORY EXPANSION
// ============================================================================
void SimulationManager::AttemptTerritoryExpansion(City &city, World &world) {
  // Territory scales primarily with population, buildings add a small buffer
  int desiredTerritorySize =
      30 + city.GetPopulation() * 8 + (int)city.buildings.size() * 5;
  if ((int)city.territory.size() >= desiredTerritorySize)
    return;

  // SCORING CANDIDATES
  // We want to find the BEST tile to add.
  // Criteria:
  // 1. Adjacent to current territory
  // 2. Walkable (Grass/Sand/Forest)
  // 3. Not owned
  // 4. Close to city center (Spiral growth)

  struct Candidate {
    int x, y;
    float score;
  };
  std::vector<Candidate> candidates;

  int dirs[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};

  // Scan ALL territory tiles to find border
  // (Optimization: In a real game, maintain a "Border List". For now, brute
  // force is fine for < 1000 tiles)
  for (const Vector2 &t : city.territory) {
    int cx = (int)t.x;
    int cy = (int)t.y;

    for (int d = 0; d < 4; d++) {
      int nx = cx + dirs[d][0];
      int ny = cy + dirs[d][1];

      // Bounds check
      if (nx < 0 || ny < 0 || nx >= world.GetWidth() || ny >= world.GetHeight())
        continue;

      // Check if already owned (by anyone)
      if (world.GetTile(nx, ny).ownerCityID != -1)
        continue;

      // Check terrain type
      TileType type = world.GetTileConst(nx, ny).type;
      bool isValid = (type == TileType::Grass || type == TileType::Forest ||
                      type == TileType::Sand || type == TileType::DesertSand);

      // If desperate (stuck), maybe allow other types? For now, stick to
      // land.
      if (!isValid)
        continue;

      // Calculate Score
      float dist = std::hypot(nx - city.center.x, ny - city.center.y);

      // Base score = distance (lower is better)
      float score = dist;

      // Terrain modifiers
      if (type == TileType::Grass)
        score -= 5.0f; // Prefer Grass
      if (type == TileType::Forest)
        score -= 2.0f; // Then Forest

      candidates.push_back({nx, ny, score});
    }
  }

  if (candidates.empty()) {
    // TraceLog(LOG_WARNING, "CITY %d: Expansion failed! No valid
    // candidates.", city.id);
    return;
  }

  // Sort by score (ascending)
  std::sort(
      candidates.begin(), candidates.end(),
      [](const Candidate &a, const Candidate &b) { return a.score < b.score; });

  // Pick top candidates (greedy expansion)
  int expansionBudget = 10; // Add up to 10 tiles per check
  for (int i = 0; i < expansionBudget && i < (int)candidates.size(); i++) {
    Candidate &c = candidates[i];

    // Double check ownership (in case duplicates in list)
    if (world.GetTile(c.x, c.y).ownerCityID != -1)
      continue;

    city.territory.push_back({(float)c.x, (float)c.y});
    world.GetTile(c.x, c.y).ownerCityID = city.id;
    // TraceLog(LOG_INFO, "CITY %d: Expanded territory to (%d,%d)", city.id,
    // c.x, c.y);
  }
}

// === HOUSING SYSTEM ===
int GetHousingCapacity(const City &city) {
  int totalCapacity = 0;
  for (const auto &b : city.buildings) {
    if (b.isComplete && b.IsHousing()) {
      totalCapacity += b.capacity;
    }
  }
  return totalCapacity;
}

void SimulationManager::AssignHousing(City &city) {
  // 1. Find Homeless
  std::vector<Citizen *> homeless;
  for (int id : city.citizenIDs) {
    Citizen *c = GetCitizen(id);
    if (c && c->isAlive && c->homeID == -1) {
      homeless.push_back(c);
    }
  }

  if (homeless.empty())
    return;

  // 2. Find Housing with vacancy
  for (auto &b : city.buildings) {
    if (b.isComplete && b.IsHousing()) {

      // Auto-configure capacity if 0 (migration fix)
      if (b.capacity == 0) {
        if (b.type == BuildingType::Cabana)
          b.capacity = 3;
        else if (b.type == BuildingType::Casa)
          b.capacity = 5;
        else if (b.type == BuildingType::Casa2)
          b.capacity = 8;
        else
          b.capacity = 2; // Fallback
      }

      // TRACE LOG for debugging housing
      // TraceLog(LOG_INFO, "HOUSING CHECK: Building %d (Cap %d, Occ %d)",
      // b.id, b.capacity, (int)b.occupants.size());

      while (b.occupants.size() < (size_t)b.capacity && !homeless.empty()) {
        Citizen *c = homeless.back();
        homeless.pop_back();

        // Assign
        c->homeID = b.id;
        b.occupants.push_back(c->id);
        TraceLog(LOG_INFO, "HOUSING: Citizen %s moved into Building %d",
                 c->name.c_str(), b.id);
      }
    }
  }
}

// === ASSIGN JOBS ===
// Government system to assign professions based on city needs
void SimulationManager::AssignJobs(City &city) {
  AssignHousing(city); // Run housing assignment first

  int population = city.GetPopulation();
  if (population == 0)
    return;

  // Count current workers
  int lumberjacks = 0;
  int farmers = 0;
  int miners = 0;
  int builders = 0;
  int soldiers = 0;
  int unemployed = 0;

  std::vector<Citizen *> availableWorkers;

  for (int id : city.citizenIDs) {
    Citizen *c = GetCitizen(id);
    if (!c || !c->isAlive || !c->isAdult())
      continue;

    switch (c->profession) {
    case Profession::Lumberjack:
      lumberjacks++;
      break;
    case Profession::Farmer:
      farmers++;
      break;
    case Profession::Miner:
      miners++;
      break;
    case Profession::Builder:
      builders++;
      break;
    case Profession::Soldier:
      soldiers++;
      break;
    case Profession::None:
      unemployed++;
      availableWorkers.push_back(c);
      break;
    default:
      break;
    }
  }

  if (availableWorkers.empty())
    return;

  // Determine Desired Jobs
  int desiredFarmers = std::max(1, population / 4); // 1 per 4 pop
  if (city.resources.food < 100)
    desiredFarmers++; // Emergency food

  int desiredLumberjacks = std::max(1, population / 6); // 1 per 6 pop
  if (city.resources.wood < 50)
    desiredLumberjacks += 2; // Need wood!

  // Aggressive Builder Assignment for City Growth
  // Aggressive Builder Assignment for City Growth
  int desiredBuilders = 0;

  // Check if we have incomplete buildings
  bool hasConstructionSites = false;
  int barracksCount = 0;
  for (const auto &b : city.buildings) {
    if (!b.isComplete) {
      hasConstructionSites = true;
    }
    if (b.isComplete && b.type == BuildingType::Quartel) {
      barracksCount++;
    }
  }

  int desiredSoldiers = barracksCount * 2; // Each barracks supports 2 soldiers

  if (hasConstructionSites) {
    // If we have stuff to build, we NEED builders
    desiredBuilders = std::max(1, population / 4);
  } else if (city.resources.wood > 20 || city.resources.stone > 10) {
    // If we have resources but no sites, we MIGHT build/upgrade
    // Assign up to 20% of population as builders (reduced from 30% to save
    // food/wood gatherers)
    desiredBuilders = std::max(1, population / 5);
  } else {
    // Maintenance mode (1 builder per 20 buildings)
    desiredBuilders = std::max(1, static_cast<int>(city.buildings.size()) / 20);
  }

  // More miners earlier (Feedback: "User saw only 1")
  int desiredMiners = 0;
  if (population > 5)
    desiredMiners = 2; // Start with 2 miners
  if (population > 20)
    desiredMiners = 3;
  if (population > 40)
    desiredMiners = 5;

  // Assign based on priority: Food > Wood > Build > Mine > Soldier
  // Women can only be Farmers
  for (Citizen *c : availableWorkers) {
    if (farmers < desiredFarmers) {
      c->profession = Profession::Farmer;
      farmers++;
      TraceLog(LOG_INFO, "GOV: Citizen %d assigned as FARMER in City %d", c->id,
               city.id);
    } else if (!c->isFemale && soldiers < desiredSoldiers) {
      c->profession = Profession::Soldier;
      soldiers++;
      TraceLog(LOG_INFO, "GOV: Citizen %d assigned as SOLDIER in City %d",
               c->id, city.id);
    } else if (!c->isFemale && lumberjacks < desiredLumberjacks) {
      c->profession = Profession::Lumberjack;
      lumberjacks++;
      TraceLog(LOG_INFO, "GOV: Citizen %d assigned as LUMBERJACK in City %d",
               c->id, city.id);
    } else if (!c->isFemale && builders < desiredBuilders) {
      c->profession = Profession::Builder;
      builders++;
      TraceLog(LOG_INFO, "GOV: Citizen %d assigned as BUILDER in City %d",
               c->id, city.id);
    } else if (!c->isFemale && miners < desiredMiners) {
      c->profession = Profession::Miner;
      miners++;
      TraceLog(LOG_INFO, "GOV: Citizen %d assigned as MINER in City %d", c->id,
               city.id);
    } else if (c->isFemale) {
      // Women default to Farmer if no farmer slots left
      c->profession = Profession::Farmer;
      farmers++;
    } else {
      // Remaining males are Gathering/Laborers
    }
  }
}

void SimulationManager::UpdateKingdoms(World &world, float deltaTime) {
  for (auto &pair : kingdoms) {
    Kingdom &kingdom = pair.second;
    if (!kingdom.isAlive)
      continue;

    kingdom.age += deltaTime / secondsPerYear;

    // Remove dead cities
    kingdom.cityIDs.erase(std::remove_if(kingdom.cityIDs.begin(),
                                         kingdom.cityIDs.end(),
                                         [this](int id) {
                                           const City *c = GetCity(id);
                                           return !c || !c->isAlive;
                                         }),
                          kingdom.cityIDs.end());

    // Kingdom dies if no cities left
    if (kingdom.cityIDs.empty()) {
      kingdom.isAlive = false;
    }
  }
}

// ============================================================================
// CITY EXPANSION
// ============================================================================
void SimulationManager::ExpandTerritory(City &city, World &world) {
  // Expansion Cost (lowered for faster growth)
  const int COST_WOOD = 30;
  const int COST_FOOD = 30;

  if (city.resources.wood < COST_WOOD || city.resources.food < COST_FOOD)
    return;

  // Find candidate tiles
  std::vector<Vector2> candidates;

  for (const auto &tilePos : city.territory) {
    int tx = static_cast<int>(tilePos.x);
    int ty = static_cast<int>(tilePos.y);

    // Check 4 neighbors
    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1, 0, 0};

    for (int i = 0; i < 4; i++) {
      int nx = tx + dx[i];
      int ny = ty + dy[i];

      // Check bounds
      if (nx < 0 || ny < 0 || nx >= world.GetWidth() || ny >= world.GetHeight())
        continue;

      // Check if already in THIS city's territory
      bool ownedByMe = false;
      for (const auto &t : city.territory) {
        if (static_cast<int>(t.x) == nx && static_cast<int>(t.y) == ny) {
          ownedByMe = true;
          break;
        }
      }
      if (ownedByMe)
        continue;

      // Check terrain type restrictions
      const Tile &tile = world.GetTileConst(nx, ny);
      if (tile.type == TileType::DeepOcean || tile.type == TileType::Ocean ||
          tile.type == TileType::ShallowOcean ||
          tile.type == TileType::Mountain) {
        continue;
      }

      // Check if owned by ANY city (expensive check, but necessary)
      bool taken = false;
      for (const auto &pair : cities) {
        for (const auto &t : pair.second.territory) {
          if (static_cast<int>(t.x) == nx && static_cast<int>(t.y) == ny) {
            taken = true;
            break;
          }
        }
        if (taken)
          break;
      }
      if (taken)
        continue;

      // Valid candidate
      candidates.push_back({static_cast<float>(nx), static_cast<float>(ny)});
    }
  }

  // Pick a random candidate if any found
  if (!candidates.empty()) {
    int idx = rand() % candidates.size(); // Simple random selection
    Vector2 target = candidates[idx];

    // Add to territory
    city.territory.push_back(target);
    world.GetTile((int)target.x, (int)target.y).ownerCityID = city.id;

    // Pay the cost
    city.resources.wood -= COST_WOOD;
    city.resources.food -= COST_FOOD;

    TraceLog(LOG_INFO, "EXPANSION: City %s grew to (%d, %d). Size: %d",
             city.name.c_str(), (int)target.x, (int)target.y,
             (int)city.territory.size());
  }
}

// === BUILDING EVOLUTION / UPGRADES ===
// === BUILDING EVOLUTION / UPGRADES ===
void SimulationManager::UpdateBuildingUpgrade(City &city) {
  // Check if we have minimum resources to even consider upgrading
  if (city.resources.wood < 10 && city.resources.stone < 5)
    return;

  // Efficiency: Small chance to process upgrade each tick
  if (rand() % 100 > 5)
    return;

  // Iterate to find a candidate for upgrade
  for (auto &b : city.buildings) {
    if (!b.isComplete)
      continue;

    // TIER 0 -> TIER 1 (Cabana/Wood -> MixedCasa)
    if (b.type == BuildingType::Cabana) {
      // Upgrade Cabana -> Casa (Wood)
      int costWood = 10;
      if (city.resources.wood >= costWood) {
        city.resources.wood -= costWood;
        b.type = BuildingType::Casa;
        b.capacity = 5;
        b.variant = rand() % 6; // Variants 0-5
        TraceLog(LOG_INFO,
                 "CITY %d: Upgraded Cabana to Wood Casa (Var %d). Cap: 5",
                 city.id, b.variant);
        return;
      }
    }
    // TIER 1 -> TIER 2 (Casa -> Mixed -> Mansion)
    else if (b.type == BuildingType::Casa) {
      bool isWoodCasa = (b.variant <= 5);
      bool isMixedCasa = (b.variant >= 6 && b.variant <= 8);

      if (isWoodCasa) {
        // Upgrade Wood Casa -> Mixed Casa (Tier 1)
        int costWood = 15;
        int costStone = 5;
        if (city.resources.wood >= costWood &&
            city.resources.stone >= costStone) {
          city.resources.wood -= costWood;
          city.resources.stone -= costStone;
          b.variant = 6 + (rand() % 3); // Variants 6-8
          b.capacity = 5;
          TraceLog(LOG_INFO,
                   "CITY %d: Upgraded Wood Casa to Mixed Casa (Var %d). Cap: 5",
                   city.id, b.variant);
          return;
        }
      } else if (isMixedCasa) {
        // Upgrade Mixed Casa -> Stone Mansion (Tier 2 / Casa2)
        int costStone = 50;
        if (city.resources.stone >= costStone) {
          city.resources.stone -= costStone;
          b.type = BuildingType::Casa2;
          b.capacity = 8;
          b.variant = rand() % 3;
          TraceLog(
              LOG_INFO,
              "CITY %d: Upgraded Mixed Casa to Stone Mansion (Var %d). Cap: 8",
              city.id, b.variant);
          return;
        }
      }
    }
    // MINE UPGRADES
    else if (b.type == BuildingType::Mina) {
      // Max level is 5 (Variant 0 to 5)
      if (b.variant < 5) {
        int currentLevel = b.variant + 1;
        int costWood = 20 * currentLevel;
        int costStone = 10 * currentLevel;

        if (city.resources.wood >= costWood &&
            city.resources.stone >= costStone) {
          city.resources.wood -= costWood;
          city.resources.stone -= costStone;
          b.variant++;
          TraceLog(LOG_INFO,
                   "CITY %d: Upgraded Mine to Level %d (Var %d). Cost: %dW %dS",
                   city.id, b.variant + 1, b.variant, costWood, costStone);
          return;
        }
      }
    }
  }
}
