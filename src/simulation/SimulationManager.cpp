#include "SimulationManager.h"
#include "../world/Entity.h"
#include "../world/Tile.h"
#include "../world/World.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

SimulationManager::SimulationManager() {}

SimulationManager::~SimulationManager() {}

void SimulationManager::Reset() {
  citizens.clear();
  cities.clear();
  kingdoms.clear();
  buildings.clear();
  nextCitizenID = 1;
  nextCityID = 1;
  nextKingdomID = 1;
  nextBuildingID = 1;
  gameTime = 0.0f;
  yearTimer = 0.0f;
  currentYear = 1;
}

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
  UpdateDiplomacy(world, deltaTime);
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

void SimulationManager::RemoveCitizen(int id) {
  Citizen *c = GetCitizen(id);
  if (c && c->cityID >= 0) {
    City *city = GetCity(c->cityID);
    if (city) {
      city->citizenIDs.erase(
          std::remove(city->citizenIDs.begin(), city->citizenIDs.end(), id),
          city->citizenIDs.end());
    }
  }

  // Remove from any housing occupants list
  for (auto &pair : cities) {
    City &city = pair.second;
    for (auto &b : city.buildings) {
      b.occupants.erase(std::remove(b.occupants.begin(), b.occupants.end(), id),
                        b.occupants.end());
    }
  }

  citizens.erase(id);
}

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
// BATTLEFIELD MANAGEMENT
// ============================================================================
int SimulationManager::AddBattlefield(const Battlefield &bf) {
  Battlefield b = bf;
  if (b.id < 0)
    b.id = nextBattlefieldID++;
  battlefields[b.id] = b;
  return b.id;
}

Battlefield *SimulationManager::GetBattlefield(int id) {
  auto it = battlefields.find(id);
  return it != battlefields.end() ? &it->second : nullptr;
}

const Battlefield *SimulationManager::GetBattlefield(int id) const {
  auto it = battlefields.find(id);
  return it != battlefields.end() ? &it->second : nullptr;
}

void SimulationManager::RemoveBattlefield(int id) { battlefields.erase(id); }

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

void SimulationManager::DestroyBuildingsAtTile(int tileX, int tileY) {
  // Buildings are primarily stored in their respective City's buildings vector
  for (auto &pair : cities) {
    City &city = pair.second;

    auto it = std::remove_if(city.buildings.begin(), city.buildings.end(),
                             [tileX, tileY](const Building &b) {
                               return b.tileX == tileX && b.tileY == tileY;
                             });

    // Evict occupants and clean up before erasing completely
    for (auto bit = it; bit != city.buildings.end(); ++bit) {
      for (int citizenID : bit->occupants) {
        Citizen *c = GetCitizen(citizenID);
        if (c) {
          c->homeID = -1;
        }
      }
      buildings.erase(bit->id); // Just in case it is mirrored in global map
    }

    if (it != city.buildings.end()) {
      city.buildings.erase(it, city.buildings.end());
    }
  }

  // Clean up any remaining buildings in the global map not tied to a city
  std::vector<int> toRemoveGlobal;
  for (const auto &pair : buildings) {
    if (pair.second.tileX == tileX && pair.second.tileY == tileY) {
      toRemoveGlobal.push_back(pair.first);
    }
  }
  for (int id : toRemoveGlobal) {
    buildings.erase(id);
  }
}
