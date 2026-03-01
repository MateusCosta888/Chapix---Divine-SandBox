#include "../world/Entity.h"
#include "../world/Tile.h"
#include "../world/World.h"
#include "SimulationManager.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

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

static std::string GenerateRandomCityName() {
  const char *prefixes[] = {"Nova ", "Old ",   "Fort ",  "Port ",
                            "San ",  "Saint ", "Mount ", ""};
  const char *roots[] = {"Valer", "Gond",  "Atlant", "Cam",   "Dor",
                         "Troj",  "Babyl", "Rom",    "Athen", "Spart",
                         "Karn",  "Lorn",  "Morn",   "Riv",   "Val"};
  const char *suffixes[] = {"ia", "or",   "is",   "elot",  "ado",
                            "y",  "grad", "burg", "ville", "ton"};

  std::string name = "";
  if (rand() % 4 == 0) {
    name += prefixes[rand() % (sizeof(prefixes) / sizeof(prefixes[0]))];
  }
  name += roots[rand() % (sizeof(roots) / sizeof(roots[0]))];
  name += suffixes[rand() % (sizeof(suffixes) / sizeof(suffixes[0]))];
  return name;
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
                         GenerateRandomCityName(), cityColor);
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
// UPDATE CITIES
// ============================================================================
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
      // Clean-up operation: Purge phantom structures and release map territory
      for (const Vector2 &tpos : city.territory) {
        int tx = static_cast<int>(tpos.x);
        int ty = static_cast<int>(tpos.y);
        Tile &mapTile = world.GetTile(tx, ty);

        // Surrender the ground back to wild nature
        if (mapTile.ownerCityID == city.id)
          mapTile.ownerCityID = -1;
        if (mapTile.farmOwnerCityID == city.id) {
          mapTile.farmOwnerCityID = -1;
          mapTile.isPlanted = false; // Destroy phantom crops
          mapTile.growthProgress = 0.0f;
        }
      }

      for (const Building &b : city.buildings) {
        Tile &buildTile = world.GetTile(b.tileX, b.tileY);
        buildTile.isOccupied = false; // Free spatial physics for new settlers
      }

      city.territory.clear();
      city.buildings.clear();
      city.isAlive = false;

      TraceLog(LOG_INFO,
               "SIMULATION: City %d has fallen! Territory and architecture "
               "purged from the simulation.",
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
