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
  dayTimer += deltaTime;

  // Day cycle
  if (dayTimer >= secondsPerDay) {
    dayTimer = 0.0f;
    currentDay++;

    // Season cycle (every 30 days)
    seasonTimer++;
    if (seasonTimer >= 30) {
      seasonTimer = 0;
      currentSeason = (currentSeason + 1) % 4;
      if (currentSeason == 0) {
        currentYear++;
      }
    }
  }

  // Update all subsystems
  UpdateCitizens(world, deltaTime);
  UpdateCities(world, deltaTime);
  UpdateKingdoms(world, deltaTime);
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
  score += waterCount * 10.0f; // Water is very valuable
  score += treeCount * 5.0f;   // Trees for wood
  score += grassCount * 3.0f;  // Fertile land

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
        }
      }
    }
  }

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
  float yearProgress =
      deltaTime / (secondsPerDay * 365.0f); // How much of a year passed

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

    // Age
    c.age += yearProgress;
    if (c.age > c.genes.maxAge) {
      c.isAlive = false; // Died of old age
      continue;
    }

    // Hunger increases over time
    c.hunger += deltaTime * 0.5f; // Gets hungry slowly
    if (c.hunger > 100.0f)
      c.hunger = 100.0f;

    // Starving damages health
    if (c.hunger >= 80.0f) {
      c.health -= deltaTime * 5.0f; // Lose 5 HP per second when starving
    }

    // Death from starvation
    if (c.health <= 0.0f) {
      c.isAlive = false;
    }

    // Energy regeneration (when not working)
    if (!c.isWorking && c.energy < 100.0f) {
      c.energy += deltaTime * 2.0f;
      if (c.energy > 100.0f)
        c.energy = 100.0f;
    }

    // === SETTLER AI ===
    // Homeless adults periodically check if they should join or found a city
    if (doSettlerCheck && c.cityID == -1 && c.isAdult()) {
      // Find this citizen's entity to get their position
      std::vector<Entity> &entities = world.GetEntitiesMutable();
      for (Entity &e : entities) {
        if (e.citizenID == c.id) {
          int tx = static_cast<int>(e.position.x);
          int ty = static_cast<int>(e.position.y);

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
            TraceLog(LOG_INFO,
                     "SIMULATION: Citizen %d joined City %d (dist: %.1f)", c.id,
                     nearestCityID, nearestDist);
          }
          // ELSE: If far from cities, try to found a new one
          else if (nearestDist > 15.0f || nearestCityID < 0) {
            float score = ScoreTileForCity(world, tx, ty);

            // If score is good enough, found a city!
            if (score >= 50.0f) {
              int newCityID = FoundCity(world, c.id, tx, ty);
              c.cityID = newCityID; // Explicitly update current reference
              TraceLog(
                  LOG_INFO,
                  "SIMULATION: Citizen %d founded city %d at (%d, %d) with "
                  "score %.1f!",
                  c.id, newCityID, tx, ty, score);
            }
          }
          break;
        }
      }
    }

    // === JOB AI: LUMBERJACK ===
    // Lumberjacks search for trees, chop them, and return wood to city
    if (c.profession == Profession::Lumberjack && c.cityID >= 0 &&
        c.isAdult()) {
      City *myCity = GetCity(c.cityID);
      if (!myCity)
        continue;

      // Find this citizen's entity
      std::vector<Entity> &entities = world.GetEntitiesMutable();
      Entity *myEntity = nullptr;
      for (Entity &e : entities) {
        if (e.citizenID == c.id) {
          myEntity = &e;
          break;
        }
      }
      if (!myEntity)
        continue;

      switch (c.workState) {
      case Citizen::WorkState::Idle: {
        // Find nearest tree in territory
        int bestTileX = -1, bestTileY = -1;
        float bestDist = 999999.0f;

        for (const Vector2 &tile : myCity->territory) {
          int tx = static_cast<int>(tile.x);
          int ty = static_cast<int>(tile.y);
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

        // Chopping takes 3 seconds (faster with skill)
        float chopTime = 3.0f - (c.skillWoodcutting * 0.02f);
        if (chopTime < 1.0f)
          chopTime = 1.0f;

        if (c.workTimer >= chopTime) {
          // Tree chopped! Remove decoration and get wood
          Tile &tile = world.GetTile(c.targetTileX, c.targetTileY);
          tile.decoration = DecorationType::None;

          c.carryingResource += 1;
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
        // Check if we reached the city center
        float distToCity = std::hypot(myEntity->position.x - myCity->center.x,
                                      myEntity->position.y - myCity->center.y);

        if (distToCity < 3.0f) {
          // Deposit resources
          myCity->resources.wood += c.carryingResource;
          TraceLog(
              LOG_INFO,
              "LUMBERJACK: Citizen %d deposited %d wood. City now has %d wood.",
              c.id, c.carryingResource, myCity->resources.wood);
          c.carryingResource = 0;
          c.workState = Citizen::WorkState::Idle;
          c.isWorking = false;
          myEntity->state = EntityState::Idle;
        }
        break;
      }
      }
    }

    // === JOB AI: FARMER ===
    // Farmers plant crops, wait for growth, and harvest food
    if (c.profession == Profession::Farmer && c.cityID >= 0 && c.isAdult()) {
      City *myCity = GetCity(c.cityID);
      if (!myCity)
        continue;

      // Find this citizen's entity
      std::vector<Entity> &entities = world.GetEntitiesMutable();
      Entity *myEntity = nullptr;
      for (Entity &e : entities) {
        if (e.citizenID == c.id) {
          myEntity = &e;
          break;
        }
      }
      if (!myEntity)
        continue;

      switch (c.workState) {
      case Citizen::WorkState::Idle: {
        // Priority 1: Check for mature crops to harvest
        int harvestX = -1, harvestY = -1;
        float closestHarvest = 999999.0f;

        // Priority 2: Find a fertile tile to plant
        int plantX = -1, plantY = -1;
        float closestPlant = 999999.0f;

        for (const Vector2 &tile : myCity->territory) {
          int tx = static_cast<int>(tile.x);
          int ty = static_cast<int>(tile.y);
          Tile &t = world.GetTile(tx, ty);

          // Check if planted and ready to harvest
          if (t.isPlanted && t.growthProgress >= 100.0f &&
              t.farmOwnerCityID == myCity->id) {
            float dist = std::hypot(myEntity->position.x - tx,
                                    myEntity->position.y - ty);
            if (dist < closestHarvest) {
              closestHarvest = dist;
              harvestX = tx;
              harvestY = ty;
            }
          }
          // Check if can plant (grass tile, not planted, no decoration)
          else if (!t.isPlanted && t.type == TileType::Grass &&
                   t.decoration == DecorationType::None) {
            float dist = std::hypot(myEntity->position.x - tx,
                                    myEntity->position.y - ty);
            if (dist < closestPlant) {
              closestPlant = dist;
              plantX = tx;
              plantY = ty;
            }
          }
        }

        // Prioritize harvesting over planting
        if (harvestX >= 0) {
          c.targetTileX = harvestX;
          c.targetTileY = harvestY;
          c.workState = Citizen::WorkState::GoingToWork;
          c.isWorking = true;
          c.workTimer = 0.0f; // Will use work timer to track if harvesting
          myEntity->targetPos = {harvestX + 0.5f, harvestY + 0.5f};
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Walking;
        } else if (plantX >= 0) {
          c.targetTileX = plantX;
          c.targetTileY = plantY;
          c.workState = Citizen::WorkState::GoingToWork;
          c.isWorking = true;
          c.workTimer = -1.0f; // Negative = planting mode
          myEntity->targetPos = {plantX + 0.5f, plantY + 0.5f};
          myEntity->hasTarget = true;
          myEntity->state = EntityState::Walking;
        }
        break;
      }

      case Citizen::WorkState::GoingToWork: {
        float distToTarget =
            std::hypot(myEntity->position.x - (c.targetTileX + 0.5f),
                       myEntity->position.y - (c.targetTileY + 0.5f));

        if (distToTarget < 1.5f) {
          c.workState = Citizen::WorkState::Working;
          myEntity->state = EntityState::Idle;
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
        if (!tile.isPlanted) {
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
              myEntity->targetPos = myCity->center;
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
          TraceLog(
              LOG_INFO,
              "FARMER: Citizen %d deposited %d food. City now has %d food.",
              c.id, c.carryingResource, myCity->resources.food);
          c.carryingResource = 0;
          c.workState = Citizen::WorkState::Idle;
          c.isWorking = false;
          myEntity->state = EntityState::Idle;
        }
        break;
      }
      }
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

    city.age += deltaTime / secondsPerDay; // Age in days

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

    // === TERRITORY EXPANSION ===
    // periodically check if city can expand
    if (rand() % 200 == 0) {
      ExpandTerritory(city, world);
    }

    // === POPULATION GROWTH (BIRTHS) ===
    if (doBirthCheck && city.resources.food > city.GetPopulation() * 3 &&
        city.HasCapacity()) {
      // Find two adult citizens for parents
      Citizen *mother = nullptr;
      Citizen *father = nullptr;

      for (int citizenID : city.citizenIDs) {
        Citizen *c = GetCitizen(citizenID);
        if (c && c->isAlive && c->isAdult() && !c->isElder()) {
          if (!mother) {
            mother = c;
          } else if (!father && c->id != mother->id) {
            father = c;
            break;
          }
        }
      }

      // If we have two adults, create a baby
      if (mother && father) {
        // Create child citizen
        Citizen child =
            CreateChildCitizen(GetNextCitizenID(), *mother, *father, city.id);
        int childID = AddCitizen(child);
        city.citizenIDs.push_back(childID);

        // Spawn child entity in the world near city center
        float spawnX = city.center.x + (rand() % 3 - 1);
        float spawnY = city.center.y + (rand() % 3 - 1);

        // Create entity for the child
        std::vector<Entity> &entities = world.GetEntitiesMutable();
        Entity e;
        e.id = entities.size();
        e.type = EntityType::HumanUnarmed;
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

        // Use some food for the birth
        city.resources.food -= 10;

        TraceLog(
            LOG_INFO,
            "SIMULATION: Birth! Child %d born in City %d (Parents: %d, %d)",
            childID, city.id, mother->id, father->id);
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

    // === JOB ASSIGNMENT ===
    // Cities automatically assign jobs based on needs
    // Count workers by profession
    int lumberjackCount = 0;
    int farmerCount = 0;
    int idleAdultCount = 0;

    for (int citizenID : city.citizenIDs) {
      Citizen *c = GetCitizen(citizenID);
      if (!c || !c->isAlive || !c->isAdult())
        continue;

      if (c->profession == Profession::Lumberjack)
        lumberjackCount++;
      else if (c->profession == Profession::Farmer)
        farmerCount++;
      else if (c->profession == Profession::None)
        idleAdultCount++;
    }

    // Need more lumberjacks if wood is low (less than 50)
    int desiredLumberjacks = (city.resources.wood < 50) ? 2 : 1;

    // Need farmers to produce food
    int desiredFarmers = 1 + city.GetPopulation() / 5; // 1 farmer per 5 people
    if (city.resources.food < 30)
      desiredFarmers++; // Extra if food is low

    // Assign idle citizens to needed jobs
    for (int citizenID : city.citizenIDs) {
      if (idleAdultCount <= 0)
        break;

      Citizen *c = GetCitizen(citizenID);
      if (!c || !c->isAlive || !c->isAdult())
        continue;
      if (c->profession != Profession::None)
        continue;

      // Assign Lumberjacks first if wood is critical
      if (lumberjackCount < desiredLumberjacks && city.resources.wood < 30) {
        c->profession = Profession::Lumberjack;
        TraceLog(LOG_INFO,
                 "SIMULATION: Citizen %d assigned as Lumberjack in City %d",
                 c->id, city.id);
        lumberjackCount++;
        idleAdultCount--;
      }
      // Then Farmers
      else if (farmerCount < desiredFarmers) {
        c->profession = Profession::Farmer;
        TraceLog(LOG_INFO,
                 "SIMULATION: Citizen %d assigned as Farmer in City %d", c->id,
                 city.id);
        farmerCount++;
        idleAdultCount--;
      }
      // Then more Lumberjacks
      else if (lumberjackCount < desiredLumberjacks) {
        c->profession = Profession::Lumberjack;
        TraceLog(LOG_INFO,
                 "SIMULATION: Citizen %d assigned as Lumberjack in City %d",
                 c->id, city.id);
        lumberjackCount++;
        idleAdultCount--;
      }
    }

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
  }
}

void SimulationManager::UpdateKingdoms(World &world, float deltaTime) {
  for (auto &pair : kingdoms) {
    Kingdom &kingdom = pair.second;
    if (!kingdom.isAlive)
      continue;

    kingdom.age += deltaTime / secondsPerDay;

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
  // Expansion Cost
  const int COST_WOOD = 100;
  const int COST_FOOD = 100;

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

    // Pay the cost
    city.resources.wood -= COST_WOOD;
    city.resources.food -= COST_FOOD;

    TraceLog(LOG_INFO, "EXPANSION: City %s grew to (%d, %d). Size: %d",
             city.name.c_str(), (int)target.x, (int)target.y,
             (int)city.territory.size());
  }
}
