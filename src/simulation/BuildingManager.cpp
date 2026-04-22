#include "../world/Entity.h"
#include "../world/Tile.h"
#include "../world/World.h"
#include "SimulationManager.h"
#include "../utils/GlobalRandom.h"
#include <algorithm>
#include <cmath>

// ============================================================================
// ASSIGN JOBS -Government system to assign professions based on city needs
// ============================================================================
void SimulationManager::AssignJobs(City &city) {
  AssignHousing(city);

  int population = city.GetPopulation();
  if (population == 0) return;

  // Count current workers
  int lumberjacks = 0, farmers = 0, miners = 0, builders = 0, soldiers = 0;
  std::vector<Citizen *> availableWorkers;

  for (int id : city.citizenIDs) {
    Citizen *c = GetCitizen(id);
    if (!c || !c->isAlive || !c->isAdult()) continue;

    switch (c->profession) {
    case Profession::Lumberjack: lumberjacks++; break;
    case Profession::Farmer: farmers++; break;
    case Profession::Miner: miners++; break;
    case Profession::Builder: builders++; break;
    case Profession::Soldier: soldiers++; break;
    case Profession::None:
      availableWorkers.push_back(c);
      break;
    default: break;
    }
  }

  if (availableWorkers.empty()) return;

  // Calculate needs based on city state
  bool hasIncompleteBuildings = false;
  for (const auto &b : city.buildings) {
    if (!b.isComplete) { hasIncompleteBuildings = true; break; }
  }

  int desiredFarmers = std::max(1, population / 4);     // 1 per 4 pop
  int desiredLumberjacks = std::max(1, population / 4); // 1 per 4 pop
  int desiredMiners = std::max(0, population / 8);      // Stone needed later
  
  // DEADLOCK FIX: Cap builders so we always have resource gatherers
  int desiredBuilders = 0;
  if (hasIncompleteBuildings) {
    if (population <= 3) desiredBuilders = 1;
    else desiredBuilders = std::max(1, (int)(population * 0.3f)); // Max 30% are builders
  }

  int barracksCount = 0;
  for (const auto &b : city.buildings) {
    if (b.isComplete && b.type == BuildingType::Quartel) barracksCount++;
  }
  int desiredSoldiers = barracksCount * 3;

  // Emergency needs
  if (city.resources.food < 30) desiredFarmers += 1;
  if (city.resources.wood < 20) desiredLumberjacks += 2; // Lumber priority
  if (city.resources.stone < 10 && hasIncompleteBuildings) desiredMiners += 1;

  // MIGRATION LOGIC: If overcrowded, someone becomes a settler
  bool spawnedSettler = false;
  if (population > 15 && GRandom.Chance(10)) { // 10% chance per assign cycle
     spawnedSettler = true;
  }

  // Assign jobs by distribution
  for (Citizen *c : availableWorkers) {
    if (spawnedSettler) {
      c->cityID = -1; // Leave city to become a settler!
      c->profession = Profession::None; 
      spawnedSettler = false;
      continue;
    }

    // Priority: Lumberjacks (can't build without wood!)
    if (lumberjacks < desiredLumberjacks) {
      c->profession = Profession::Lumberjack;
      lumberjacks++;
    } else if (farmers < desiredFarmers) {
      c->profession = Profession::Farmer;
      farmers++;
    } else if (builders < desiredBuilders) {
      c->profession = Profession::Builder;
      builders++;
    } else if (miners < desiredMiners) {
      c->profession = Profession::Miner;
      miners++;
    } else if (soldiers < desiredSoldiers) {
      c->profession = Profession::Soldier;
      soldiers++;
    } else {
      // Default to what's most needed
      int roll = GRandom.Int(0, 2);
      if (roll == 0) c->profession = Profession::Farmer;
      else if (roll == 1) c->profession = Profession::Lumberjack;
      else c->profession = Profession::Builder;
    }
  }
}

// ============================================================================
// BUILDING CONSTRUCTION - Updated to use city-centered placement
// ============================================================================
void SimulationManager::AttemptConstruction(City &city, World &world) {
  // 1. Calculate storage and capacity
  int totalStorage = 200;
  int plannedCapacity = 0;

  for (const auto &b : city.buildings) {
    if (b.isComplete) {
      if (b.type == BuildingType::Recursos || b.type == BuildingType::StockpileStone)
        totalStorage += 500;
    }
    if (b.IsHousing()) {
      int cap = b.capacity > 0 ? b.capacity : GetBuildingHousingCapacity(b.type);
      if (cap == 0) cap = 2;
      plannedCapacity += cap;
    }
  }
  city.maxStorage = totalStorage;

  int population = city.GetPopulation();

  // 2. Determine what to build
  bool needHousing = population >= plannedCapacity;
  bool needWoodStorage = city.resources.wood >= (city.maxStorage * 0.85f);
  bool needStoneStorage = city.resources.stone >= (city.maxStorage * 0.85f);
  bool needMine = city.resources.stone < 30;

  BuildingType typeToBuild = BuildingType::None;
  int woodCost = 0;
  int stoneCost = 0;

  // Priority 1: Housing (most important)
  if (needHousing && city.resources.wood >= 5) {
    if (city.buildings.size() < 3) {
      typeToBuild = BuildingType::Cabana;
      woodCost = 2; stoneCost = 0;
    } else if (city.buildings.size() < 8) {
      typeToBuild = BuildingType::Casa;
      woodCost = 5; stoneCost = 2;
    } else {
      typeToBuild = BuildingType::Casa2;
      woodCost = 8; stoneCost = 5;
    }
  }
  // Priority 2: Storage
  else if (needStoneStorage && city.resources.wood >= 25) {
    typeToBuild = BuildingType::StockpileStone;
    woodCost = 25; stoneCost = 0;
  }
  // Priority 3: Mines (if stone is low)
  else if (needMine && city.resources.wood >= 30) {
    typeToBuild = BuildingType::Mina;
    woodCost = 30; stoneCost = 0;
  }
  // Priority 4: Wood storage
  else if (needWoodStorage && city.resources.wood >= 20) {
    typeToBuild = BuildingType::Recursos;
    woodCost = 20; stoneCost = 0;
  }

  // Priority 5: Population-gated buildings
  if (typeToBuild == BuildingType::None) {
    int quartelCount = 0, mercadoCount = 0, workshopCount = 0, tavernaCount = 0, casteloCount = 0;
    for (const auto &b : city.buildings) {
      if (b.type == BuildingType::Quartel) quartelCount++;
      else if (b.type == BuildingType::Mercado) mercadoCount++;
      else if (b.type == BuildingType::Workshop) workshopCount++;
      else if (b.type == BuildingType::Taverna) tavernaCount++;
      else if (b.type == BuildingType::Castelo) casteloCount++;
    }

    // Barracks: 1 when pop >= 15
    if (quartelCount == 0 && population >= 15) {
      BuildingCost cost = GetBuildingCost(BuildingType::Quartel);
      if (city.resources.wood >= cost.wood && city.resources.stone >= cost.stone) {
        typeToBuild = BuildingType::Quartel;
        woodCost = cost.wood; stoneCost = cost.stone;
      }
    }
    // Market: 1 per 30 pop
    else if (mercadoCount < (population / 30) + 1 && population >= 10) {
      BuildingCost cost = GetBuildingCost(BuildingType::Mercado);
      if (city.resources.wood >= cost.wood && city.resources.stone >= cost.stone) {
        typeToBuild = BuildingType::Mercado;
        woodCost = cost.wood; stoneCost = cost.stone;
      }
    }
    // Workshop: 1 per 25 pop
    else if (workshopCount < (population / 25) + 1 && population >= 12) {
      BuildingCost cost = GetBuildingCost(BuildingType::Workshop);
      if (city.resources.wood >= cost.wood && city.resources.stone >= cost.stone) {
        typeToBuild = BuildingType::Workshop;
        woodCost = cost.wood; stoneCost = cost.stone;
      }
    }
    // Tavern: 1 per 20 pop
    else if (tavernaCount < (population / 20) + 1 && population >= 8) {
      BuildingCost cost = GetBuildingCost(BuildingType::Taverna);
      if (city.resources.wood >= cost.wood && city.resources.stone >= cost.stone) {
        typeToBuild = BuildingType::Taverna;
        woodCost = cost.wood; stoneCost = cost.stone;
      }
    }
    // Castle: max 1, pop >= 40
    else if (casteloCount == 0 && population >= 40) {
      BuildingCost cost = GetBuildingCost(BuildingType::Castelo);
      if (city.resources.wood >= cost.wood && city.resources.stone >= cost.stone) {
        typeToBuild = BuildingType::Castelo;
        woodCost = cost.wood; stoneCost = cost.stone;
      }
    }
  }

  if (typeToBuild == BuildingType::None || city.resources.wood < woodCost)
    return;

  // 3. Find placement (use city center as primary reference)
  int bestBx = -1, bestBy = -1;
  float bestScore = 999999.0f;

  BuildingSize size = GetBuildingSize(typeToBuild);
  int centerBx = static_cast<int>(city.center.x);
  int centerBy = static_cast<int>(city.center.y);

  // Search in expanding rings around city center
  for (int ring = 2; ring <= 8 && bestBx == -1; ring++) {
    for (int dy = -ring; dy <= ring && bestBx == -1; dy++) {
      for (int dx = -ring; dx <= ring && bestBx == -1; dx++) {
        // Only check perimeter of ring
        if (dx > -ring && dx < ring && dy > -ring && dy < ring) continue;

        int bx = centerBx + dx;
        int by = centerBy + dy;

        if (bx < 0 || by < 0 || bx >= world.GetWidth() || by >= world.GetHeight())
          continue;

        // Check if all tiles are free and walkable
        bool placeable = true;
        for (int bdy = 0; bdy < size.height && placeable; bdy++) {
          for (int bdx = 0; bdx < size.width && placeable; bdx++) {
            int tx = bx + bdx;
            int ty = by + bdy;
            if (tx < 0 || ty < 0 || tx >= world.GetWidth() || ty >= world.GetHeight()) {
              placeable = false;
            } else {
              const Tile &tile = world.GetTileConst(tx, ty);
              if (tile.isOccupied || tile.type == TileType::DeepOcean ||
                  tile.type == TileType::Ocean || tile.type == TileType::ShallowOcean ||
                  tile.type == TileType::Mountain) {
                placeable = false;
              }
            }
          }
        }

        if (placeable) {
          float dist = std::hypot(bx - centerBx, by - centerBy);
          float score = dist + GRandom.FloatRange(0.0f, 1.0f);
          if (score < bestScore) {
            bestScore = score;
            bestBx = bx;
            bestBy = by;
          }
        }
      }
    }
  }

  if (bestBx != -1) {
    // Deduct costs
    city.resources.wood -= woodCost;
    city.resources.stone -= stoneCost;

    // Create building
    Building newBuilding;
    newBuilding.id = GetNextBuildingID();
    newBuilding.cityID = city.id;
    newBuilding.tileX = bestBx;
    newBuilding.tileY = bestBy;
    newBuilding.isComplete = false;
    newBuilding.constructionProgress = 0.0f;
    newBuilding.type = typeToBuild;
    newBuilding.variant = GRandom.Int(0, 2);
    newBuilding.capacity = GetBuildingHousingCapacity(typeToBuild);

    city.buildings.push_back(newBuilding);

    // Mark tiles as occupied
    for (int bdy = 0; bdy < size.height; bdy++) {
      for (int bdx = 0; bdx < size.width; bdx++) {
        int tx = bestBx + bdx;
        int ty = bestBy + bdy;
        if (tx >= 0 && tx < world.GetWidth() && ty >= 0 && ty < world.GetHeight()) {
          world.GetTile(tx, ty).isOccupied = true;
          world.GetTile(tx, ty).decoration = DecorationType::None;
        }
      }
    }

    TraceLog(LOG_INFO, "CITY %d: Started construction of %s at (%d,%d). "
             "Wood: %d/%d, Stone: %d",
             city.id, GetBuildingName(typeToBuild), bestBx, bestBy,
             city.resources.wood, city.maxStorage, city.resources.stone);
  } else {
    TraceLog(LOG_INFO, "CITY %d: Failed to place %s! No space around center (%d,%d)",
             city.id, GetBuildingName(typeToBuild), centerBx, centerBy);
  }
}

// ============================================================================
// HOUSING SYSTEM
// ============================================================================
int GetHousingCapacity(const City &city) {
  int totalCapacity = 0;
  for (const auto &b : city.buildings) {
    if (b.isComplete && b.IsHousing()) {
      totalCapacity += b.capacity > 0 ? b.capacity : GetBuildingHousingCapacity(b.type);
    }
  }
  return totalCapacity;
}

void SimulationManager::AssignHousing(City &city) {
  // Find homeless citizens
  std::vector<Citizen *> homeless;
  for (int id : city.citizenIDs) {
    Citizen *c = GetCitizen(id);
    if (c && c->isAlive && c->homeID == -1) {
      homeless.push_back(c);
    }
  }

  if (homeless.empty()) return;

  // Find available housing
  for (auto &b : city.buildings) {
    if (b.isComplete && b.IsHousing()) {
      if (b.capacity == 0) {
        if (b.type == BuildingType::Cabana) b.capacity = 3;
        else if (b.type == BuildingType::Casa) b.capacity = 5;
        else if (b.type == BuildingType::Casa2) b.capacity = 8;
        else b.capacity = 2;
      }

      while (b.occupants.size() < (size_t)b.capacity && !homeless.empty()) {
        Citizen *c = homeless.back();
        homeless.pop_back();
        c->homeID = b.id;
        b.occupants.push_back(c->id);
      }
    }
  }
}

// ============================================================================
// BUILDING EVOLUTION / UPGRADES - Deterministic (no random chance)
// ============================================================================
void SimulationManager::UpdateBuildingUpgrade(City &city) {
  // Must have resources
  if (city.resources.wood < 10 && city.resources.stone < 5)
    return;

  // Process all buildings
  int maxUpgrades = 3; // Allow up to 3 upgrades per tick to speed up evolution
  int upgradesDone = 0;

  for (auto &b : city.buildings) {
    if (!b.isComplete || upgradesDone >= maxUpgrades) continue;

    // Upgrade Cabana -> Casa
    if (b.type == BuildingType::Cabana) {
      int costWood = 10;
      if (city.resources.wood >= costWood) {
        city.resources.wood -= costWood;
        b.type = BuildingType::Casa;
        b.capacity = 5;
        b.variant = GRandom.Int(0, 5);
        upgradesDone++;
        TraceLog(LOG_INFO, "CITY %d: Upgraded Cabana to Casa (Var %d). Cap: 5",
                 city.id, b.variant);
      }
    }
    // Upgrade Casa
    else if (b.type == BuildingType::Casa) {
      bool isWoodCasa = (b.variant <= 5);
      bool isMixedCasa = (b.variant >= 6 && b.variant <= 8);

      if (isWoodCasa) {
        int costWood = 15, costStone = 5;
        if (city.resources.wood >= costWood && city.resources.stone >= costStone) {
          city.resources.wood -= costWood;
          city.resources.stone -= costStone;
          b.variant = 6 + GRandom.Int(0, 2);
          b.capacity = 5;
          upgradesDone++;
          TraceLog(LOG_INFO, "CITY %d: Upgraded Wood Casa to Mixed Casa (Var %d). Cap: 5",
                   city.id, b.variant);
        }
      } else if (isMixedCasa) {
        int costStone = 50;
        if (city.resources.stone >= costStone) {
          city.resources.stone -= costStone;
          b.type = BuildingType::Casa2;
          b.capacity = 8;
          b.variant = GRandom.Int(0, 2);
          upgradesDone++;
          TraceLog(LOG_INFO, "CITY %d: Upgraded Mixed Casa to Mansion (Var %d). Cap: 8",
                   city.id, b.variant);
        }
      }
    }
    // Upgrade Mine
    else if (b.type == BuildingType::Mina) {
      if (b.variant < 5) {
        int currentLevel = b.variant + 1;
        int costWood = 20 * currentLevel;
        int costStone = 10 * currentLevel;

        if (city.resources.wood >= costWood && city.resources.stone >= costStone) {
          city.resources.wood -= costWood;
          city.resources.stone -= costStone;
          b.variant++;
          upgradesDone++;
          TraceLog(LOG_INFO, "CITY %d: Upgraded Mine to Level %d (Var %d). Cost: %dW %dS",
                   city.id, b.variant + 1, b.variant, costWood, costStone);
        }
      }
    }
  }
}
