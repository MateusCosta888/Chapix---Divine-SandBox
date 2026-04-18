#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

// ============================================================================
// SPATIAL HASH GRID - O(1) spatial queries for entities
// ============================================================================
// Divides the world into cells of CELL_SIZE tiles. Each cell stores a list of
// entity IDs. To find entities near a position, we only check the relevant
// cells instead of iterating over ALL entities.
//
// Usage:
//   spatialHash.Clear();
//   for (auto& [id, e] : entities) spatialHash.Insert(id, e.position);
//   auto nearby = spatialHash.Query(myPos, 30.0f); // IDs within 30 tiles
// ============================================================================

class SpatialHash {
public:
  static constexpr int CELL_SIZE = 16; // Each cell covers 16x16 tiles

  void Clear() { cells.clear(); }

  void Insert(int entityID, float worldX, float worldY) {
    int64_t key = CellKey((int)(worldX / CELL_SIZE), (int)(worldY / CELL_SIZE));
    cells[key].push_back(entityID);
  }

  // Returns all entity IDs within 'radius' tiles of center
  std::vector<int> Query(float centerX, float centerY, float radius) const {
    std::vector<int> result;

    int minCX = (int)((centerX - radius) / CELL_SIZE);
    int maxCX = (int)((centerX + radius) / CELL_SIZE);
    int minCY = (int)((centerY - radius) / CELL_SIZE);
    int maxCY = (int)((centerY + radius) / CELL_SIZE);

    float radiusSq = radius * radius;

    for (int cy = minCY; cy <= maxCY; cy++) {
      for (int cx = minCX; cx <= maxCX; cx++) {
        int64_t key = CellKey(cx, cy);
        auto it = cells.find(key);
        if (it != cells.end()) {
          for (int id : it->second) {
            result.push_back(id);
          }
        }
      }
    }
    return result;
  }

private:
  // Pack two 32-bit cell coordinates into a single 64-bit key
  static int64_t CellKey(int cx, int cy) {
    return ((int64_t)cx << 32) | (uint32_t)cy;
  }

  std::unordered_map<int64_t, std::vector<int>> cells;
};
