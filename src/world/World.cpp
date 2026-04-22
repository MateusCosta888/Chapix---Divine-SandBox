#include "World.h"
#include "../core/TimeManager.h"
#include "../resources/ResourceManager.h"
#include "../utils/Noise.h"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "../utils/GlobalRandom.h"
#include <functional>
#include "../core/Constants.h"

World::World(int width, int height, uint32_t seed)
    : width(width), height(height), seed_(seed), rng_(seed) {
  tiles.resize(width * height);
}

World::~World() { UnloadTextures(); }

void World::Reset(uint32_t seed) {
  seed_ = seed;
  rng_ = Random(seed);
}

void World::ResizeAndGenerate(
    int newWidth, int newHeight,
    std::function<void(const char *)> loadingCallback) {
  width = newWidth;
  height = newHeight;
  tiles.clear();
  tiles.resize(width * height);
  // Clear entities on new generation
  entities.clear();
  Generate(loadingCallback);
}

void World::LoadTextures(std::function<void(const char *)> loadingCallback) {
  if (loadingCallback)
    loadingCallback("Loading world textures...");
  resourceManager.Load();
}
void World::UnloadTextures() { resourceManager.Unload(); }

void World::UpdateAnimation(float deltaTime) {
  // No animation currently - static textures
  (void)deltaTime;
}

// Helper: Determine biome from temperature, humidity
// Also calculates biomeDistance (0 = at threshold edge, 1 = far from edge)
BiomeType DetermineBiome(float temp, float humidity, float latitude,
                         float &outBiomeDistance) {
  // Calculate distances to each threshold
  float distSnow = std::abs(temp - 0.30f);
  float distDesertTemp = std::abs(temp - 0.65f);
  float distDesertHumid = std::abs(humidity - 0.45f);
  float distForest = std::abs(humidity - 0.50f);

  // Snow biome: Cold regions
  if (temp < 0.30f) {
    outBiomeDistance = std::min(1.0f, distSnow * 5.0f);
    return BiomeType::Snow;
  }

  // Desert biome: Hot and Dry
  if (temp > 0.65f && humidity < 0.45f) {
    outBiomeDistance =
        std::min(1.0f, std::min(distDesertTemp, distDesertHumid) * 5.0f);
    return BiomeType::Desert;
  }

  // Mountain: very cold (handled mostly by height, but also temp)
  if (temp < 0.20f) {
    outBiomeDistance = std::min(1.0f, std::abs(temp - 0.20f) * 5.0f);
    return BiomeType::Mountain;
  }

  // Forest: Temperate and Humid
  if (humidity > 0.50f) {
    outBiomeDistance = std::min(1.0f, distForest * 5.0f);
    return BiomeType::Forest;
  }

  // Plains: Default temperate
  // Distance = minimum distance to any threshold
  outBiomeDistance =
      std::min(1.0f, std::min({distSnow, distDesertTemp, distForest}) * 5.0f);
  return BiomeType::Plains;
}

// Helper: Get terrain type based on height and biome
TileType GetTerrainForBiome(BiomeType biome, float height) {
  // STRICT LAYER SYSTEM (Refined for better gradient)
  // 1. Deep Water - Only for very low noise (Trenches)
  if (height < 0.15f)
    return TileType::DeepOcean;
  // 2. Medium Water - Standard Ocean
  if (height < 0.35f)
    return TileType::Ocean;
  // 3. Shallow Water - Coastal areas
  if (height < 0.42f)
    return TileType::ShallowOcean;

  // 4. Land Layer (Sand/Grass/Forest/Snow)
  // This layer exists between water and mountain.
  if (height < 0.75f) {
    if (biome == BiomeType::Desert) {
      return TileType::DesertSand;
    }
    if (biome == BiomeType::Snow) {
      return TileType::Snow;
    }
    // Coastal check for Plains/Forest
    if (height < 0.44f) {    // Thinner beach (0.42 - 0.44)
      return TileType::Sand; // Beach just above water
    }
    if (biome == BiomeType::Forest) {
      return TileType::Forest;
    }
    return TileType::Grass; // Default land
  }

  if (biome == BiomeType::Snow)
    return TileType::Snow;
  if (biome == BiomeType::Desert)
    return TileType::DesertSand;
  if (biome == BiomeType::Forest)
    return TileType::Forest;
  return TileType::Grass;
}

bool World::IsWalkable(int x, int y, bool ignoreBuildings) const {
  if (x < 0 || x >= width || y < 0 || y >= height)
    return false;
  const Tile &t = tiles[y * width + x];

  // Mountains are too high to walk over
  if (t.type == TileType::Mountain)
    return false;

  // Deep ocean might be too deep? User said "need swimming in water".
  // Assuming "Walkable" means "Can stand/walk".
  // So Water is NOT walkable (must swim).
  // Land is walkable.
  if (t.type == TileType::DeepOcean || t.type == TileType::Ocean) {
    return false; // Requires swimming
  }

  // Check blocking decorations
  if (t.decoration == DecorationType::Tree ||
      t.decoration == DecorationType::PineTree ||
      t.decoration == DecorationType::PalmTree ||
      t.decoration == DecorationType::Rock ||
      t.decoration == DecorationType::BigRock ||
      t.decoration == DecorationType::SmallRock ||
      t.decoration == DecorationType::MediumRock ||
      t.decoration == DecorationType::Bush ||
      t.decoration == DecorationType::Ruins ||
      t.decoration == DecorationType::Crystal) {
    return false;
  }

  // Check if there is a building
  if (!ignoreBuildings && t.isOccupied)
    return false;

  return true;
}

bool World::IsSwimmable(int x, int y) const {
  if (x < 0 || x >= width || y < 0 || y >= height)
    return false;
  const Tile &t = tiles[y * width + x];
  return (t.type == TileType::DeepOcean || t.type == TileType::Ocean ||
          t.type == TileType::ShallowOcean);
}

float World::GetHeight(int x, int y) const {
  if (x < 0 || x >= width || y < 0 || y >= height)
    return 0.0f;
  return tiles[y * width + x].height;
}

void World::Generate(std::function<void(const char *)> loadingCallback) {
  // Clear all simulation data (citizens, cities, kingdoms, buildings)
  // This prevents crashes when generating a new world after playing
  simulation.Reset();

  // Create noise generators with different seeds for each layer
  Noise heightNoise(seed_);
  Noise tempNoise(seed_ + 1000);     // Offset seed for variation
  Noise humidityNoise(seed_ + 2000); // Different offset
  Noise mountainNoise(seed_ + 3000); // New independent noise for mountains

  // Pseudo-random centers based on world seed for mountain regions
  float mCenters[3][2];
  float mRadii[3];

  std::mt19937 mRng(seed_ ^ 9999);
  std::uniform_real_distribution<float> distX(0, (float)width);
  std::uniform_real_distribution<float> distY(0, (float)height);

  // 1 Major Region
  mCenters[0][0] = distX(mRng);
  mCenters[0][1] = distY(mRng);
  mRadii[0] = std::min(width, height) * 0.35f; // Large radius (35% of map size)

  // 2 Minor Regions
  mCenters[1][0] = distX(mRng);
  mCenters[1][1] = distY(mRng);
  mRadii[1] = std::min(width, height) * 0.15f;

  mCenters[2][0] = distX(mRng);
  mCenters[2][1] = distY(mRng);
  mRadii[2] = std::min(width, height) * 0.15f;

  int padding = 10; // Safe zone padding

  if (loadingCallback)
    loadingCallback("Generating terrain & biomes...");

  for (int y = 0; y < height; y++) {
    if (y % 16 == 0 && loadingCallback) {
      loadingCallback(TextFormat("Generating terrain & biomes... %d%%",
                                 (y * 100) / height));
    }
    for (int x = 0; x < width; x++) {
      Tile &tile = GetTile(x, y);
      tile.decoration = DecorationType::None; // Reset decorations

      // Check if this is outside safe zone
      bool outsideSafeZone = (x < padding || x >= width - padding ||
                              y < padding || y >= this->height - padding);

      if (outsideSafeZone) {
        tile.height = 0.0f;
        tile.type = TileType::DeepOcean;
        tile.biome = BiomeType::Ocean;
        tile.temperature = 0.3f;
        tile.humidity = 1.0f;
        continue;
      }

      float nx = (float)x / width;
      float ny = (float)y / this->height;

      // 1. Generate base height
      float h = heightNoise.GetFractal(nx, ny, 6, 4.0f, 0.5f);

      // 2. Apply border mask
      int distFromSafeEdge =
          std::min({x - padding, width - padding - 1 - x, y - padding,
                    this->height - padding - 1 - y});
      float transitionWidth = 5.0f;
      float mask = std::min(1.0f, distFromSafeEdge / transitionWidth);
      mask = mask * mask * (3.0f - 2.0f * mask);
      h = h * mask - 0.05f;
      tile.height = h;

      // 3. Generate temperature and humidity
      // Blend noise (local variation) with latitude (global gradient)
      // ny goes from 0 (North) to 1 (South)
      float tempNoiseVal = tempNoise.GetFractal(nx, ny, 3, 2.0f, 0.5f);
      float temp = tempNoiseVal * 0.6f + ny * 0.4f;

      float humid = humidityNoise.GetFractal(nx, ny, 3, 2.0f, 0.5f);

      tile.temperature = temp;
      tile.humidity = humid;

      // 4. Determine biome (using temp/humidity logic only)
      if (h < 0.35f) {
        tile.biome = BiomeType::Ocean;
        tile.biomeDistance = 1.0f;
      } else {
        tile.biome = DetermineBiome(temp, humid, ny, tile.biomeDistance);
      }

      // 5. Get terrain type based on biome and height
      tile.type = GetTerrainForBiome(tile.biome, h);

      // Check if current point (x, y) is inside any mountain region
      bool inMountainRegion = false;
      float distanceFade = 0.0f; // 0 edge, 1 center

      for (int i = 0; i < 3; i++) {
        float dx = x - mCenters[i][0];
        float dy = y - mCenters[i][1];
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < mRadii[i]) {
          inMountainRegion = true;
          // Simple fade factor for blending?
          // For now, hard boolean mask is what user asked ("max 3 biomes").
          // But let's attenuate edges slightly for smoothness?
          // Actually user wants "distinct", so let's keep boolean but rely on
          // noise to shape it.
          break;
        }
      }

      if (h > 0.0f &&
          inMountainRegion) { // Land only AND inside one of the 3 circles
        // Ridge Noise Logic...
        float mVal = mountainNoise.GetFractal(nx, ny, 4, 2.5f, 0.5f);
        float ridge = 1.0f - (std::abs(mVal - 0.5f) * 2.0f);
        ridge = ridge * ridge * ridge * ridge;

        float threshold = 0.85f;
        if (tile.type == TileType::Grass)
          threshold = 0.80f;

        if (ridge > threshold) {
          tile.type = TileType::Mountain;
          tile.height += 0.5f;
        }
      }

      // Pre-calculate visual variants
      tile.variant = x * 73856093 ^ y * 19349663;
      tile.decorationVariant = (tile.variant ^ seed_) % 4;
    }
  }

  // ==========================================================================
  // MOUNTAIN FILTERING (Despeckle)
  // Remove small mountain clusters to enforce minimum size roughly 10x10
  // perception. We do this by checking neighbor count.
  // ==========================================================================

  if (loadingCallback)
    loadingCallback("Filtering mountains...");
  for (int minSizePass = 0; minSizePass < 3;
       minSizePass++) { // Multiple passes to erode small bits
    std::vector<TileType> newTypes(width * height);
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        newTypes[y * width + x] = GetTile(x, y).type;
      }
    }

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        Tile &tile = GetTile(x, y);
        if (tile.type == TileType::Mountain) {
          int mountainNeighbors = 0;
          // Check 5x5 area (radius 2)
          for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
              if (dx == 0 && dy == 0)
                continue;
              int nx = x + dx;
              int ny = y + dy;
              if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                if (GetTile(nx, ny).type == TileType::Mountain)
                  mountainNeighbors++;
              }
            }
          }
          // Max neighbors in 5x5 is 24.
          // If isolated or thin strip, remove.
          // Require strong support (e.g., > 15) to survive.
          if (mountainNeighbors < 15) {
            // Revert to underlying biome logic (approximate)
            // Just make it the neighbor's generic type (Grass/Forest) or
            // re-calc Simplest: Set to Grass or closest non-mountain neighbor?
            // Let's just set to Grass (default land) or Forest if biome
            // matches.
            if (tile.biome == BiomeType::Forest)
              newTypes[y * width + x] = TileType::Forest;
            else if (tile.biome == BiomeType::Desert)
              newTypes[y * width + x] = TileType::DesertSand;
            else if (tile.biome == BiomeType::Snow)
              newTypes[y * width + x] = TileType::Snow;
            else
              newTypes[y * width + x] = TileType::Grass;
          }
        }
      }
    }

    // Apply changes
    for (int i = 0; i < width * height; i++) {
      tiles[i].type = newTypes[i];
    }
  }

  // ==========================================================================
  // EDGE MASK CALCULATION
  // ==========================================================================
  if (loadingCallback)
    loadingCallback("Calculating edge masks...");
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      Tile &tile = GetTile(x, y);
      tile.edgeMask = 0;

      if (y > 0 && GetTile(x, y - 1).type != tile.type)
        tile.edgeMask |= 0x01;
      if (x < width - 1 && GetTile(x + 1, y).type != tile.type)
        tile.edgeMask |= 0x02;
      if (y < height - 1 && GetTile(x, y + 1).type != tile.type)
        tile.edgeMask |= 0x04;
      if (x > 0 && GetTile(x - 1, y).type != tile.type)
        tile.edgeMask |= 0x08;
    }
  }

  // ==========================================================================
  // PROCEDURAL DECORATION GENERATION (Baking visuals into logic)
  // ==========================================================================
  if (loadingCallback)
    loadingCallback("Placing decorations...");
  for (int y = 0; y < height; y++) {
    if (y % 16 == 0 && loadingCallback) {
      loadingCallback(
          TextFormat("Placing decorations... %d%%", (y * 100) / height));
    }
    for (int x = 0; x < width; x++) {
      Tile &tile = GetTile(x, y);

      // Skip if already decorated (manual or water safety)
      if (tile.decoration != DecorationType::None)
        continue;
      if (IsSwimmable(x, y))
        continue;

      unsigned int seed = tile.variant ^ seed_ ^ 9284387;
      int tileHash = tile.variant;

      // 1. TREES
      bool hasTree = false;
      if (tile.type == TileType::Forest) {
        if ((seed % 100) < 12) { // Reduced from 20% to 12%
          tile.decoration = DecorationType::Tree;
          // Variant logic: 0=Fruit, 1=Normal, 2=Moss.
          // We store specific type in variant? Or just generic Tree and
          // renderer handles? Rendering used complex logic. For simple
          // collision, DecorationType::Tree is enough. We can use
          // decorationVariant to store the specific subtype if needed.
          // WorldRenderer manual pass used decorationVariant directly.
          // Let's store a random variant 0-100 and let renderer map it.
          tile.decorationVariant = seed % 100;
          hasTree = true;
        }
      } else if (tile.type == TileType::Snow) {
        if ((seed % 100) < 8) { // Reduced from 15% to 8%
          tile.decoration = DecorationType::PineTree;
          tile.decorationVariant = seed % 100;
          hasTree = true;
        }
      } else if (tile.type == TileType::Sand) { // Beach
        if ((seed % 100) < 5) {                 // 5% Palm Trees on Beach
          tile.decoration = DecorationType::PalmTree;
          tile.decorationVariant = seed % 100;
          hasTree = true;
        }
      } else if (tile.type == TileType::DesertSand) {
        if ((seed % 100) < 5) { // 5% Desert Plants
          tile.decoration = DecorationType::DesertPlant;
          tile.decorationVariant =
              seed %
              100; // Will map to 0,1,2 in Renderer (Rock, Cactus1, Cactus2)
          hasTree = true; // Use hasTree flag to skip other decorations
        }
      }

      if (hasTree)
        continue;

      // 2. CRYSTALS (Mountain)
      if (tile.type == TileType::Mountain && tile.height > 0.65f) {
        unsigned int crySeed = tile.variant ^ 0x111 ^ seed_ ^ 33333;
        if ((crySeed % 1000) < 15) { // 1.5%
          tile.decoration = DecorationType::Crystal;
          tile.decorationVariant = crySeed % 100;
          continue;
        }
      }

      // 3. ROCKS
      // Rock 1 (Common - Grass/Forest)
      if (tile.type == TileType::Grass || tile.type == TileType::Forest) {
        unsigned int rockSeed = tile.variant ^ seed_ ^ 1234567;
        if ((rockSeed % 1000) < 2) { // 0.2% chance (Drastically reduced)
          tile.decoration = DecorationType::Rock; // Rock1
          tile.decorationVariant = 0;
          tile.resourceAmount = 50.0f; // Durable
          continue;
        }
      }

      // Rock 2 (Mountain)
      if (tile.type == TileType::Mountain) {
        unsigned int rockSeed = tile.variant ^ seed_ ^ 88888;
        if ((rockSeed % 1000) <
            5) { // 0.5% chance on mountains (Reduced from 5%)
          tile.decoration = DecorationType::SmallRock; // Rock2
          tile.decorationVariant = 0;
          tile.resourceAmount = 80.0f; // Harder
          continue;
        }
      }

      // Rock 3 (Universal - Any land)
      if (tile.type != TileType::DeepOcean && tile.type != TileType::Ocean &&
          tile.type != TileType::ShallowOcean) {
        unsigned int rockSeed = tile.variant ^ seed_ ^ 55555;
        if ((rockSeed % 10000) <
            5) { // 0.05% chance anywhere (Reduced from 0.5%)
          tile.decoration = DecorationType::MediumRock; // Rock3
          tile.decorationVariant = 0;
          tile.resourceAmount = 100.0f; // Very Durable
          continue;
        }
      }

      // Rock 4 (Desert)
      if (tile.type == TileType::DesertSand) {
        unsigned int rockSeed = tile.variant ^ seed_ ^ 44444;
        if ((rockSeed % 1000) < 2) { // 0.2% chance (Reduced from 3%)
          tile.decoration = DecorationType::BigRock; // Rock4
          tile.decorationVariant = 0;
          tile.resourceAmount = 60.0f;
          continue; // Skip cactus if rock
        }
      }

      // 5. BUSHES & CACTI
      unsigned int bushSeed = tile.variant ^ 0x876 ^ seed_ ^ 55555;

      // Removed standard bushes per user request
      /*
      if (tile.type == TileType::Forest || tile.type == TileType::Grass) {
        if ((bushSeed % 100) < 20) { // 20%
          tile.decoration = DecorationType::Bush;
          tile.decorationVariant = bushSeed % 100;
          continue;
        }
      } else
      */

      if (tile.type == TileType::Snow) {
        if ((bushSeed % 100) < 5) {               // 5%
          tile.decoration = DecorationType::Bush; // Snow bush
          tile.decorationVariant = bushSeed % 100;
          continue;
        }
      } else if (tile.type == TileType::DesertSand) {
        if ((bushSeed % 100) < 3) { // 3%
          tile.decoration = DecorationType::Cactus;
          tile.decorationVariant = bushSeed % 100;
          continue;
        }
      }

      // 6. GRASS TUFTS (Forest1 only)
      if (tile.type == TileType::Forest) {
        if ((tileHash % 100) < 60) { // Only on Forest1
          unsigned int grassSeed = tile.variant ^ 0x444 ^ seed_ ^ 66666;
          if ((grassSeed % 100) < 25) { // 25%
            tile.decoration = DecorationType::GrassTuft;
            tile.decorationVariant = grassSeed % 100;
            continue;
          }
        }
      }
    }
  }

  // ==========================================================================
  // LIQUID LEVEL INITIALIZATION - Set water tiles to full liquid
  // ==========================================================================
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      Tile &tile = GetTile(x, y);
      if (tile.type == TileType::DeepOcean) {
        tile.liquidLevel = 1.0f;
      } else if (tile.type == TileType::Ocean) {
        tile.liquidLevel = 0.5f; // Matches > 0.3f threshold in SimulateWater
      } else if (tile.type == TileType::ShallowOcean) {
        tile.liquidLevel = 0.25f; // Matches > 0.0f threshold in SimulateWater
      } else {
        tile.liquidLevel = 0.0f; // Dry land
      }
    }
  }

  // ==========================================================================
  // NATURAL ANIMAL SPAWNING - Sparse population on grasslands
  // ==========================================================================
  entities.clear(); // Clear any existing entities

  int cowCount = 0, chickenCount = 0, sheepCount = 0, boarCount = 0,
      slimeCount = 0;
  int maxCows = 12, maxChickens = 15, maxSheep = 12, maxBoars = 2,
      maxSlimes = 2; // Drastically reduced for early game survival

  for (int y = 20; y < height - 20; y += 4) { // Increased step to spread more
    for (int x = 20; x < width - 20; x += 4) {
      Tile &tile = GetTile(x, y);

      // Land check
      if (tile.type == TileType::DeepOcean || tile.type == TileType::Ocean ||
          tile.type == TileType::ShallowOcean)
        continue;

      // Use tile variant for deterministic random
      unsigned int spawnRoll = (tile.variant ^ seed_ ^ 0xDEADBEEF) % 100;

      if (spawnRoll < 4 && cowCount < maxCows && tile.type == TileType::Grass) {
        AddEntity(EntityType::Cow, {(float)x + 0.5f, (float)y + 0.5f});
        cowCount++;
      } else if (spawnRoll >= 4 && spawnRoll < 10 &&
                 chickenCount < maxChickens &&
                 (tile.type == TileType::Grass ||
                  tile.type == TileType::Forest)) {
        AddEntity(EntityType::Chicken, {(float)x + 0.5f, (float)y + 0.5f});
        chickenCount++;
      } else if (spawnRoll >= 10 && spawnRoll < 16 && sheepCount < maxSheep &&
                 tile.type == TileType::Grass) {
        AddEntity(EntityType::Sheep, {(float)x + 0.5f, (float)y + 0.5f});
        sheepCount++;
      } else if (spawnRoll >= 16 && spawnRoll < 20 && boarCount < maxBoars &&
                 tile.type == TileType::Forest) {
        AddEntity(EntityType::Boar, {(float)x + 0.5f, (float)y + 0.5f});
        boarCount++;
      } else if (spawnRoll >= 20 && spawnRoll < 25 && slimeCount < maxSlimes &&
                 (tile.type == TileType::Forest ||
                  tile.type == TileType::Mountain)) {
        AddEntity(EntityType::Slime, {(float)x + 0.5f, (float)y + 0.5f});
        slimeCount++;
      }
    }
  }

  TraceLog(
      LOG_INFO,
      "ANIMALS: Spawned %d cows, %d chickens, %d sheep, %d boars, %d slimes",
      cowCount, chickenCount, sheepCount, boarCount, slimeCount);
}

Tile &World::GetTile(int x, int y) {
  if (x < 0 || x >= width || y < 0 || y >= height) {
    static Tile empty;
    return empty;
  }
  return tiles[y * width + x];
}

const Tile &World::GetTileConst(int x, int y) const {
  static Tile empty;
  if (x < 0 || x >= width || y < 0 || y >= height) {
    return empty;
  }
  return tiles[y * width + x];
}

void World::Update() {
  // Use TimeManager for pause and speed control
  float dt = TimeManager::Get().GetDeltaTime();

  // Skip all updates when paused (dt will be 0)
  if (dt <= 0.0f)
    return;

  // Run water physics
  SimulateWater(dt);

  // Update entity movement and animations
  UpdateEntities(dt);

  // Run civilization simulation (citizens, cities, kingdoms)
  simulation.Update(*this, dt);

  // World Events
  UpdateWorldEvents(dt);
}

void World::UpdateWorldEvents(float deltaTime) {
  static float eventTimer = 0.0f;
  eventTimer += deltaTime;
  float eventInterval = 120.0f; // Check for events every 2 minutes
  if (eventTimer >= eventInterval) {
    eventTimer = 0.0f;
    // Random chance for Dragon Spawn
    if (GRandom.Chance(10)) { // 10% chance
      // Count current dragons
      int dragonCount = 0;
      for (const auto &pair : entities) {
        if (pair.second.type == EntityType::Dragon && pair.second.health > 0) {
          dragonCount++;
        }
      }

      if (dragonCount < Constants::MAX_DRAGONS_IN_WORLD) {
        // Find a random position, perhaps near a city
        int attempts = 10;
        for (int i = 0; i < attempts; i++) {
          int x = GRandom.Int(0, width - 1);
          int y = GRandom.Int(0, height - 1);
          if (GetTile(x, y).type == TileType::Grass) {
            Vector2 pos = {static_cast<float>(x), static_cast<float>(y)};
            AddEntity(EntityType::Dragon, pos, true); // true = skip gender random
            TraceLog(LOG_INFO, "WORLD EVENT: Dragon spawned at (%d, %d)", x, y);
            break;
          }
        }
      }
    }
    // Other events can be added here
  }
}

void World::SimulateWater(float deltaTime) {
  // STATIC WATER (Painting Mode)
  // No flow, no gravity. Water stays where it is placed.
  // We only update the visual type based on the liquidLevel.

  // Updates per second (less frequent is fine for static, but responsive for
  // painting)
  static float timer = 0.0f;
  timer += deltaTime;
  if (timer < 0.1f)
    return;
  timer = 0.0f;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      Tile &tile = GetTile(x, y);
      float level = tile.liquidLevel;

      // Update visual tile type based on liquid level
      if (level > 0.6f) {
        if (tile.type != TileType::DeepOcean)
          tile.type = TileType::DeepOcean;
      } else if (level > 0.3f) {
        if (tile.type != TileType::Ocean)
          tile.type = TileType::Ocean;
      } else if (level > 0.0f) {
        if (tile.type != TileType::ShallowOcean)
          tile.type = TileType::ShallowOcean;
      } else {
        // Dry - restore natural terrain if it was water
        // Only change back if it IS currently a water type
        if (tile.type == TileType::DeepOcean || tile.type == TileType::Ocean ||
            tile.type == TileType::ShallowOcean) {
          tile.type = GetTerrainForBiome(tile.biome, tile.height);
        }
      }
    }
  }
}

// Helper: Get biome priority for autotiling (Higher number = draws on top)
int GetBiomePriority(TileType type) {
  switch (type) {
  case TileType::DeepOcean:
    return 0;
  case TileType::Ocean:
    return 1;
  case TileType::ShallowOcean:
    return 2;
  case TileType::Sand:
    return 3;
  case TileType::Grass:
    return 4;
  case TileType::Forest:
    return 4; // Forest acts as Grass for transitions
  case TileType::Mountain:
    return 5;
  case TileType::Snow:
    return 6;
  default:
    return -1;
  }
}

Texture2D *World::GetTextureForTile(TileType type) {
  // Use ResourceManager to get the base texture for the type
  static Texture2D tempTex;
  tempTex = resourceManager.GetTextureForTile(type);
  return &tempTex;
}

void World::SetTileBiome(int x, int y, BiomeType newBiome) {
  Tile &tile = GetTile(x, y);
  if (tile.type == TileType::DeepOcean || tile.type == TileType::Ocean) {
    return; // Cannot change biome of water
  }
  tile.biome = newBiome;
  tile.type = GetTerrainForBiome(newBiome, tile.height);
  UpdateNeighborsEdgeMask(x, y);
}

void World::SetTileType(int x, int y, TileType newType) {
  if (x < 0 || x >= width || y < 0 || y >= height)
    return;

  Tile &tile = GetTile(x, y);
  tile.type = newType;

  // Sync liquidLevel with water tile types
  if (newType == TileType::DeepOcean) {
    tile.liquidLevel = 1.0f;
  } else if (newType == TileType::Ocean) {
    tile.liquidLevel = 0.8f;
  } else if (newType == TileType::ShallowOcean) {
    tile.liquidLevel = 0.5f;
  } else {
    // Non-water tile - clear liquid
    tile.liquidLevel = 0.0f;
  }

  UpdateNeighborsEdgeMask(x, y);

  // When water is placed, destroy EVERYTHING on that tile
  bool isWater =
      (newType == TileType::DeepOcean || newType == TileType::Ocean ||
       newType == TileType::ShallowOcean);
  if (isWater) {
    ClearTileContents(x, y);
  }
}

void World::ClearTileContents(int x, int y) {
  if (x < 0 || x >= width || y < 0 || y >= height)
    return;

  Tile &tile = GetTile(x, y);

  // 1. Remove decorations and resources
  tile.decoration = DecorationType::None;
  tile.resourceAmount = 0.0f;
  tile.hasStump = false;
  tile.regrowthTimer = 0.0f;
  tile.originalTree = DecorationType::None;

  // Clear farming data
  tile.isPlanted = false;
  tile.growthProgress = 0.0f;
  tile.farmOwnerCityID = -1;

  // 2. Remove buildings on this tile
  simulation.DestroyBuildingsAtTile(x, y);
  tile.isOccupied = false;

  // 3. Kill all entities on this tile (humans and animals)
  for (auto &[eid, e] : entities) {
    if ((int)e.position.x == x && (int)e.position.y == y) {
      e.health = 0;
      e.state = EntityState::Die;
    }
  }
}

void World::SetTileDecoration(int x, int y, DecorationType type) {
  if (x < 0 || x >= width || y < 0 || y >= height)
    return;
  Tile &tile = GetTile(x, y);
  tile.decoration = type;
  tile.decorationVariant = rng_.Int(0, 3);
}

Texture2D World::GetTextureForUI(TileType type) const {
  return resourceManager.GetTextureForUI(type);
}

Texture2D World::GetTextureForUI(DecorationType type) const {
  return resourceManager.GetTextureForUI(type);
}

Texture2D World::GetTextureForUI(EntityType type) const {
  if (type == EntityType::HumanUnarmed) {
    if (!resourceManager.texHumanUnarmed[0][0].empty())
      return resourceManager.texHumanUnarmed[0][0][0]; // Idle Down Frame 0
    return {0};
  }
  if (type == EntityType::HumanWoman) {
    if (!resourceManager.texHumanWoman[0][0].empty())
      return resourceManager.texHumanWoman[0][0][0];
    return {0};
  }
  if (type == EntityType::HumanArmed) {
    if (!resourceManager.texHumanArmed[0][0].empty())
      return resourceManager.texHumanArmed[0][0][0];
    return {0};
  }
  if (type == EntityType::Boar) {
    if (!resourceManager.texBoarIdle.empty())
      return resourceManager.texBoarIdle[0];
    return {0};
  }
  if (type == EntityType::Slime) {
    if (!resourceManager.slimeIdle.empty() && !resourceManager.slimeIdle[0].empty())
      return resourceManager.slimeIdle[0][0];
    return {0};
  }
  if (type == EntityType::Dragon) {
    if (!resourceManager.texDragonFly.empty())
      return resourceManager.texDragonFly[0];
    return {0};
  }
  if (type == EntityType::Cow) {
    return resourceManager.texCow[0];
  }
  if (type == EntityType::Chicken) {
    return resourceManager.texChicken[0];
  }
  if (type == EntityType::Sheep) {
    return resourceManager.texSheep[0];
  }
  if (type == EntityType::Bull) {
    return resourceManager.texBull[0];
  }
  if (type == EntityType::Chicken2) {
    return resourceManager.texChicken2[0];
  }
  if (type == EntityType::Lamb) {
    return resourceManager.texLamb[0];
  }
  if (type == EntityType::Pig) {
    return resourceManager.texPig[0];
  }
  if (type == EntityType::Turkey) {
    return resourceManager.texTurkey[0];
  }
  return {0};
}

void World::UpdateTileEdgeMask(int x, int y) {
  if (x < 0 || x >= width || y < 0 || y >= height)
    return;

  Tile &tile = GetTile(x, y);
  tile.edgeMask = 0;

  // Check each direction for different terrain
  // Bit 0 = North, Bit 1 = East, Bit 2 = South, Bit 3 = West
  if (y > 0 && GetTile(x, y - 1).type != tile.type)
    tile.edgeMask |= 0x01;
  if (x < width - 1 && GetTile(x + 1, y).type != tile.type)
    tile.edgeMask |= 0x02;
  if (y < height - 1 && GetTile(x, y + 1).type != tile.type)
    tile.edgeMask |= 0x04;
  if (x > 0 && GetTile(x - 1, y).type != tile.type)
    tile.edgeMask |= 0x08;
}

void World::UpdateNeighborsEdgeMask(int x, int y) {
  UpdateTileEdgeMask(x, y);
  UpdateTileEdgeMask(x, y - 1);
  UpdateTileEdgeMask(x + 1, y);
  UpdateTileEdgeMask(x, y + 1);
  UpdateTileEdgeMask(x - 1, y);
}

int World::GetKingdomAtTile(int tx, int ty) const {
  if (tx < 0 || tx >= width || ty < 0 || ty >= height)
    return -1;

  const Tile &tile = GetTileConst(tx, ty);
  if (tile.ownerCityID < 0)
    return -1;

  const City *city = simulation.GetCity(tile.ownerCityID);
  if (!city)
    return -1;

  return city->kingdomID;
}

void World::AddEntity(EntityType type, Vector2 pos, bool skipGenderRandom) {
  // 50/50 chance: player-placed unarmed humans may be women (only for Random)
  // If type is already HumanWoman, skip randomization
  if (!skipGenderRandom && type == EntityType::HumanUnarmed &&
      GRandom.Chance(50)) {
    type = EntityType::HumanWoman;
  }

  // Prevent spawning on water/obstacles (unless user forces it, but we block it
  // per request)
  if (!IsWalkable((int)pos.x, (int)pos.y)) {
    // Try to find a valid spot nearby? Or just block?
    // User said "should not be possible", so blocking is safest.
    TraceLog(LOG_WARNING,
             "WORLD: Blocked spawning entity on invalid tile at %.2f, %.2f",
             pos.x, pos.y);
    return;
  }

  Entity e;
  e.id = nextEntityID++;
  e.type = type;
  e.position = pos;
  e.targetPos = pos;
  e.state = EntityState::Idle;
  e.health = 100.0f;
  e.currentFrame = 0;
  e.animTime = 0.0f;
  e.facingDirection = 0; // Down
  e.hasTarget = false;
  e.targetID = -1;
  e.justSpawned = true;
  e.enemyScanTimer = 0.0f;
  e.attackCooldown = 0.0f;
  e.attackSpeed = 1.0f;
  e.citizenID = -1; // Initialize as non-citizen

  // Set speed based on creature type
  // Set speed and health based on creature type
  // Set speed and health based on creature type
  if (type == EntityType::HumanUnarmed || type == EntityType::HumanWoman) {
    e.speed = 2.0f;
    e.health = 80.0f;
    e.attackSpeed = 1.0f;
  } else if (type == EntityType::HumanArmed) {
    e.speed = 2.2f;
    e.health = 160.0f;
    e.attackSpeed = 0.75f;
  } else if (type == EntityType::Boar) {
    e.speed = 2.4f;
    e.health = 30.0f; // Nerfed
    e.attackSpeed = 1.2f;
  } else if (type == EntityType::Slime) {
    e.speed = 1.3f;
    e.health = 10.0f; // Nerfed
    e.attackSpeed = 1.0f;
  } else if (type == EntityType::Cow || type == EntityType::Bull) {
    e.speed = 0.5f;
    e.health = 10.0f;
    e.attackSpeed = 1.5f;
  } else if (type == EntityType::Chicken || type == EntityType::Chicken2 ||
             type == EntityType::Turkey) {
    e.speed = 0.8f;
    e.health = 5.0f;
    e.attackSpeed = 1.5f;
  } else if (type == EntityType::Sheep || type == EntityType::Lamb) {
    e.speed = 0.6f;
    e.health = 10.0f;
    e.attackSpeed = 1.5f;
  } else if (type == EntityType::Pig) {
    e.speed = 0.55f;
    e.health = 10.0f;
    e.attackSpeed = 1.5f;
  } else if (type == EntityType::Dragon) {
    e.speed = 3.0f;
    e.health = 300.0f; // High HP boss
    e.attackSpeed = 2.0f; // Slower attacks
  } else {
    e.speed = 1.0f;
    e.attackSpeed = 1.0f;
  }

  e.maxHP = e.health; // Set maxHP to initial health

  // For intelligent creatures (humans), create citizen data
  if (e.IsIntelligent()) {
    Citizen citizen = CreateRandomCitizen(simulation.GetNextCitizenID());
    // Sync gender with entity type
    if (type == EntityType::HumanWoman)
      citizen.isFemale = true;
    else
      citizen.isFemale = false;
    e.citizenID = simulation.AddCitizen(citizen);
    TraceLog(LOG_INFO, "SIMULATION: Created Citizen %d for Entity %d",
             e.citizenID, e.id);

    // Check if near an existing city - join it immediately
    int tx = static_cast<int>(pos.x);
    int ty = static_cast<int>(pos.y);
    const auto &cities = simulation.GetAllCities();
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

    // If close to a city (within 10 tiles), join it immediately
    if (nearestCityID >= 0 && nearestDist <= 10.0f) {
      Citizen *c = simulation.GetCitizen(e.citizenID);
      if (c) {
        c->cityID = nearestCityID;
        simulation.AddCitizenToCity(nearestCityID, e.citizenID);
        TraceLog(LOG_INFO,
                 "SIMULATION: Citizen %d joined City %d on spawn (dist: %.1f)",
                 e.citizenID, nearestCityID, nearestDist);
      }
    }
  }

  entities[e.id] = e;
  RebuildEntityCache();
  TraceLog(LOG_INFO, "WORLD: Added Entity Type %d at %.2f, %.2f", (int)type,
           pos.x, pos.y);
}

Entity *World::GetEntityByID(int id) {
  auto it = entities.find(id);
  return it != entities.end() ? &it->second : nullptr;
}

const Entity *World::GetEntityByID(int id) const {
  auto it = entities.find(id);
  return it != entities.end() ? &it->second : nullptr;
}

void World::RebuildSpatialHash() {
  spatialHash.Clear();
  for (auto &[id, e] : entities) {
    if (e.health > 0 && e.state != EntityState::Die) {
      spatialHash.Insert(id, e.position.x, e.position.y);
    }
  }
}

std::vector<Entity *> World::GetEntitiesInRadius(Vector2 center, float radius) {
  std::vector<Entity *> result;
  auto ids = spatialHash.Query(center.x, center.y, radius);
  float radiusSq = radius * radius;
  for (int id : ids) {
    Entity *e = GetEntityByID(id);
    if (e && e->health > 0 && e->state != EntityState::Die) {
      float dx = e->position.x - center.x;
      float dy = e->position.y - center.y;
      if (dx * dx + dy * dy <= radiusSq) {
        result.push_back(e);
      }
    }
  }
  return result;
}

void World::RebuildEntityCache() {
  citizenEntityMap.clear();
  for (auto &[id, e] : entities) {
    if (e.citizenID != -1) {
      citizenEntityMap[e.citizenID] = &e;
    }
  }
}

Entity *World::GetEntityByCitizenID(int citizenID) {
  auto it = citizenEntityMap.find(citizenID);
  if (it != citizenEntityMap.end()) {
    return it->second;
  }
  return nullptr; // Not found
}

const Entity *World::GetEntityByCitizenID(int citizenID) const {
  auto it = citizenEntityMap.find(citizenID);
  if (it != citizenEntityMap.end()) {
    return it->second;
  }
  return nullptr; // Not found
}

void World::UpdateEntities(float deltaTime) {
  // === PHASED NATURAL SPAWNING ===
  static float spawnTimer = 0.0f;
  spawnTimer += deltaTime;
  if (spawnTimer >= 10.0f) { // Check every 10s
    spawnTimer = 0.0f;
    
    int currentPop = simulation.GetTotalPopulation();
    int slimeTarget = 2 + currentPop / 20;
    int boarTarget = 1 + currentPop / 40;
    
    int currentSlimes = 0;
    int currentBoars = 0;
    for (auto &pair : entities) {
        if (pair.second.type == EntityType::Slime) currentSlimes++;
        else if (pair.second.type == EntityType::Boar) currentBoars++;
    }

    if (currentSlimes < slimeTarget && rng_.Int(0, 100) < 30) {
        // Spawn slime at edge
        int rx = rng_.Int(0, 1) == 0 ? rng_.Int(2, 10) : rng_.Int(width - 10, width - 2);
        int ry = rng_.Int(2, height - 2);
        if (IsWalkable(rx, ry)) AddEntity(EntityType::Slime, {(float)rx, (float)ry}, true);
    }
    if (currentBoars < boarTarget && rng_.Int(0, 100) < 20) {
        // Spawn boar far from center
        int rx = rng_.Int(5, width - 5);
        int ry = rng_.Int(5, height - 5);
        if (IsWalkable(rx, ry)) AddEntity(EntityType::Boar, {(float)rx, (float)ry}, true);
    }
  }

  // Rebuild caches at start of frame
  RebuildEntityCache();
  RebuildSpatialHash();

  // Collect IDs of dead entities for deferred removal
  std::vector<int> deadEntityIDs;

  for (auto &[entityID, e] : entities) {

    if (e.isGrabbed) {
      continue; // Hand of God: skip all physics/logic while grabbed
    }

    // === ENTITY-DECORATION SEPARATION (Prevent overlapping with trees/rocks) ===
    int ctx = (int)e.position.x;
    int cty = (int)e.position.y;
    for (int dy = -1; dy <= 1; dy++) {
      for (int dx = -1; dx <= 1; dx++) {
        int nx = ctx + dx, ny = cty + dy;
        if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
        const Tile &t = GetTileConst(nx, ny);
        if (t.decoration != DecorationType::None) {
          Vector2 decoPos = {(float)nx + 0.5f, (float)ny + 0.5f};
          float d = Vector2Distance(e.position, decoPos);
          if (d < 0.7f && d > 0.001f) {
            Vector2 diff = Vector2Normalize(Vector2Subtract(e.position, decoPos));
            Vector2 push = Vector2Scale(diff, 0.08f);
            Vector2 nextPos = Vector2Add(e.position, push);
            if (IsWalkable((int)nextPos.x, (int)nextPos.y, true)) {
              e.position = nextPos;
            }
          }
        }
      }
    }

    // === WATER REPULSION (Prevent getting stuck in water) ===
    int curX = (int)e.position.x;
    int curY = (int)e.position.y;
    if (!IsWalkable(curX, curY, true)) {
      // Find nearest walkable tile nearby
      Vector2 bestEscape = {0, 0};
      float minDist = 999.0f;
      for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
          int nx = curX + dx, ny = curY + dy;
          if (nx >= 0 && nx < width && ny >= 0 && ny < height && IsWalkable(nx, ny, true)) {
            Vector2 target = {(float)nx + 0.5f, (float)ny + 0.5f};
            float d = Vector2Distance(e.position, target);
            if (d < minDist) {
              minDist = d;
              bestEscape = target;
            }
          }
        }
      }
      if (minDist < 999.0f) {
        Vector2 escapeDir = Vector2Normalize(Vector2Subtract(bestEscape, e.position));
        e.position = Vector2Add(e.position, Vector2Scale(escapeDir, 0.15f)); // Stronger escape
      }
    }

    // === ENTITY-ENTITY SEPARATION (Prevent overlapping) ===
    // Small displacement if too close to another entity
    for (auto &[otherID, other] : entities) {
      if (otherID == entityID || other.health <= 0) continue;
      float d = Vector2Distance(e.position, other.position);
      if (d < 0.7f && d > 0.001f) { // Increased separation radius
        Vector2 diff = Vector2Normalize(Vector2Subtract(e.position, other.position));
        Vector2 push = Vector2Scale(diff, 0.08f); // Stronger push
        Vector2 nextPos = Vector2Add(e.position, push);
        if (IsWalkable((int)nextPos.x, (int)nextPos.y, true)) {
          e.position = nextPos;
        }
      }
    }

    // === UNIVERSAL DEATH HANDLER ===
    if (e.health <= 0 || e.state == EntityState::Die) {
      e.state = EntityState::Die;
      e.health = 0;
      e.animTime += deltaTime;
      if (e.animTime >= 0.2f) {
        e.animTime = 0.0f;
        e.currentFrame++;
      }
      if (e.currentFrame >= 4) {
        if (e.IsIntelligent() && e.citizenID != -1) {
          simulation.RemoveCitizen(e.citizenID);
        }
        deadEntityIDs.push_back(entityID);
      }
      continue;
    }

    // === Sim timers ===
    e.enemyScanTimer -= deltaTime;
    e.attackCooldown -= deltaTime;
    if (e.attackCooldown < 0.0f)
      e.attackCooldown = 0.0f;

    // === ANIMAL COLD / HYPOTHERMIA SYSTEM ===
    if (!e.IsIntelligent()) {
      int atX = static_cast<int>(e.position.x);
      int atY = static_cast<int>(e.position.y);
      if (atX >= 0 && atX < width && atY >= 0 && atY < height) {
        const Tile &tile = GetTileConst(atX, atY);
        if (tile.biome == BiomeType::Snow) {
          e.bodyTemperature -= deltaTime * 1.5f;
        } else {
          // Warm up
          if (e.bodyTemperature < 37.0f) {
            e.bodyTemperature += deltaTime * 1.0f;
            if (e.bodyTemperature > 37.0f)
              e.bodyTemperature = 37.0f;
          }
        }
        // Cold damage
        if (e.bodyTemperature < 28.0f) {
          e.health -= deltaTime * 2.0f;
        }
        if (e.bodyTemperature < 0.0f)
          e.bodyTemperature = 0.0f;
      }
    }

    // Force initial scan (on spawn) or periodic scan (every 0.5s)
    if (e.justSpawned || e.enemyScanTimer <= 0.0f) {
      e.enemyScanTimer = 0.5f;
      e.justSpawned = false;

      // Determine which enemies this entity should look for
      int bestTargetID = -1;
      float bestDist = 99999.0f;

      if (e.type == EntityType::Boar || e.type == EntityType::Slime) {
        // Boars/Slimes hunt humans
        for (auto &pair : citizenEntityMap) {
          Entity *candidate = pair.second;
          if (!candidate || candidate->health <= 0)
            continue;
          float d = Vector2Distance(e.position, candidate->position);
          if (d < bestDist) {
            bestDist = d;
            bestTargetID = candidate->id;
          }
        }
        if (bestTargetID >= 0 && bestDist < 8.0f) { // Reduced from 12.0
          e.hasTarget = true;
          e.targetID = bestTargetID;
        } else {
          e.hasTarget = false;
          e.targetID = -1;
        }
      } else if (e.IsIntelligent()) {
        // Humans hunt boars and slimes
        for (const auto &[oid, other] : entities) {
          if ((other.type == EntityType::Boar ||
               other.type == EntityType::Slime) &&
              other.health > 0) {
            float d = Vector2Distance(e.position, other.position);
            if (d < bestDist) {
              bestDist = d;
              bestTargetID = other.id;
            }
          }
        }
        if (bestTargetID >= 0 && bestDist < 10.0f) {
          e.hasTarget = true;
          e.targetID = bestTargetID;
        } else {
          e.hasTarget = false;
          e.targetID = -1;
        }
      } else {
        // Non-intelligent animals do not hunt
        e.hasTarget = false;
        e.targetID = -1;
      }
    }

    // === ANIMAL REPRODUCTION SYSTEM ===
    if (!e.IsIntelligent() && e.type != EntityType::Boar &&
        e.type != EntityType::Slime && e.health > 0) {
      e.reproductionTimer += deltaTime;
      if (e.reproductionTimer >= 60.0f) { // Increased cooldown
        e.reproductionTimer = 0.0f;

        // Count current population of this species
        int speciesCount = 0;
        for (const auto &[oid, other] : entities) {
          if (other.type == e.type && other.health > 0)
            speciesCount++;
        }

        // Cap at 15 per species (Reduced from 30)
        if (speciesCount < 15) {
          // Find a mate of same species within radius 5
          bool foundMate = false;
          for (auto &[oid, mate] : entities) {
            if (oid == entityID)
              continue;
            if (mate.type == e.type && mate.health > 0 &&
                mate.state != EntityState::Die) {
              float d = Vector2Distance(e.position, mate.position);
              if (d < 5.0f) {
                foundMate = true;
                mate.reproductionTimer = 0.0f;
                break;
              }
            }
          }

          if (foundMate) {
            // Find a walkable spot nearby to spawn offspring
            for (int attempt = 0; attempt < 10; attempt++) {
              int ox = static_cast<int>(e.position.x) + GRandom.Int(-2, 2);
              int oy = static_cast<int>(e.position.y) + GRandom.Int(-2, 2);
              if (ox >= 0 && ox < width && oy >= 0 && oy < height &&
                  IsWalkable(ox, oy)) {
                Entity baby;
                baby.id = nextEntityID++;
                baby.type = e.type;
                baby.position = {(float)ox + 0.5f, (float)oy + 0.5f};
                baby.targetPos = baby.position;
                baby.state = EntityState::Idle;
                baby.facingDirection = GRandom.Int(0, 3);
                baby.speed = e.speed * 0.8f;
                baby.health = e.health * 0.5f;
                baby.animTime = 0.0f;
                baby.currentFrame = 0;
                baby.hasTarget = false;
                baby.citizenID = -1;
                baby.bodyTemperature = 37.0f;
                baby.reproductionTimer = 0.0f;
                baby.reproductionCooldown = e.reproductionCooldown;
                baby.maxHP = baby.health;
                entities[baby.id] = baby;
                TraceLog(
                    LOG_INFO,
                    "REPRODUCE: Animal type %d spawned offspring at (%d,%d)",
                    (int)e.type, ox, oy);
                break;
              }
            }
          }
        }
      }
    }

    // Basic AI
    // === BOAR AI ===
    if (e.type == EntityType::Boar || e.type == EntityType::Slime) {
      if (e.state == EntityState::Die) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.15f) {
          e.animTime = 0.0f;
          e.currentFrame++;
        }
        if (e.currentFrame >= 6)
          deadEntityIDs.push_back(entityID);
        continue;
      }
      if (e.state == EntityState::Hurt) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.1f) {
          e.animTime = 0.0f;
          e.currentFrame++;
          if (e.currentFrame >= 4)
            e.state = EntityState::Idle;
        }
        continue;
      }

      // Attack Logic
      if (e.state == EntityState::Attack) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.1f) {
          e.animTime = 0.0f;
          e.currentFrame++;
          if (e.currentFrame == 3) { // Impact
            // Predators hunt Humans AND Animals
            bool foundTarget = false;
            
            // Priority 1: Humans
            for (auto &pair : citizenEntityMap) {
              Entity *target = pair.second;
              if (target && target->health > 0) {
                if (Vector2Distance(e.position, target->position) < 1.5f) {
                  float dmg = (e.type == EntityType::Slime) ? 1.5f : 8.0f;
                  target->health -= dmg;
                  if (target->health <= 0) target->state = EntityState::Die;
                  foundTarget = true;
                  break;
                }
              }
            }
            
            // Priority 2: Animals (to control population)
            if (!foundTarget) {
              for (auto &[tid, target] : entities) {
                if (!target.IsIntelligent() && target.health > 0 && target.id != entityID) {
                  if (Vector2Distance(e.position, target.position) < 1.5f) {
                    float dmg = (e.type == EntityType::Slime) ? 5.0f : 20.0f; // Higher damage to animals
                    target.health -= dmg;
                    if (target.health <= 0) target.state = EntityState::Die;
                    break;
                  }
                }
              }
            }
          }
          if (e.currentFrame >= 5)
            e.state = EntityState::Idle;
        }
        continue;
      }

      // Resolve current target pointer (IDs are stable, pointers may rehash)
      Entity *target = nullptr;
      if (e.hasTarget && e.targetID >= 0) {
        target = GetEntityByID(e.targetID);
        if (!target || target->health <= 0) {
          e.hasTarget = false;
          e.targetID = -1;
          target = nullptr;
        }
      }

      if (target) {
        float dist = Vector2Distance(e.position, target->position);

        // Attack when in range and off cooldown
        if (dist < 1.0f && e.attackCooldown <= 0.0f) {
          e.state = EntityState::Attack;
          e.currentFrame = 0;
          e.attackCooldown = e.attackSpeed;
        } else if (dist >= 1.0f) {
          // Move towards target
          e.state = EntityState::Run;
          Vector2 dir =
              Vector2Normalize(Vector2Subtract(target->position, e.position));
          Vector2 moveVec = Vector2Scale(dir, e.speed * 1.5f * deltaTime);
          Vector2 nextPos = Vector2Add(e.position, moveVec);

          if (IsWalkable((int)nextPos.x, (int)nextPos.y)) {
            e.position = nextPos;
          } else if (IsWalkable((int)nextPos.x, (int)e.position.y)) {
            e.position.x = nextPos.x;
          } else if (IsWalkable((int)e.position.x, (int)nextPos.y)) {
            e.position.y = nextPos.y;
          } else {
            e.state = EntityState::Idle;
          }

          // Facing based on direction
          if (fabs(dir.x) > fabs(dir.y))
            e.facingDirection = (dir.x > 0) ? 1 : -1;
          else
            e.facingDirection = (dir.y > 0) ? 0 : 2;
        }
      } else {
        // Random Wander
        if (e.state != EntityState::Walking)
          e.state = EntityState::Walking;

        // Randomly change direction
        if (rng_.Int(0, 50) == 0) {
          int r = rng_.Int(0, 4);
          if (r == 0)
            e.facingDirection = 0;
          else if (r == 1)
            e.facingDirection = 1;
          else if (r == 2)
            e.facingDirection = -1;
          else
            e.facingDirection = 2;
        }

        Vector2 dir = {0, 0};
        if (e.facingDirection == 0)
          dir.y = 1;
        else if (e.facingDirection == 1)
          dir.x = 1;
        else if (e.facingDirection == -1)
          dir.x = -1;
        else
          dir.y = -1;

        if (rng_.Int(0, 10) > 1) { // Move 80% of time
          Vector2 next = Vector2Add(
              e.position, Vector2Scale(dir, e.speed * 0.5f * deltaTime));
          if (IsWalkable((int)next.x, (int)next.y))
            e.position = next;
          else {
            // Hit wall, turn
            e.facingDirection = (e.facingDirection + 1) % 4; // Simple turn
          }
        }
      }

      // Anim Ticker
      e.animTime += deltaTime;
      if (e.animTime > 0.1f) {
        e.animTime = 0.0f;
        e.currentFrame++;
      }
      continue;
    }

    // === DRAGON AI ===
    else if (e.type == EntityType::Dragon) {
      if (e.state == EntityState::Die) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.2f) {
          e.animTime = 0.0f;
          e.currentFrame++;
        }
        if (e.currentFrame >= 6)
          deadEntityIDs.push_back(entityID);
        continue;
      }
      if (e.state == EntityState::Hurt) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.15f) {
          e.animTime = 0.0f;
          e.currentFrame++;
          if (e.currentFrame >= 4)
            e.state = EntityState::Idle;
        }
        continue;
      }

      // Attack Logic
      if (e.state == EntityState::Attack) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.2f) {
          e.animTime = 0.0f;
          e.currentFrame++;
          if (e.currentFrame == 3) { // Impact
            // Visual Fire Effect at target position
            Vector2 targetVfxPos = e.position;
            Entity* targetEntity = GetEntityByID(e.targetID);
            if (targetEntity) targetVfxPos = targetEntity->position;
            AddSpawnEffect((int)targetVfxPos.x, (int)targetVfxPos.y, {255, 100, 0, 200}, VfxType::Fire);
            
            // Attack humans and destroy buildings
            for (auto &pair : citizenEntityMap) {
              Entity *target = pair.second;
              if (target && target->health > 0) {
                float dToImpact = Vector2Distance(targetVfxPos, target->position);
                if (dToImpact < 4.0f) { // Impact radius from target
                  float dmg = 50.0f; // Dragon damage
                  target->health -= dmg;
                  if (rng_.Int(0, 100) < 40) {
                    AddSpawnEffect((int)target->position.x, (int)target->position.y, {255, 50, 0, 180}, VfxType::Fire);
                  }
                  if (target->health <= 0)
                    target->state = EntityState::Die;
                }
              }
            }
            // Destroy buildings
            int tx = (int)e.position.x;
            int ty = (int)e.position.y;
            simulation.DestroyBuildingsAtTile(tx, ty);
            // Also destroy adjacent tiles (larger area for Dragon)
            for (int dy = -2; dy <= 2; dy++) {
              for (int dx = -2; dx <= 2; dx++) {
                simulation.DestroyBuildingsAtTile(tx + dx, ty + dy);
              }
            }
          }
          if (e.currentFrame >= 5)
            e.state = EntityState::Idle;
        }
        continue;
      }

      // Dragon behavior: patrol and attack humans
      bool foundTarget = false;
      Vector2 targetPos = e.position;
      float minDist = 20.0f; // Larger range
      for (const auto &pair : citizenEntityMap) {
        Entity *target = pair.second;
        if (target && target->health > 0) {
          float d = Vector2Distance(e.position, target->position);
          if (d < minDist) {
            minDist = d;
            targetPos = target->position;
            foundTarget = true;
          }
        }
      }

      if (foundTarget) {
        if (minDist < 2.0f) {
          e.state = EntityState::Attack;
          e.currentFrame = 0;
          e.animTime = 0.0f;
        } else {
          e.state = EntityState::Run;
          Vector2 dir = Vector2Subtract(targetPos, e.position);
          if (fabs(dir.x) > fabs(dir.y))
            e.facingDirection = (dir.x > 0) ? 1 : -1;
          else
            e.facingDirection = (dir.y > 0) ? 0 : 2;

          dir = Vector2Normalize(dir);
          Vector2 moveVec = Vector2Scale(dir, e.speed * deltaTime);
          Vector2 nextPos = Vector2Add(e.position, moveVec);

          if (IsWalkable((int)nextPos.x, (int)nextPos.y)) {
            e.position = nextPos;
          } else {
            // Occasionally fly over obstacles
            if (rng_.Int(0, 100) < 10) { // 10% chance to fly
              e.position = nextPos; // Ignore walkable for flying
            } else {
              e.state = EntityState::Idle;
            }
          }
        }
      } else {
        // Patrol: random movement
        if (e.state != EntityState::Walking)
          e.state = EntityState::Walking;

        if (rng_.Int(0, 100) < 5) { // Change direction occasionally
          e.facingDirection = rng_.Int(0, 4);
        }

        Vector2 dir = {0, 0};
        if (e.facingDirection == 0) dir.y = 1;
        else if (e.facingDirection == 1) dir.x = 1;
        else if (e.facingDirection == 2) dir.y = -1;
        else dir.x = -1;

        Vector2 next = Vector2Add(e.position, Vector2Scale(dir, e.speed * 0.5f * deltaTime));
        if (IsWalkable((int)next.x, (int)next.y)) {
          e.position = next;
        } else {
          e.facingDirection = (e.facingDirection + 1) % 4;
        }
      }

      // Anim
      e.animTime += deltaTime;
      if (e.animTime > 0.15f) {
        e.animTime = 0.0f;
        e.currentFrame++;
      }
      continue;
    }

    // === HUMAN AI ===
    else if (e.type == EntityType::HumanUnarmed ||
             e.type == EntityType::HumanArmed ||
             e.type == EntityType::HumanWoman) {
      // Skip AI if player controlled
      if (e.isPlayerControlled) {
        // Still update timers and universal handlers
        e.enemyScanTimer -= deltaTime;
        e.attackCooldown -= deltaTime;
        if (e.attackCooldown < 0.0f) e.attackCooldown = 0.0f;
        continue;
      }

      // Death
      if (e.state == EntityState::Die) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.2f) {
          e.animTime = 0.0f;
          e.currentFrame++;
        }
        if (e.currentFrame >= 4)
          deadEntityIDs.push_back(entityID);
        continue;
      }
      // Block
      if (e.state == EntityState::Block) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.3f)
          e.state = EntityState::Idle;
        continue;
      }

      // Hurt (Fix: Humans freezing after damage)
      if (e.state == EntityState::Hurt) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.25f) { // Hurt animation time
          e.animTime = 0.0f;
          e.state = EntityState::Idle;
        }
        continue;
      }

      // Attack
      if (e.state == EntityState::Attack) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.20f) { // Faster attack (was 0.30)
          e.animTime = 0.0f;
          e.currentFrame++;
          if (e.currentFrame == 2) {
            for (auto &[tid, target] : entities) {
              if ((target.type == EntityType::Boar ||
                   target.type == EntityType::Slime ||
                   target.type == EntityType::Dragon) &&
                  target.health > 0) {
                float dist = Vector2Distance(e.position, target.position);
                if (dist < 2.0f) { // Increased range from 1.5
                  Vector2 dirTo = Vector2Normalize(
                      Vector2Subtract(target.position, e.position));
                  bool facing = false;
                  // HUMANS ATTACK RADIUS ONLY (No facing needed)
                  if (e.IsIntelligent()) {
                    facing = true; // Always facing if in range
                  } else {
                    if (e.facingDirection == 0 && dirTo.y > 0.3f) facing = true;
                    else if (e.facingDirection == 1 && dirTo.x > 0.3f) facing = true;
                    else if (e.facingDirection == -1 && dirTo.x < -0.3f) facing = true;
                    else if (e.facingDirection == 2 && dirTo.y < -0.3f) facing = true;
                    if (dist < 0.6f) facing = true;
                  }

                  if (facing) {
                    float dmg =
                        (e.type == EntityType::HumanArmed) ? 25.0f : 12.0f; // Buffed
                    // Hero bonus damage
                    if (e.isHero) dmg += 15.0f;
                    target.health -= dmg;
                    target.state = EntityState::Hurt;
                    target.currentFrame = 0;
                    if (target.health <= 0) {
                      target.state = EntityState::Die;
                      // Hero system: increment kills
                      if (e.IsIntelligent()) {
                        e.kills++;
                        // Check if becomes hero
                        if (!e.isHero && e.kills >= 5) {
                          e.isHero = true;
                          e.level = 1;
                          e.maxHP += 20; // Bonus HP
                          e.health = e.maxHP; // Heal to full
                          // Notification
                          std::string name = "Unknown";
                          if (e.citizenID >= 0) {
                            Citizen *c = simulation.GetCitizen(e.citizenID);
                            if (c) name = c->name;
                          }
                          // Add notification via UIManager, but since we don't have access, use TraceLog for now
                          TraceLog(LOG_INFO, "HERO: %s has become a Hero!", name.c_str());
                        }
                      }
                    }
                    TraceLog(LOG_INFO, "COMBAT: Human Hit Enemy!");
                  }
                }
              }
            }
          }
          int maxFrames = 4;
          if (e.type == EntityType::HumanUnarmed) {
            int dirIdx = 0;
            if (e.facingDirection == 1)
              dirIdx = 1;
            else if (e.facingDirection == -1)
              dirIdx = 2;
            else if (e.facingDirection == 2)
              dirIdx = 3;

            if (!resourceManager.texHumanUnarmed[2][dirIdx].empty()) {
              maxFrames =
                  (int)resourceManager.texHumanUnarmed[2][dirIdx].size();
            }
          }

          if (e.currentFrame >= maxFrames)
            e.state = EntityState::Idle;
        }
        continue;
      }

      // Hunt Boars, Slimes and Dragons
      Entity *target = nullptr;
      if (e.hasTarget && e.targetID >= 0) {
        // Lookup target by entity ID from the map
        target = GetEntityByID(e.targetID);
        if (!target || target->health <= 0) {
          e.hasTarget = false;
          e.targetID = -1;
          target = nullptr;
        }
      }

      if (target) {
        float dist = Vector2Distance(e.position, target->position);

        // Attack when in range and off cooldown - Increased range to 1.8f
        if (dist < 1.8f && e.attackCooldown <= 0.0f) {
          e.state = EntityState::Attack;
          e.currentFrame = 0;
          e.animTime = 0.0f;
          e.attackCooldown = e.attackSpeed;
        } else if (dist >= 1.8f) {
          // Move logic
          e.state = EntityState::Walking;
          Vector2 dir = Vector2Subtract(target->position, e.position);

          if (fabs(dir.x) > fabs(dir.y)) {
            e.facingDirection = (dir.x > 0) ? 1 : -1;
          } else {
            e.facingDirection = (dir.y > 0) ? 0 : 2;
          }

          dir = Vector2Normalize(dir);
          Vector2 moveVec = Vector2Scale(dir, e.speed * deltaTime);
          Vector2 nextPos = Vector2Add(e.position, moveVec);

          if (IsWalkable((int)nextPos.x, (int)nextPos.y, false)) {
            e.position = nextPos;
          } else if (IsWalkable((int)nextPos.x, (int)e.position.y, false)) {
            e.position.x = nextPos.x;
          } else if (IsWalkable((int)e.position.x, (int)nextPos.y, false)) {
            e.position.y = nextPos.y;
          } else {
            e.state = EntityState::Idle;
            e.hasTarget = false;
          }
        }
      }

      // SELF DEFENSE: Scan for nearby enemies if idle or wandering
      if (!target && e.enemyScanTimer <= 0.0f) {
        e.enemyScanTimer = 1.0f; // Scan every second
        float scanRange = 8.0f;
        int bestEnemyID = -1;
        float bestEnemyDist = 999.0f;
        
        for (auto &[tid, other] : entities) {
          if ((other.type == EntityType::Boar || other.type == EntityType::Slime || other.type == EntityType::Dragon) && other.health > 0) {
            float d = Vector2Distance(e.position, other.position);
            if (d < scanRange && d < bestEnemyDist) {
              bestEnemyDist = d;
              bestEnemyID = other.id;
            }
          }
        }
        if (bestEnemyID >= 0) {
          e.targetID = bestEnemyID;
          e.hasTarget = true;
          target = GetEntityByID(e.targetID);
        }
      }

      bool skipWander = false;
      if (e.citizenID >= 0) {
        Citizen *citizen = simulation.GetCitizen(e.citizenID);
        if (citizen && citizen->isWorking) {
          skipWander = true;
        }
      }

      if (target) {
        // (The logic above handled movement/attack)
      } else {
        if (!skipWander && rng_.Int(0, 100) < 2) {
          float tx = e.position.x;
          float ty = e.position.y;

          // Check if this human belongs to a city
          bool hasValidTarget = false;
          if (e.citizenID >= 0) {
            Citizen *citizen = simulation.GetCitizen(e.citizenID);
            if (citizen && citizen->cityID >= 0) {
              City *city = simulation.GetCity(citizen->cityID);
              if (city && !city->territory.empty()) {
                // Pick a random tile within city territory
                int randIdx = rng_.Int(0, (int)city->territory.size());
                tx = city->territory[randIdx].x + 0.5f;
                ty = city->territory[randIdx].y + 0.5f;

                // Make sure it's walkable
                if (IsWalkable((int)tx, (int)ty)) {
                  hasValidTarget = true;
                }
              }
            }
          }

          // Fallback to random wandering if no city or invalid target
          if (!hasValidTarget) {
            int wanderDir = rng_.Int(0, 4);
            float wanderDist = 2.0f + rng_.Float() * 3.0f;

            if (wanderDir == 0)
              ty -= wanderDist;
            else if (wanderDir == 1)
              ty += wanderDist;
            else if (wanderDir == 2)
              tx -= wanderDist;
            else
              tx += wanderDist;

            hasValidTarget = IsWalkable((int)tx, (int)ty);
          }

          if (hasValidTarget) {
            e.targetPos = {tx, ty};
            e.hasTarget = true;
          }
        }

        if (e.hasTarget && e.targetID < 0) { // Wander target
          Vector2 dir = Vector2Subtract(e.targetPos, e.position);
          if (Vector2Length(dir) < 0.1f) {
            e.hasTarget = false;
            e.state = EntityState::Idle;
          } else {
            // Check if in water to enter Swim state
            int atX = static_cast<int>(e.position.x);
            int atY = static_cast<int>(e.position.y);
            bool inWater = false;
            if (atX >= 0 && atX < width && atY >= 0 && atY < height) {
              TileType t = GetTileConst(atX, atY).type;
              inWater = (t == TileType::ShallowOcean || t == TileType::Ocean || t == TileType::DeepOcean);
            }

            if (inWater) {
              e.state = EntityState::Swim;
            } else {
              e.state = EntityState::Walking;
            }

            // Force cardinal direction (no diagonal) - REMOVED
            if (fabs(dir.x) > fabs(dir.y)) {
              // dir.y = 0; // Don't zero out
              e.facingDirection = (dir.x > 0) ? 1 : -1;
            } else {
              // dir.x = 0; // Don't zero out
              e.facingDirection = (dir.y > 0) ? 0 : 2;
            }

            dir = Vector2Normalize(dir);
            float currentSpeed = e.speed;
            if (e.state == EntityState::Run) currentSpeed *= 1.5f;
            else if (e.state == EntityState::Swim) currentSpeed *= 0.5f;
            
            Vector2 moveVec = Vector2Scale(dir, currentSpeed * deltaTime);
            Vector2 nextPos = Vector2Add(e.position, moveVec);

            if (IsWalkable((int)nextPos.x, (int)nextPos.y, false) || (e.state == EntityState::Swim && IsSwimmable((int)nextPos.x, (int)nextPos.y))) {
              e.position = nextPos;
            } else if (IsWalkable((int)nextPos.x, (int)e.position.y, false) || (e.state == EntityState::Swim && IsSwimmable((int)nextPos.x, (int)e.position.y))) {
              e.position.x = nextPos.x;
            } else if (IsWalkable((int)e.position.x, (int)nextPos.y, false) || (e.state == EntityState::Swim && IsSwimmable((int)e.position.x, (int)nextPos.y))) {
              e.position.y = nextPos.y;
            } else {
              // Path blocked
              e.hasTarget = false; 
              e.state = EntityState::Idle;
            }
          }
        } else if (!e.hasTarget && e.state == EntityState::Walking) {
          // Only reset to Idle if we were Walking.
          // If we were Attacking (Working), don't reset!
          e.state = EntityState::Idle;
        }
      }

      // Anim
      if (e.state == EntityState::Walking || e.state == EntityState::Run || e.state == EntityState::Swim) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.1f) {
          e.animTime = 0.0f;
          if (e.type == EntityType::HumanUnarmed) {
            e.currentFrame++;
          } else {
            e.currentFrame = (e.currentFrame + 1) % 4;
          }
        }
      }
    }

    // ========================================================================
    // ANIMAL AI - Simple wander behavior
    // ========================================================================
    else if (e.type == EntityType::Cow || e.type == EntityType::Chicken ||
             e.type == EntityType::Sheep || e.type == EntityType::Bull ||
             e.type == EntityType::Chicken2 || e.type == EntityType::Lamb ||
             e.type == EntityType::Pig || e.type == EntityType::Turkey) {
      // Random Wander
      if (e.state != EntityState::Walking)
        e.state = EntityState::Walking;

      // Randomly change direction (less frequent than Boar to seem calmlier)
      if (rng_.Int(0, 150) == 0) {
        int r = rng_.Int(0, 4);
        if (r == 0)
          e.facingDirection = 0;
        else if (r == 1)
          e.facingDirection = 1;
        else if (r == 2)
          e.facingDirection = -1;
        else
          e.facingDirection = 2;
      }

      Vector2 dir = {0, 0};
      if (e.facingDirection == 0)
        dir.y = 1;
      else if (e.facingDirection == 1)
        dir.x = 1;
      else if (e.facingDirection == -1)
        dir.x = -1;
      else
        dir.y = -1;

      // Move constantly but slowly (their e.speed is natively low, e.g. 0.5f)
      Vector2 next =
          Vector2Add(e.position, Vector2Scale(dir, e.speed * deltaTime));


      if (IsWalkable((int)next.x, (int)next.y, false)) {
        e.position = next;
      } else {
        // Hit wall, bounce to a different direction
        e.facingDirection = (e.facingDirection + 1) % 4;
      }

      // Animation (6 frames per direction for animals)
      if (e.state == EntityState::Walking) {
        e.animTime += deltaTime;
        float animSpeed = (e.type == EntityType::Chicken) ? 0.1f : 0.15f;
        if (e.animTime >= animSpeed) {
          e.animTime = 0.0f;
          e.currentFrame = (e.currentFrame + 1) % 6;
        }
      } else {
        e.currentFrame = 0;
      }
    }
  }

  // === DEFERRED REMOVAL of dead entities ===
  for (int deadID : deadEntityIDs) {
    entities.erase(deadID);
  }
  if (!deadEntityIDs.empty()) {
    RebuildEntityCache();
  }
}

// ============================================================================
// SPAWN EFFECT SYSTEM
// ============================================================================

void World::AddSpawnEffect(int tileX, int tileY, Color color, VfxType type) {
  // Rate limit to avoid too many effects
  if (spawnEffects.size() > 100)
    return;

  SpawnEffect fx;
  fx.worldX = tileX * 10.0f + 5.0f;
  fx.worldY = tileY * 10.0f + 5.0f;
  fx.timer = 0.0f;
  fx.duration =
      (type == VfxType::Default) ? 1.0f :
      (type == VfxType::Fire) ? 2.5f :
      (type == VfxType::Lightning) ? 1.5f :
      (type == VfxType::Tornado) ? 2.0f :
      (type == VfxType::FireBomb) ? 1.8f :
      (type == VfxType::DarkBolt) ? 1.5f :
      (type == VfxType::Thunder2) ? 1.5f : 0.8f;
  fx.scale = 0.0f;
  fx.type = type;
  fx.currentFrame = 0;

  // Create 6-10 sparkle particles
  int numParticles = GRandom.Int(6, 10);
  for (int i = 0; i < numParticles; i++) {
    SpawnParticle p;
    float angle = (float)GRandom.Int(0, 359) * DEG2RAD;
    float dist = (float)GRandom.Int(0, 39) / 10.0f;
    p.pos = {fx.worldX + cosf(angle) * dist, fx.worldY + sinf(angle) * dist};

    float speed = 8.0f + (float)GRandom.Int(0, 19);
    p.velocity = {cosf(angle) * speed, -5.0f - (float)GRandom.Int(0, 14)};

    p.maxLife = 0.3f + (float)GRandom.Int(0, 49) / 100.0f;
    p.lifetime = p.maxLife;

    p.color = color;
    p.color.r = (unsigned char)std::min(255, (int)color.r + GRandom.Int(-30, 29));
    p.color.g = (unsigned char)std::min(255, (int)color.g + GRandom.Int(-30, 29));
    p.color.b = (unsigned char)std::min(255, (int)color.b + GRandom.Int(-30, 29));
    p.color.a = 255;

    p.size = 1.0f + (float)GRandom.Int(0, 19) / 10.0f;

    fx.particles.push_back(p);
  }

  spawnEffects.push_back(fx);
}

void World::UpdateSpawnEffects(float dt) {
  for (auto it = spawnEffects.begin(); it != spawnEffects.end();) {
    it->timer += dt;

    // Scale-in animation
    if (it->timer < 0.15f) {
      it->scale = it->timer / 0.15f;
    } else {
      it->scale = 1.0f;
    }

    // Update particles
    for (auto &p : it->particles) {
      p.pos.x += p.velocity.x * dt;
      p.pos.y += p.velocity.y * dt;
      p.velocity.y += 20.0f * dt; // Gravity
      p.lifetime -= dt;
      p.size = std::max(0.0f, p.size - dt * 2.0f);
    }

    // Remove expired effects
    if (it->timer >= it->duration) {
      it = spawnEffects.erase(it);
    } else {
      ++it;
    }
  }
}

void World::DrawSpawnEffects(const ResourceManager &resourceManager) const {
  for (const auto &fx : spawnEffects) {
    // Draw particles
    for (const auto &p : fx.particles) {
      if (p.lifetime <= 0.0f)
        continue;
      float alpha = p.lifetime / p.maxLife;
      Color c = p.color;
      c.a = (unsigned char)(alpha * 255.0f);
      DrawCircleV(p.pos, p.size, c);
    }

    // Draw a scale-in ring at spawn point
    if (fx.timer < fx.duration * 0.2f) {
      float ringAlpha = 1.0f - (fx.timer / 0.3f);
      float ringRadius = 3.0f + fx.timer * 25.0f;
      Color ringColor = {255, 255, 200, (unsigned char)(ringAlpha * 120.0f)};
      DrawCircleLines((int)fx.worldX, (int)fx.worldY, ringRadius, ringColor);
    }

    // Type-specific VFX
    if (fx.type == VfxType::Lightning) {
      // Animated Thunder Strike using sprite textures (12 frames)
      int totalThunder = (int)resourceManager.texThunderStrike.size();
      if (totalThunder > 0) {
        float progress = fx.timer / fx.duration; // 0.0 to 1.0
        int frameIdx = (int)(progress * totalThunder);
        if (frameIdx >= totalThunder) frameIdx = totalThunder - 1;

        Texture2D thunderTex = resourceManager.texThunderStrike[frameIdx];
        float thunderScale = 1.0f;
        Rectangle thunderSrc = {0, 0, (float)thunderTex.width, (float)thunderTex.height};
        Rectangle thunderDest = {
          fx.worldX - (thunderTex.width * thunderScale / 2.0f),
          fx.worldY - (thunderTex.height * thunderScale * 0.95f),
          thunderTex.width * thunderScale,
          thunderTex.height * thunderScale
        };
        Color tint = WHITE;
        // Fade out in the last 15%
        if (progress > 0.85f) {
          tint.a = (unsigned char)(255 * (1.0f - (progress - 0.85f) / 0.15f));
        }
        DrawTexturePro(thunderTex, thunderSrc, thunderDest, {0, 0}, 0.0f, tint);

        // Subtle flash at impact point during early frames
        if (progress < 0.4f) {
          float flashAlpha = (1.0f - progress / 0.4f) * 150.0f;
          DrawCircleGradient((int)fx.worldX, (int)fx.worldY, 30.0f,
                             {255, 255, 200, (unsigned char)flashAlpha},
                             {255, 255, 200, 0});
        }
      }
    } else if (fx.type == VfxType::Fire) {
      // Animated Fire using sprite textures
      int totalStart = (int)resourceManager.texFireStart.size();
      int totalLoop = (int)resourceManager.texFireLoop.size();
      int totalEnd = (int)resourceManager.texFireEnd.size();
      int totalFrames = totalStart + totalLoop + totalEnd;

      if (totalStart > 0 && totalLoop > 0) {
        float progress = fx.timer / fx.duration; // 0.0 to 1.0
        Texture2D fireTex;
        bool hasFire = false;

        if (progress < 0.2f && totalStart > 0) {
          // Start phase (first 20% of duration)
          int idx = (int)(progress / 0.2f * totalStart) % totalStart;
          fireTex = resourceManager.texFireStart[idx];
          hasFire = true;
        } else if (progress < 0.75f && totalLoop > 0) {
          // Loop phase (20% to 75% of duration)
          float loopProgress = (progress - 0.2f) / 0.55f;
          int cycles = (int)(loopProgress * totalLoop * 3) % totalLoop; // 3 full loops
          fireTex = resourceManager.texFireLoop[cycles];
          hasFire = true;
        } else if (totalEnd > 0) {
          // End phase (last 25%)
          float endProgress = (progress - 0.75f) / 0.25f;
          int idx = (int)(endProgress * totalEnd);
          if (idx >= totalEnd) idx = totalEnd - 1;
          fireTex = resourceManager.texFireEnd[idx];
          hasFire = true;
        }

        if (hasFire) {
          float fireScale = 2.0f;
          Rectangle fireSrc = {0, 0, (float)fireTex.width, (float)fireTex.height};
          Rectangle fireDest = {fx.worldX - (fireTex.width * fireScale / 2.0f),
                                fx.worldY - (fireTex.height * fireScale * 0.8f),
                                fireTex.width * fireScale,
                                fireTex.height * fireScale};
          Color tint = WHITE;
          tint.a = (progress > 0.85f) ? (unsigned char)(255 * (1.0f - (progress - 0.85f) / 0.15f)) : 255;
          DrawTexturePro(fireTex, fireSrc, fireDest, {0, 0}, 0.0f, tint);
        }
      }
    } else if (fx.type == VfxType::Tornado || fx.type == VfxType::FireBomb ||
               fx.type == VfxType::DarkBolt || fx.type == VfxType::Thunder2) {
      // Generic sprite animation for new spell types
      const std::vector<Texture2D> *texVec = nullptr;
      float spellScale = 1.0f;
      float anchorY = 0.9f; // How far down the sprite the hit point is (0.9 = bottom 10%)

      if (fx.type == VfxType::Tornado) {
        texVec = &resourceManager.texTornado;
        spellScale = 1.5f;
        anchorY = 0.95f;
      } else if (fx.type == VfxType::FireBomb) {
        texVec = &resourceManager.texFireBomb;
        spellScale = 1.5f;
        anchorY = 0.7f;
      } else if (fx.type == VfxType::DarkBolt) {
        texVec = &resourceManager.texDarkBolt;
        spellScale = 1.0f;
        anchorY = 0.95f;
      } else if (fx.type == VfxType::Thunder2) {
        texVec = &resourceManager.texThunder2;
        spellScale = 1.0f;
        anchorY = 0.95f;
      }

      if (texVec && !texVec->empty()) {
        int totalFrames = (int)texVec->size();
        float progress = fx.timer / fx.duration;
        int frameIdx = (int)(progress * totalFrames);
        if (frameIdx >= totalFrames) frameIdx = totalFrames - 1;

        Texture2D spellTex = (*texVec)[frameIdx];
        Rectangle spellSrc = {0, 0, (float)spellTex.width, (float)spellTex.height};
        Rectangle spellDest = {
          fx.worldX - (spellTex.width * spellScale / 2.0f),
          fx.worldY - (spellTex.height * spellScale * anchorY),
          spellTex.width * spellScale,
          spellTex.height * spellScale
        };
        Color tint = WHITE;
        if (progress > 0.85f) {
          tint.a = (unsigned char)(255 * (1.0f - (progress - 0.85f) / 0.15f));
        }
        DrawTexturePro(spellTex, spellSrc, spellDest, {0, 0}, 0.0f, tint);
      }
    }
  }
}

// ============================================================================
// HAND OF GOD - DROP ENTITY
// ============================================================================
void World::DropEntity(int entityID, Vector2 dropPos) {
  Entity *e = GetEntityByID(entityID);
  if (!e) return;

  int tx = (int)dropPos.x;
  int ty = (int)dropPos.y;

  // Clamp to world bounds
  if (tx < 0) tx = 0;
  if (ty < 0) ty = 0;
  if (tx >= width) tx = width - 1;
  if (ty >= height) ty = height - 1;

  e->isGrabbed = false;

  // Check if dropped on water -> drown!
  if (IsSwimmable(tx, ty)) {
    e->health = 0;
    e->state = EntityState::Die;
    e->animTime = 0.0f;
    e->currentFrame = 0;
    TraceLog(LOG_INFO, "HAND OF GOD: Entity %d drowned at (%d, %d)!", entityID, tx, ty);

    // Spawn splash effect
    AddSpawnEffect(tx, ty, (Color){100, 150, 255, 200}, VfxType::Default);
    return;
  }

  // Find nearest walkable tile if dropped on obstacle
  if (!IsWalkable(tx, ty)) {
    bool found = false;
    for (int r = 1; r <= 5 && !found; r++) {
      for (int dx = -r; dx <= r && !found; dx++) {
        for (int dy = -r; dy <= r && !found; dy++) {
          int nx = tx + dx, ny = ty + dy;
          if (nx >= 0 && ny >= 0 && nx < width && ny < height && IsWalkable(nx, ny)) {
            tx = nx;
            ty = ny;
            found = true;
          }
        }
      }
    }
  }

  // Place entity on valid ground
  e->position = {tx + 0.5f, ty + 0.5f};
  e->state = EntityState::Idle;
  e->hasTarget = false;
  e->targetID = -1;
  e->animTime = 0.0f;
  e->currentFrame = 0;

  // Reset citizen AI if it's a human
  if (e->citizenID != -1) {
    Citizen *c = simulation.GetCitizen(e->citizenID);
    if (c) {
      c->workState = Citizen::WorkState::Idle;
      c->targetEntityID = -1;
      c->targetTileX = -1;
      c->targetTileY = -1;
      c->isWorking = false;
      c->stateTimer = 0.0f;
      c->workTimer = 0.0f;
      TraceLog(LOG_INFO, "HAND OF GOD: Citizen '%s' dropped at (%d, %d) - AI reset.",
               c->name.c_str(), tx, ty);
    }
  }
}

// ============================================================================
// GOD POWERS
// ============================================================================
void World::TriggerGodPower(int powerIndex, int tx, int ty) {
  if (tx < 0 || ty < 0 || tx >= width || ty >= height)
    return;

  // Use GRID coordinates for distance checks to match e.position
  Vector2 impactGrid = {(float)tx + 0.5f, (float)ty + 0.5f};

  if (powerIndex == 0) { // Lightning
    AddSpawnEffect(tx, ty, {200, 200, 255, 255}, VfxType::Lightning);

    // Apply AOE Damage (Radius in TILES)
    float radius = 4.5f;
    for (auto &[eid, e] : entities) {
      if (e.health <= 0)
        continue;
      float d = Vector2Distance(e.position, impactGrid);
      if (d < radius) {
        e.TakeDamage(40.0f); // Strong hit, uses TakeDamage for state sync
      }
    }
  } else if (powerIndex == 1) { // Fire
    AddSpawnEffect(tx, ty, {255, 150, 50, 255}, VfxType::Fire);

    // Impact area logic (3x3 grid for spreading)
    for (int dy = -1; dy <= 1; dy++) {
      for (int dx = -1; dx <= 1; dx++) {
        int nx = tx + dx;
        int ny = ty + dy;
        if (nx < 0 || nx >= width || ny < 0 || ny >= height)
          continue;

        Tile &t = GetTile(nx, ny);
        if (t.decoration == DecorationType::Tree ||
            t.decoration == DecorationType::PineTree ||
            t.decoration == DecorationType::PalmTree) {
          t.decoration = DecorationType::None;
          AddSpawnEffect(nx, ny, {255, 100, 0, 255}, VfxType::Fire);
        }
      }
    }

    // Damage entities in radius
    float radius = 3.5f;
    for (auto &[eid, e] : entities) {
      if (e.health <= 0)
        continue;
      float d = Vector2Distance(e.position, impactGrid);
      if (d < radius) {
        e.TakeDamage(15.0f);
        AddSpawnEffect((int)e.position.x, (int)e.position.y, {255, 120, 0, 255},
                       VfxType::Fire);
      }
    }
  } else if (powerIndex == 2) { // Tornado
    AddSpawnEffect(tx, ty, {180, 220, 255, 255}, VfxType::Tornado);
    // Tornado: knock entities and destroy decorations in radius
    float radius = 5.0f;
    for (int dy = -2; dy <= 2; dy++) {
      for (int dx = -2; dx <= 2; dx++) {
        int nx = tx + dx;
        int ny = ty + dy;
        if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
        Tile &t = GetTile(nx, ny);
        if (t.decoration != DecorationType::None) {
          t.decoration = DecorationType::None;
        }
      }
    }
    for (auto &[eid, e] : entities) {
      if (e.health <= 0) continue;
      float d = Vector2Distance(e.position, impactGrid);
      if (d < radius) {
        e.TakeDamage(20.0f);
      }
    }
  } else if (powerIndex == 3) { // Fire Bomb
    AddSpawnEffect(tx, ty, {255, 120, 30, 255}, VfxType::FireBomb);
    // Fire Bomb: big explosion, destroy trees, damage entities
    for (int dy = -2; dy <= 2; dy++) {
      for (int dx = -2; dx <= 2; dx++) {
        int nx = tx + dx;
        int ny = ty + dy;
        if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
        Tile &t = GetTile(nx, ny);
        if (t.decoration == DecorationType::Tree ||
            t.decoration == DecorationType::PineTree ||
            t.decoration == DecorationType::PalmTree ||
            t.decoration == DecorationType::Bush) {
          t.decoration = DecorationType::None;
          AddSpawnEffect(nx, ny, {255, 80, 0, 255}, VfxType::Fire);
        }
      }
    }
    float radius = 5.0f;
    for (auto &[eid, e] : entities) {
      if (e.health <= 0) continue;
      float d = Vector2Distance(e.position, impactGrid);
      if (d < radius) {
        e.TakeDamage(30.0f);
        AddSpawnEffect((int)e.position.x, (int)e.position.y, {255, 80, 0, 255}, VfxType::Fire);
      }
    }
  } else if (powerIndex == 4) { // Dark Bolt
    AddSpawnEffect(tx, ty, {120, 50, 180, 255}, VfxType::DarkBolt);
    float radius = 4.0f;
    for (auto &[eid, e] : entities) {
      if (e.health <= 0) continue;
      float d = Vector2Distance(e.position, impactGrid);
      if (d < radius) {
        e.TakeDamage(50.0f); // Very strong single target
      }
    }
  } else if (powerIndex == 5) { // Thunder 2
    AddSpawnEffect(tx, ty, {200, 200, 255, 255}, VfxType::Thunder2);
    float radius = 4.5f;
    for (auto &[eid, e] : entities) {
      if (e.health <= 0) continue;
      float d = Vector2Distance(e.position, impactGrid);
      if (d < radius) {
        e.TakeDamage(35.0f);
      }
    }
  }
}

void to_json(nlohmann::json &j, const World &w) {
  // Convert entity map to vector for JSON serialization (backward compatible)
  std::vector<Entity> entityVec;
  entityVec.reserve(w.entities.size());
  for (const auto &[id, e] : w.entities) {
    entityVec.push_back(e);
  }

  j = nlohmann::json{{"width", w.width},
                     {"height", w.height},
                     {"seed", w.seed_},
                     {"tiles", w.tiles},
                     {"nextEntityID", w.nextEntityID},
                     {"entities", entityVec},
                     {"simulation", w.simulation}};
}

void from_json(const nlohmann::json &j, World &w) {
  j.at("width").get_to(w.width);
  j.at("height").get_to(w.height);
  j.at("seed").get_to(w.seed_);
  j.at("tiles").get_to(w.tiles);

  // Load entities from vector format into the map
  std::vector<Entity> entityVec;
  j.at("entities").get_to(entityVec);
  w.entities.clear();
  int maxID = 0;
  for (auto &e : entityVec) {
    w.entities[e.id] = e;
    if (e.id >= maxID) maxID = e.id + 1;
  }

  if (j.contains("nextEntityID")) {
    j.at("nextEntityID").get_to(w.nextEntityID);
  } else {
    w.nextEntityID = maxID; // Backward compatibility
  }

  j.at("simulation").get_to(w.simulation);
}

