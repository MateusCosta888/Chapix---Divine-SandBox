#include "../world/Entity.h"
#include "../world/Tile.h"
#include "../world/World.h"
#include "SimulationManager.h"
#include "../utils/GlobalRandom.h"
#include <algorithm>
#include <cmath>


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
      continue;
    }

    // === WAR SYSTEM: Periodic evaluation ===
    kingdom.warCheckTimer += deltaTime;
    if (kingdom.warCheckTimer >= kingdom.warCooldown) {
      kingdom.warCheckTimer = 0.0f;

      // Evaluate relations with other kingdoms
      for (auto &otherPair : kingdoms) {
        Kingdom &other = otherPair.second;
        if (!other.isAlive || other.id == kingdom.id)
          continue;

        float relation = kingdom.GetRelation(other.id);

        // Check for war declaration
        if (!kingdom.IsAtWarWith(other.id)) {
          // Natural war chance when relations are very hostile
          if (relation < -50.0f) {
            float chance = (-relation / 100.0f) * 0.05f;
            float roll = GRandom.Float();
            if (roll < chance) {
              ForceDeclareWar(kingdom.id, other.id);
              TraceLog(LOG_INFO,
                       "WAR: Natural war erupted between %s and %s (rel: %.1f, chance %.3f)",
                       kingdom.name.c_str(), other.name.c_str(), relation,
                       chance);
              continue;
            }
          }

          // Calculate average aggression from cities
          float avgAggression = 0.0f;
          for (int cityID : kingdom.cityIDs) {
            City *c = GetCity(cityID);
            if (c)
              avgAggression += c->cultureAggression;
          }
          if (!kingdom.cityIDs.empty())
            avgAggression /= kingdom.cityIDs.size();

          // Declare war if aggressive and hostile relations
          if (avgAggression > 0.5f && relation < -30.0f) {
            kingdom.DeclareWar(other.id);
            other.DeclareWar(kingdom.id);
            TraceLog(LOG_INFO,
                     "WAR: Kingdom %s declared war on Kingdom %s! (rel: %.1f)",
                     kingdom.name.c_str(), other.name.c_str(), relation);
          }
        } else {
          // Check peace conditions
          // Peace if relation improves above 0
          if (relation > 0.0f) {
            kingdom.DeclarePeace(other.id);
            other.DeclarePeace(kingdom.id);
            TraceLog(LOG_INFO, "PEACE: Kingdom %s and %s made peace (relation improved)",
                     kingdom.name.c_str(), other.name.c_str());
            continue;
          }

          // Peace if one side lost 50%+ population
          int myPop = 0, enemyPop = 0;
          for (int cityID : kingdom.cityIDs) {
            City *c = GetCity(cityID);
            if (c)
              myPop += c->GetPopulation();
          }
          for (int cityID : other.cityIDs) {
            City *c = GetCity(cityID);
            if (c)
              enemyPop += c->GetPopulation();
          }

          if (myPop <= 1 || enemyPop <= 1) {
            kingdom.DeclarePeace(other.id);
            other.DeclarePeace(kingdom.id);
            TraceLog(LOG_INFO, "PEACE: War ended between %s and %s (population depleted)",
                     kingdom.name.c_str(), other.name.c_str());
          }

          // Gradually worsen relations during war
          kingdom.ModifyRelation(other.id, -deltaTime * 0.1f);
        }
      }
    }
  }
}

void SimulationManager::ForceDeclareWar(int kingdomA, int kingdomB) {
  if (kingdomA < 0 || kingdomB < 0 || kingdomA == kingdomB)
    return;

  Kingdom *a = GetKingdom(kingdomA);
  Kingdom *b = GetKingdom(kingdomB);
  if (!a || !b)
    return;

  a->SetRelation(kingdomB, -100.0f);
  b->SetRelation(kingdomA, -100.0f);
  a->DeclareWar(kingdomB);
  b->DeclareWar(kingdomA);
  TraceLog(LOG_INFO, "WAR: Forced declaration between %s and %s",
           a->name.c_str(), b->name.c_str());
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
    int idx = GRandom.Int(0, (int)candidates.size() - 1);
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
