#include "../world/Entity.h"
#include "../world/Tile.h"
#include "../world/World.h"
#include "SimulationManager.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

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

  // === WAR DRAFT MECHANIC ===
  bool isAtWar = false;
  if (city.kingdomID != -1) {
    if (const Kingdom *k = GetKingdom(city.kingdomID)) {
      for (const auto &kv : k->diplomaticStatus) {
        if (kv.second == DiplomaticStatus::Hostile) {
          isAtWar = true;
          // Draft 50% of the active population for war
          desiredSoldiers = std::max(desiredSoldiers, population / 2);
          break;
        }
      }
    }
  }

  // If at war, fire non-essential workers (everyone except Farmers) to force
  // them into the draft pool.
  if (isAtWar) {
    for (int id : city.citizenIDs) {
      Citizen *c = GetCitizen(id);
      if (c && c->isAlive && c->isAdult() && !c->isFemale) {
        if (c->profession == Profession::Lumberjack ||
            c->profession == Profession::Builder ||
            c->profession == Profession::Miner) {
          // Fire them
          if (c->profession == Profession::Lumberjack)
            lumberjacks--;
          if (c->profession == Profession::Builder)
            builders--;
          if (c->profession == Profession::Miner)
            miners--;

          c->profession = Profession::None;
          c->workState = Citizen::WorkState::Idle;
          unemployed++;
          availableWorkers.push_back(c);
        }
      }
    }
  } else {
    // If NOT at war, fire excess soldiers drafted for the war!
    if (soldiers > desiredSoldiers) {
      for (int id : city.citizenIDs) {
        Citizen *c = GetCitizen(id);
        if (c && c->isAlive && c->profession == Profession::Soldier) {
          c->profession = Profession::None;
          c->workState = Citizen::WorkState::Idle;
          soldiers--;
          unemployed++;
          availableWorkers.push_back(c);
          if (soldiers <= desiredSoldiers)
            break;
        }
      }
    }
  }

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
