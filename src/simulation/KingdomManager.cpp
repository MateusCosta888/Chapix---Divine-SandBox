#include "../world/Entity.h"
#include "../world/Tile.h"
#include "../world/World.h"
#include "SimulationManager.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

// ============================================================================
// UPDATE KINGDOMS
// ============================================================================
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

      // BROADCAST DEATH TO OTHER KINGDOMS (Erase memory of war)
      for (auto &otherK_pair : kingdoms) {
        if (otherK_pair.second.id != kingdom.id) {
          otherK_pair.second.diplomaticStatus.erase(kingdom.id);
          otherK_pair.second.relations.erase(kingdom.id);
        }
      }
    }
  }

  // ==========================================
  // BATTLEFIELD UPDATES
  // ==========================================
  std::vector<int> battlefieldsToRemove;
  for (auto &bfPair : battlefields) {
    Battlefield &bf = bfPair.second;
    bf.timer -= deltaTime;

    if (bf.timer <= 0.0f) {
      // BATTLE IS OVER!
      Kingdom *kA = GetKingdom(bf.kingdomA);
      Kingdom *kB = GetKingdom(bf.kingdomB);

      if (kA && kB) {
        // Make peace
        kA->diplomaticStatus[kB->id] = DiplomaticStatus::Neutral;
        kB->diplomaticStatus[kA->id] = DiplomaticStatus::Neutral;
        kA->SetRelation(kB->id, 0.0f);
        kB->SetRelation(kA->id, 0.0f);

        if (bf.killsA > bf.killsB) {
          TraceLog(LOG_INFO,
                   "BATTLE ENDED: %s WON against %s! (%d vs %d kills)",
                   kA->name.c_str(), kB->name.c_str(), bf.killsA, bf.killsB);
        } else if (bf.killsB > bf.killsA) {
          TraceLog(LOG_INFO,
                   "BATTLE ENDED: %s WON against %s! (%d vs %d kills)",
                   kB->name.c_str(), kA->name.c_str(), bf.killsB, bf.killsA);
        } else {
          TraceLog(LOG_INFO,
                   "BATTLE ENDED: DRAW between %s and %s! (%d kills each)",
                   kA->name.c_str(), kB->name.c_str(), bf.killsA);
        }
      }
      battlefieldsToRemove.push_back(bf.id);
    }
  }

  for (int id : battlefieldsToRemove) {
    RemoveBattlefield(id);
  }
}

// ============================================================================
// DIPLOMACY UPDATE (Autonomous War Declarations)
// ============================================================================
void SimulationManager::UpdateDiplomacy(World &world, float deltaTime) {
  static float diplomacyTimer = 0.0f;
  diplomacyTimer += deltaTime;

  // Only evaluate diplomacy every 10 seconds to save CPU and simulate slower
  // politics
  if (diplomacyTimer < 10.0f)
    return;
  diplomacyTimer = 0.0f;

  for (auto &pairA : kingdoms) {
    if (!pairA.second.isAlive)
      continue;
    Kingdom &kA = pairA.second;

    for (auto &pairB : kingdoms) {
      if (!pairB.second.isAlive || pairA.first == pairB.first)
        continue;
      Kingdom &kB = pairB.second;

      // Skip if already at war or allied
      if (kA.IsAtWarWith(kB.id) || kA.IsAlliedWith(kB.id))
        continue;

      // Calculate border distance (shortest distance between any two cities of
      // these kingdoms)
      float minSqDist = 9999999.0f;
      int foodA = 0;
      int foodB = 0;

      // Aggregate Resources and gather positions
      for (int cityIdA : kA.cityIDs) {
        if (const City *cA = GetCity(cityIdA)) {
          foodA += cA->resources.food;
          for (int cityIdB : kB.cityIDs) {
            if (const City *cB = GetCity(cityIdB)) {
              foodB += cB->resources.food;
              float dx = cA->center.x - cB->center.x;
              float dy = cA->center.y - cB->center.y;
              float sqDist = (dx * dx) + (dy * dy);
              if (sqDist < minSqDist)
                minSqDist = sqDist;
            }
          }
        }
      }

      // 1. BORDER FRICTION
      // If borders are touching/cities are nearby (e.g., < 40 tiles away -> <
      // 1600 sqDist)
      if (minSqDist < 1600.0f) {
        float friction = kA.totalAggression * 10.0f;
        kA.ModifyRelation(kB.id, -friction);
      }

      // 2. RESOURCE ENVY (Hunger Mechanics)
      // If kingdom A is starving and kingdom B is rich
      if (foodA < 20 && foodB > 100) {
        float envy = kA.totalAggression * 15.0f;
        kA.ModifyRelation(kB.id, -envy);
      }

      // Default cooldown decay back to 0 (slowly cooling off)
      float currentRel = kA.GetRelation(kB.id);
      if (currentRel < 0.0f)
        kA.ModifyRelation(kB.id, 1.0f);
      else if (currentRel > 0.0f)
        kA.ModifyRelation(kB.id, -1.0f);

      // PEACE DECLARATION (Armistice)
      if (currentRel >= 0.0f &&
          kA.diplomaticStatus[kB.id] == DiplomaticStatus::Hostile) {
        kA.diplomaticStatus[kB.id] = DiplomaticStatus::Neutral;
        kB.diplomaticStatus[kA.id] = DiplomaticStatus::Neutral;
        TraceLog(LOG_INFO, "PEACE SIGNED: %s and %s ended their war!",
                 kA.name.c_str(), kB.name.c_str());
      }

      // WAR DECLARATION TRIGGER
      if (kA.GetRelation(kB.id) <= -50.0f && kA.totalAggression > 0.3f) {
        DeclareWar(kA.id, kB.id, world);
      }
    }
  }
}

// ============================================================================
// DECLARE WAR (Updates Internal Diplomatic Maps)
// ============================================================================
void SimulationManager::DeclareWar(int kingdomA, int kingdomB, World &world) {
  Kingdom *kA = GetKingdom(kingdomA);
  Kingdom *kB = GetKingdom(kingdomB);
  if (!kA || !kB)
    return;

  // Set the relations bilaterally to -100 (locked in war)
  kA->SetRelation(kingdomB, -100.0f);
  kB->SetRelation(kingdomA, -100.0f);

  // Alter exact state
  kA->diplomaticStatus[kingdomB] = DiplomaticStatus::Hostile;
  kB->diplomaticStatus[kingdomA] = DiplomaticStatus::Hostile;

  // --- BATTLEFIELD GENERATION ---
  City *cityA = GetCity(kA->capitalCityID);
  City *cityB = GetCity(kB->capitalCityID);
  if (!cityA && !kA->cityIDs.empty())
    cityA = GetCity(kA->cityIDs[0]);
  if (!cityB && !kB->cityIDs.empty())
    cityB = GetCity(kB->cityIDs[0]);

  if (cityA && cityB) {
    Vector2 midPos = {(cityA->center.x + cityB->center.x) / 2.0f,
                      (cityA->center.y + cityB->center.y) / 2.0f};

    int cx = (int)midPos.x;
    int cy = (int)midPos.y;
    int bestX = cx;
    int bestY = cy;
    float minDiff = 999999.0f;
    int searchRadius = 25; // Wide range to escape water lakes

    for (int y = cy - searchRadius; y <= cy + searchRadius; y++) {
      for (int x = cx - searchRadius; x <= cx + searchRadius; x++) {
        if (x >= 0 && x < world.GetWidth() && y >= 0 && y < world.GetHeight()) {
          const Tile &t = world.GetTileConst(x, y);
          if (t.type == TileType::Grass || t.type == TileType::Sand ||
              t.type == TileType::Forest || t.type == TileType::DesertSand) {
            if (!t.isOccupied) { // Find open terrain
              float dist = std::hypot(x - cx, y - cy);
              if (dist < minDiff) {
                minDiff = dist;
                bestX = x;
                bestY = y;
              }
            }
          }
        }
      }
    }

    midPos.x = (float)bestX;
    midPos.y = (float)bestY;

    Battlefield bf;
    bf.kingdomA = kA->id;
    bf.kingdomB = kB->id;
    bf.centerPos = midPos;
    bf.timer = 60.0f; // 60 Real Seconds War Clock
    bf.killsA = 0;
    bf.killsB = 0;
    AddBattlefield(bf);
    TraceLog(LOG_INFO, "BATTLEFIELD Spawned at (%.1f, %.1f) between %s and %s",
             midPos.x, midPos.y, kA->name.c_str(), kB->name.c_str());
  }

  TraceLog(LOG_INFO, "WAR DECLARED: %s declared war on %s!", kA->name.c_str(),
           kB->name.c_str());
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
