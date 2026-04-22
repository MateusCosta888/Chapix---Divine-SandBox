#include "../world/Entity.h"
#include "../world/Tile.h"
#include "../world/World.h"
#include "SimulationManager.h"
#include "../utils/GlobalRandom.h"
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

      // Trees are resources (Include Pine and Palm)
      if (tile.decoration == DecorationType::Tree ||
          tile.decoration == DecorationType::PineTree ||
          tile.decoration == DecorationType::PalmTree)
        treeCount++;

      // Fertile land (Grass, Snow, Sand, Forest)
      if (tile.type == TileType::Grass || tile.type == TileType::Snow ||
          tile.type == TileType::DesertSand || tile.type == TileType::Sand ||
          tile.type == TileType::Forest)
        grassCount++;
    }
  }

  // Score calculation
  score += treeCount * 5.0f;  // Trees for wood
  score += grassCount * 3.0f; // Fertile land
  if (waterCount > 0) score += 10.0f; // Small bonus for water

  // Penalty for being too close to other cities
  for (const auto &pair : cities) {
    float dist = std::hypot(pair.second.center.x - x, pair.second.center.y - y);
    if (dist < 18.0f) {
      score -= (18.0f - dist) * 20.0f; // Heavy penalty for proximity
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
  if (GRandom.Chance(25)) {
    int numPrefixes = sizeof(prefixes) / sizeof(prefixes[0]);
    name += prefixes[GRandom.Int(0, numPrefixes - 1)];
  }
  int numRoots = sizeof(roots) / sizeof(roots[0]);
  int numSuffixes = sizeof(suffixes) / sizeof(suffixes[0]);
  name += roots[GRandom.Int(0, numRoots - 1)];
  name += suffixes[GRandom.Int(0, numSuffixes - 1)];
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

  // Initial resources to help start the city - Boosted
  cities[cityID].resources.wood = 25;
  cities[cityID].resources.food = 50;
  cities[cityID].resources.stone = 5;

  // === KINGDOM ASSIGNMENT ===
  int bestKingdomID = -1;
  float bestDist = 150.0f; // Join existing kingdom if within 150 units

  for (const auto &pair : kingdoms) {
    const Kingdom &k = pair.second;
    if (!k.isAlive) continue;
    float d = std::hypot(k.center.x - x, k.center.y - y);
    if (d < bestDist) {
      bestDist = d;
      bestKingdomID = k.id;
    }
  }

  if (bestKingdomID != -1) {
    cities[cityID].kingdomID = bestKingdomID;
    kingdoms[bestKingdomID].cityIDs.push_back(cityID);
  } else {
    // Create new kingdom
    Kingdom newK;
    newK.id = GetNextKingdomID();
    newK.name = "Kingdom of " + cities[cityID].name;
    newK.color = cities[cityID].color;
    newK.center = cities[cityID].center;
    newK.capitalCityID = cityID;
    newK.cityIDs.push_back(cityID);
    newK.isAlive = true;
    kingdoms[newK.id] = newK;
    cities[cityID].kingdomID = newK.id;
  }

  // Add a starting cabana at city center - NOT FREE, must be built
  Building cabana;
  cabana.id = static_cast<int>(cities[cityID].buildings.size());
  cabana.type = BuildingType::Cabana;
  cabana.tileX = x;
  cabana.tileY = y;
  cabana.cityID = cityID;
  cabana.variant = 0;
  cabana.isComplete = false;
  cabana.constructionProgress = 0.1f; // 10% started
  cities[cityID].buildings.push_back(cabana);
  world.GetTile(x, y).isOccupied = true; // Mark as occupied

  // Update housing capacity
  cities[cityID].populationCap =
      2 + GetBuildingHousingCapacity(BuildingType::Cabana);

  // Give city some starting resources for a fast start
  cities[cityID].resources.food = 60;
  cities[cityID].resources.wood = 40;
  cities[cityID].resources.stone = 10;

  // Add a few more settlers to jumpstart the city
  for (int i = 0; i < 3; i++) {
    Vector2 p = {x + GRandom.Float() * 2 - 1, y + GRandom.Float() * 2 - 1};
    if (world.IsWalkable((int)p.x, (int)p.y)) {
      world.AddEntity(EntityType::HumanUnarmed, p, false);
      // The AddEntity logic will automatically join them to this city if close
    }
  }

  TraceLog(LOG_INFO, "CITY: Founded with free Cabana at (%d,%d), popCap = %d, Starting Settlers added",
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
  static float globalBirthScanTimer = 0.0f;
  globalBirthScanTimer += deltaTime;
  bool doBirthCheck = globalBirthScanTimer >= 5.0f;
  if (doBirthCheck)
    globalBirthScanTimer = 0.0f;

  for (auto &pair : cities) {
    City &city = pair.second;
    if (!city.isAlive)
      continue;

    city.age += deltaTime / secondsPerYear; // Age in years

    // === FOOD PRODUCTION ===
    // Passive food generation based on territory (farms, foraging, etc.)
    city.foodTimer += deltaTime;
    if (city.foodTimer >= 10.0f) {
      city.foodTimer = 0.0f;
      // Each city produces some food passively (simulating gathering/farming)
      int baseProduction = 10 + city.GetPopulation() / 2;
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

    // Dynamic reproduction scaling
    // Timer increases with population: 25s at start, +0.5s per person, max 60s
    float birthInterval = std::min(60.0f, 25.0f + (city.GetPopulation() * 0.5f));
    
    // Dynamic housing cap: base 10, plus 6 per building (reduced from 8)
    int housingCap = 10 + static_cast<int>(city.buildings.size()) * 6;

    city.birthTimer += deltaTime;
    if (city.birthTimer >= birthInterval &&
        city.resources.food >= (20 + city.GetPopulation() / 2) &&
        city.GetPopulation() < housingCap) {
      city.birthTimer = 0.0f;

      // Find a female and male adult citizen for parents
      Citizen *mother = nullptr;
      Citizen *father = nullptr;

      for (int citizenID : city.citizenIDs) {
        Citizen *c = GetCitizen(citizenID);
        // Lenient requirements for parents (Removed energy/hunger for debug/growth)
        if (c && c->isAlive && c->isAdult() && !c->isElder()) {
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
        float spawnX = city.center.x + (float)GRandom.Int(-1, 1);
        float spawnY = city.center.y + (float)GRandom.Int(-1, 1);

        auto &entities = world.GetEntitiesMutable();
        Entity e;
        e.id = world.GenerateEntityID();
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
        entities[e.id] = e;
        world.RebuildEntityCache();

        // Cost for birth scales with population
        int birthCost = std::min(100, 20 + city.GetPopulation() / 2);
        city.resources.food -= birthCost;

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

    // City dies if no citizens left AND it has existed for a while (grace period)
    // This prevents newly founded cities from vanishing if the founder dies instantly
    if (city.citizenIDs.empty() && city.age > 120.0f) { 
      city.isAlive = false;
      TraceLog(LOG_INFO, "SIMULATION: City %d has fallen! (No citizens remain after grace period)",
               city.id);

      // === CLEANUP: Remove ghost buildings and free tiles ===
      // 1. Clear isOccupied for all building tiles
      for (const auto &b : city.buildings) {
        BuildingSize size = GetBuildingSize(b.type);
        for (int dy = 0; dy < size.height; dy++) {
          for (int dx = 0; dx < size.width; dx++) {
            int tx = b.tileX + dx;
            int ty = b.tileY + dy;
            if (tx >= 0 && tx < world.GetWidth() && ty >= 0 && ty < world.GetHeight()) {
              world.GetTile(tx, ty).isOccupied = false;
            }
          }
        }
      }
      city.buildings.clear();

      // 2. Release territory ownership and clear farms
      for (const Vector2 &tile : city.territory) {
        int tx = static_cast<int>(tile.x);
        int ty = static_cast<int>(tile.y);
        if (tx >= 0 && tx < world.GetWidth() && ty >= 0 && ty < world.GetHeight()) {
          Tile &t = world.GetTile(tx, ty);
          if (t.ownerCityID == city.id) {
            t.ownerCityID = -1;
          }
          if (t.farmOwnerCityID == city.id) {
            t.isPlanted = false;
            t.growthProgress = 0.0f;
            t.farmOwnerCityID = -1;
          }
        }
      }
      city.territory.clear();
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
    city.buildTimer += deltaTime;
    if (city.buildTimer >= 2.0f) {
      city.buildTimer = 0.0f;
      AttemptConstruction(city, world);
    }

    // === BUILDING EVOLUTION (UPGRADES) ===
    city.upgradeTimer += deltaTime;
    if (city.upgradeTimer >= 3.0f) {
      city.upgradeTimer = 0.0f;
      UpdateBuildingUpgrade(city);
    }

    // === TERRITORY EXPANSION ===
    AttemptTerritoryExpansion(city, world);

    // === PASSIVE STONE GENERATION (MINES) ===
    city.mineTimer += deltaTime;
    if (city.mineTimer >= 3.0f) { // Every 3 seconds
      city.mineTimer = 0.0f;
      int stoneGen = 0;
      int workshopCount = 0;
      for (const auto &b : city.buildings) {
        if (b.isComplete && b.type == BuildingType::Mina) {
          stoneGen += (b.variant + 1) * 2;
        }
        if (b.isComplete && b.type == BuildingType::Workshop)
          workshopCount++;
      }
      if (workshopCount > 0 && stoneGen > 0) {
        stoneGen = (int)(stoneGen * (1.0f + workshopCount * 0.2f));
      }
      if (stoneGen > 0) {
        city.resources.stone += stoneGen;
        if (city.resources.stone > city.maxStorage)
          city.resources.stone = city.maxStorage;
      }
    }

  }
} // Close UpdateCities
