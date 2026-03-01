#include "World.h"
#include "../core/TimeManager.h"
#include "../utils/Noise.h"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>

World::World(int width, int height, uint32_t seed)
    : width(width), height(height), seed_(seed), rng_(seed) {
  tiles.resize(width * height);
}

World::~World() { UnloadTextures(); }

void World::Reset(uint32_t seed) {
  seed_ = seed;
  rng_ = Random(seed);
}

void World::ResizeAndGenerate(int newWidth, int newHeight) {
  width = newWidth;
  height = newHeight;
  tiles.clear();
  tiles.resize(width * height);
  // Clear entities on new generation
  entities.clear();
  Generate();
}

void World::LoadTextures() { resourceManager.Load(); }
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
  if (t.type == TileType::DeepOcean || t.type == TileType::Ocean ||
      t.type == TileType::ShallowOcean) {
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
      t.decoration == DecorationType::Bush) {
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

void World::Generate() {
  // Clear all simulation data (citizens, cities, kingdoms, buildings)
  // This prevents crashes when generating a new world after playing
  simulation.Reset();

  // Create noise generators with different seeds for each layer
  Noise heightNoise(seed_);
  Noise tempNoise(seed_ + 1000);     // Offset seed for variation
  Noise humidityNoise(seed_ + 2000); // Different offset
  Noise mountainNoise(seed_ + 3000); // New independent noise for mountains

  int padding = 10; // Safe zone padding

  for (int y = 0; y < height; y++) {
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

      // 6. Independent Mountain Generation (Constrained Regions)
      // Pick 3 Mountain Centers randomly based on seed
      // We do this procedurally per pixel, but that's expensive/tricky for
      // "global" centers. Better to define them statically based on seed at the
      // start of Generate? Since we are in the loop, we can just hash the seed
      // to get 3 fixed points (relative to map size).

      // Pseudo-random centers based on world seed
      float mCenters[3][2];
      float mRadii[3];

      std::mt19937 mRng(seed_ ^ 9999);
      std::uniform_real_distribution<float> distX(0, (float)width);
      std::uniform_real_distribution<float> distY(0, (float)height);

      // 1 Major Region
      mCenters[0][0] = distX(mRng);
      mCenters[0][1] = distY(mRng);
      mRadii[0] =
          std::min(width, height) * 0.35f; // Large radius (35% of map size)

      // 2 Minor Regions
      mCenters[1][0] = distX(mRng);
      mCenters[1][1] = distY(mRng);
      mRadii[1] = std::min(width, height) * 0.15f;

      mCenters[2][0] = distX(mRng);
      mCenters[2][1] = distY(mRng);
      mRadii[2] = std::min(width, height) * 0.15f;

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
  for (int y = 0; y < height; y++) {
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
        if ((seed % 100) < 20) { // Reduced from 30% to 20%
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
        if ((seed % 100) < 15) { // 15% Snow
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

  int cowCount = 0, chickenCount = 0, sheepCount = 0;
  int maxCows = 8, maxChickens = 12, maxSheep = 10;

  for (int y = 20; y < height - 20; y += 3) { // Skip border areas, step 3
    for (int x = 20; x < width - 20; x += 3) {
      Tile &tile = GetTile(x, y);

      // Only spawn on grass/forest
      if (tile.type != TileType::Grass && tile.type != TileType::Forest)
        continue;

      // Use tile variant for deterministic random
      unsigned int spawnRoll = (tile.variant ^ seed_ ^ 0xDEADBEEF) % 100;

      // Higher spawn rates: 5% cow, 7% chicken, 6% sheep
      if (spawnRoll < 5 && cowCount < maxCows) {
        Entity cow;
        cow.id = entities.size();
        cow.type = EntityType::Cow;
        cow.position = {(float)x + 0.5f, (float)y + 0.5f};
        cow.state = EntityState::Idle;
        cow.facingDirection = rng_.Int(-1, 2); // -1, 0, 1, 2
        cow.speed = 0.5f;
        cow.health = 100.0f;
        cow.animTime = 0.0f;
        cow.currentFrame = 0;
        cow.hasTarget = false;
        entities.push_back(cow);
        cowCount++;
      } else if (spawnRoll >= 5 && spawnRoll < 12 &&
                 chickenCount < maxChickens) {
        Entity chicken;
        chicken.id = entities.size();
        chicken.type = EntityType::Chicken;
        chicken.position = {(float)x + 0.5f, (float)y + 0.5f};
        chicken.state = EntityState::Idle;
        chicken.facingDirection = rng_.Int(-1, 2);
        chicken.speed = 0.8f;
        chicken.health = 20.0f;
        chicken.animTime = 0.0f;
        chicken.currentFrame = 0;
        chicken.hasTarget = false;
        entities.push_back(chicken);
        chickenCount++;
      } else if (spawnRoll >= 12 && spawnRoll < 20 && sheepCount < maxSheep) {
        Entity sheep;
        sheep.id = entities.size();
        sheep.type = EntityType::Sheep;
        sheep.position = {(float)x + 0.5f, (float)y + 0.5f};
        sheep.state = EntityState::Idle;
        sheep.facingDirection = rng_.Int(-1, 2);
        sheep.speed = 0.6f;
        sheep.health = 50.0f;
        sheep.animTime = 0.0f;
        sheep.currentFrame = 0;
        sheep.hasTarget = false;
        entities.push_back(sheep);
        sheepCount++;
      }
    }
  }

  TraceLog(LOG_INFO, "ANIMALS: Spawned %d cows, %d chickens, %d sheep",
           cowCount, chickenCount, sheepCount);
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
  TraceLog(LOG_INFO, "WORLD: SetTileType %d,%d to Type %d", x, y, (int)newType);
  tile.type = newType;

  // Sync liquidLevel with water tile types
  if (newType == TileType::DeepOcean) {
    tile.liquidLevel = 1.0f;
  } else if (newType == TileType::Ocean) {
    tile.liquidLevel = 0.5f;
  } else if (newType == TileType::ShallowOcean) {
    tile.liquidLevel = 0.2f;
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
    for (auto &e : entities) {
      if ((int)e.position.x == x && (int)e.position.y == y) {
        e.health = 0;
        e.state = EntityState::Die;
      }
    }
  }
}

void World::SetTileDecoration(int x, int y, DecorationType type) {
  if (x < 0 || x >= width || y < 0 || y >= height)
    return;

  // Prevent placing decorations on water, but allow removing them
  if (IsSwimmable(x, y) && type != DecorationType::None) {
    return;
  }

  Tile &tile = GetTile(x, y);
  tile.decoration = type;
  tile.decorationVariant = rng_.Int(0, 3);
}

Texture2D World::GetTextureForUI(TileType type) {
  return resourceManager.GetTextureForUI(type);
}

Texture2D World::GetTextureForUI(DecorationType type) {
  return resourceManager.GetTextureForUI(type);
}

Texture2D World::GetTextureForUI(EntityType type) {
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

void World::AddEntity(EntityType type, Vector2 pos) {
  // 50/50 chance: player-placed unarmed humans may be women
  if (type == EntityType::HumanUnarmed && (rand() % 2 == 0)) {
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
  e.id = entities.size();
  e.type = type;
  e.position = pos;
  e.targetPos = pos;
  e.state = EntityState::Idle;
  e.health = 100.0f;
  e.currentFrame = 0;
  e.animTime = 0.0f;
  e.facingDirection = 0; // Down
  e.hasTarget = false;
  e.citizenID = -1; // Initialize as non-citizen

  // Set speed based on creature type
  // Set speed and health based on creature type
  if (type == EntityType::HumanUnarmed || type == EntityType::HumanWoman) {
    e.speed = 2.0f;
    e.health = 15.0f;
  } else if (type == EntityType::HumanArmed) {
    e.speed = 2.0f;
    e.health = 20.0f;
  } else if (type == EntityType::Boar) {
    e.speed = 2.5f; // Faster
    e.health = 15.0f;
  } else if (type == EntityType::Cow || type == EntityType::Bull) {
    e.speed = 0.5f;
    e.health = 10.0f;
  } else if (type == EntityType::Chicken || type == EntityType::Chicken2 ||
             type == EntityType::Turkey) {
    e.speed = 0.8f;
  } else if (type == EntityType::Sheep || type == EntityType::Lamb) {
    e.speed = 0.6f;
  } else if (type == EntityType::Pig) {
    e.speed = 0.55f;
  } else {
    e.speed = 1.0f;
  }

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

  entities.push_back(e);
  TraceLog(LOG_INFO, "WORLD: Added Entity Type %d at %.2f, %.2f", (int)type,
           pos.x, pos.y);
}

void World::RebuildEntityCache() {
  citizenEntityMap.clear();
  // We reserve space to minimize rehashing
  citizenEntityMap.reserve(entities.size() /
                           2); // Roughly half entities are citizens
  for (auto &e : entities) {
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
  // Always rebuild the pointer cache at the start of the frame.
  // This guarantees pointers are valid even if entities vector reallocated in
  // the previous frame. Rebuilding a hash map of 1000 items is much faster than
  // doing massive O(N) searches everywhere.
  RebuildEntityCache();

  for (size_t i = 0; i < entities.size(); i++) {
    Entity &e = entities[i];

    // === UNIVERSAL DEATH HANDLER ===
    // Handles death for ALL entity types (animals, humans, etc.)
    if (e.health <= 0 || e.state == EntityState::Die) {
      e.state = EntityState::Die;
      e.health = 0;
      e.animTime += deltaTime;
      if (e.animTime >= 0.2f) {
        e.animTime = 0.0f;
        e.currentFrame++;
      }
      if (e.currentFrame >= 4) {
        // Remove citizen from simulation if this was an intelligent entity
        if (e.IsIntelligent() && e.citizenID != -1) {
          simulation.RemoveCitizen(e.citizenID);
        }
        entities.erase(entities.begin() + i--);
      }
      continue;
    }

    // Basic AI
    // === BOAR AI ===
    if (e.type == EntityType::Boar) {
      if (e.state == EntityState::Die) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.15f) {
          e.animTime = 0.0f;
          e.currentFrame++;
        }
        if (e.currentFrame >= 6)
          entities.erase(entities.begin() + i--);
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
            for (auto &target : entities) {
              if ((target.type == EntityType::HumanArmed ||
                   target.type == EntityType::HumanUnarmed ||
                   target.type == EntityType::HumanWoman) &&
                  target.health > 0) {
                if (Vector2Distance(e.position, target.position) < 1.5f) {
                  float dmg = 5.0f;
                  if (target.type == EntityType::HumanArmed &&
                      rng_.Int(0, 100) < 20) {
                    dmg *= 0.5f;
                    target.state = EntityState::Block;
                    target.currentFrame = 0;
                    TraceLog(LOG_INFO, "COMBAT: Human BLOCKED Boar!");
                  }
                  target.health -= dmg;
                  if (target.health <= 0)
                    target.state = EntityState::Die;
                }
              }
            }
          }
          if (e.currentFrame >= 5)
            e.state = EntityState::Idle;
        }
        continue;
      }

      // Hunt Humans
      bool targetFound = false;
      Vector2 targetPos = e.position;
      for (const auto &target : entities) {
        if ((target.type == EntityType::HumanArmed ||
             target.type == EntityType::HumanUnarmed ||
             target.type == EntityType::HumanWoman) &&
            target.health > 0) {
          if (Vector2Distance(e.position, target.position) < 6.0f) {
            targetPos = target.position;
            targetFound = true;
            break;
          }
        }
      }

      if (targetFound) {
        e.state = EntityState::Run;
        if (Vector2Distance(e.position, targetPos) < 1.0f) {
          e.state = EntityState::Attack;
          e.currentFrame = 0;
        } else {
          Vector2 dir =
              Vector2Normalize(Vector2Subtract(targetPos, e.position));
          e.position = Vector2Add(
              e.position, Vector2Scale(dir, e.speed * 1.5f * deltaTime));
          // Facing
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
          if (IsWalkable((int)next.x, (int)next.y, true))
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

    // === HUMAN AI ===
    else if (e.type == EntityType::HumanUnarmed ||
             e.type == EntityType::HumanArmed ||
             e.type == EntityType::HumanWoman) {
      // Death
      if (e.state == EntityState::Die) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.2f) {
          e.animTime = 0.0f;
          e.currentFrame++;
        }
        if (e.currentFrame >= 4)
          entities.erase(entities.begin() + i--);
        continue;
      }
      // Block
      if (e.state == EntityState::Block) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.3f)
          e.state = EntityState::Idle;
        continue;
      }

      // Attack
      if (e.state == EntityState::Attack) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.30f) { // Slow attack
          e.animTime = 0.0f;
          e.currentFrame++;
          if (e.currentFrame == 2) {
            for (auto &target : entities) {
              if (target.type == EntityType::Boar && target.health > 0) {
                float dist = Vector2Distance(e.position, target.position);
                if (dist < 1.5f) {
                  Vector2 dirTo = Vector2Normalize(
                      Vector2Subtract(target.position, e.position));
                  bool facing = false;
                  if (e.facingDirection == 0 && dirTo.y > 0.5f)
                    facing = true;
                  else if (e.facingDirection == 1 && dirTo.x > 0.5f)
                    facing = true;
                  else if (e.facingDirection == -1 && dirTo.x < -0.5f)
                    facing = true;
                  else if (e.facingDirection == 2 && dirTo.y < -0.5f)
                    facing = true;

                  if (facing) {
                    float dmg =
                        (e.type == EntityType::HumanArmed) ? 5.0f : 2.0f;
                    target.health -= dmg;
                    target.state = EntityState::Hurt;
                    target.currentFrame = 0;
                    if (target.health <= 0)
                      target.state = EntityState::Die;
                    TraceLog(LOG_INFO, "COMBAT: Human Hit Boar!");
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

      // Hunt Boars
      bool foundTarget = false;
      Vector2 targetPos = e.position;
      float minDist = 10.0f;
      for (const auto &target : entities) {
        if (target.type == EntityType::Boar && target.health > 0) {
          float d = Vector2Distance(e.position, target.position);
          if (d < minDist) {
            minDist = d;
            targetPos = target.position;
            foundTarget = true;
          }
        }
      }

      if (foundTarget) {
        if (minDist < 1.0f) {
          e.state = EntityState::Attack;
          e.currentFrame = 0;
          e.animTime = 0.0f;
          e.hasTarget = false;

          // Still allow facing update
          Vector2 dir = Vector2Subtract(targetPos, e.position);
          if (fabs(dir.x) > fabs(dir.y))
            e.facingDirection = (dir.x > 0) ? 1 : -1;
          else
            e.facingDirection = (dir.y > 0) ? 0 : 2;

        } else {
          // Move logic
          e.state = EntityState::Walking;
          Vector2 dir = Vector2Subtract(targetPos, e.position);

          // Force cardinal direction (no diagonal) - REMOVED
          // if (fabs(dir.x) > fabs(dir.y)) {
          //   dir.y = 0;
          //   e.facingDirection = (dir.x > 0) ? 1 : -1;
          // } else {
          //   dir.x = 0;
          //   e.facingDirection = (dir.y > 0) ? 0 : 2;
          // }

          // Use dominance only for facing direction
          if (fabs(dir.x) > fabs(dir.y)) {
            e.facingDirection = (dir.x > 0) ? 1 : -1;
          } else {
            e.facingDirection = (dir.y > 0) ? 0 : 2;
          }

          dir = Vector2Normalize(dir);
          // Stop if hit wall
          e.state = EntityState::Idle;
        }

      } else {
        // Wander - but stay within city territory if assigned to one
        // SKIP wander if citizen is actively working a job (e.g., lumberjack
        // going to tree)
        bool skipWander = false;
        if (e.citizenID >= 0) {
          Citizen *citizen = simulation.GetCitizen(e.citizenID);
          if (citizen && citizen->isWorking) {
            skipWander = true; // Don't override job target with wander
          }
        }

        bool isIdle = (e.state == EntityState::Idle);
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

        if (e.hasTarget && !foundTarget) { // Wander target
          Vector2 dir = Vector2Subtract(e.targetPos, e.position);
          if (Vector2Length(dir) < 0.1f) {
            e.hasTarget = false;
            e.state = EntityState::Idle;
          } else {
            // Keep running if marching/attacking, else just walk
            if (e.state != EntityState::Run) {
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
            float actualSpeed =
                (e.state == EntityState::Run) ? (e.speed * 2.0f) : e.speed;
            Vector2 nextPos = Vector2Add(
                e.position,
                Vector2Scale(dir,
                             actualSpeed *
                                 deltaTime)); // Use e.speed instead of 1.0f

            // Collision Check with Sliding
            bool moved = false;
            // 1. Try full diagonal move
            if (IsWalkable((int)nextPos.x, (int)nextPos.y)) {
              e.position = nextPos;
              moved = true;
            } else {
              // 2. Try sliding along X axis
              if (IsWalkable((int)nextPos.x, (int)e.position.y)) {
                e.position.x = nextPos.x;
                moved = true;
              }
              // 3. Try sliding along Y axis
              else if (IsWalkable((int)e.position.x, (int)nextPos.y)) {
                e.position.y = nextPos.y;
                moved = true;
              }
            }

            // 4. Stuck (Hit a perfect corner or surrounded)
            if (!moved) {
              e.hasTarget = false;
              e.state = EntityState::Idle;
            }
          }
        } else if (!foundTarget && (e.state == EntityState::Walking ||
                                    e.state == EntityState::Run)) {
          // Only reset to Idle if we were Walking or Running but lost target.
          // If we were Attacking (Working), don't reset!
          e.state = EntityState::Idle;
        }
      }

      // Anim
      if (e.state == EntityState::Walking || e.state == EntityState::Run) {
        float animSpeed = (e.state == EntityState::Run) ? 0.05f : 0.1f;
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

      if (!e.IsIntelligent() && e.type != EntityType::Boar) {
        static int logTimer = 0;
        logTimer++;
        if (logTimer % 60 == 0) {
          TraceLog(
              LOG_INFO,
              "ANIMAL DEBUG: speed: %f, dir: %f, %f. pos.x: %f, next.x: %f "
              "walkable: %d",
              e.speed, dir.x, dir.y, e.position.x, next.x,
              IsWalkable((int)next.x, (int)next.y, true));
        }
      }

      if (IsWalkable((int)next.x, (int)next.y, true)) {
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
}

// ============================================================================
// SPAWN EFFECT SYSTEM
// ============================================================================

void World::AddSpawnEffect(int tileX, int tileY, Color color) {
  // Rate limit to avoid too many effects
  if (spawnEffects.size() > 100)
    return;

  SpawnEffect fx;
  fx.worldX = tileX * 10.0f + 5.0f;
  fx.worldY = tileY * 10.0f + 5.0f;
  fx.timer = 0.0f;
  fx.duration = 0.6f;
  fx.scale = 0.0f;

  // Create 6-10 sparkle particles
  int numParticles = 6 + (rand() % 5);
  for (int i = 0; i < numParticles; i++) {
    SpawnParticle p;
    float angle = (float)(rand() % 360) * DEG2RAD;
    float dist = (float)(rand() % 40) / 10.0f;
    p.pos = {fx.worldX + cosf(angle) * dist, fx.worldY + sinf(angle) * dist};

    float speed = 8.0f + (float)(rand() % 20);
    p.velocity = {cosf(angle) * speed, -5.0f - (float)(rand() % 15)};

    p.maxLife = 0.3f + (float)(rand() % 50) / 100.0f;
    p.lifetime = p.maxLife;

    p.color = color;
    p.color.r = (unsigned char)std::min(255, (int)color.r + (rand() % 60 - 30));
    p.color.g = (unsigned char)std::min(255, (int)color.g + (rand() % 60 - 30));
    p.color.b = (unsigned char)std::min(255, (int)color.b + (rand() % 60 - 30));
    p.color.a = 255;

    p.size = 1.0f + (float)(rand() % 20) / 10.0f;

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

void World::DrawSpawnEffects() const {
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
    if (fx.timer < 0.3f) {
      float ringAlpha = 1.0f - (fx.timer / 0.3f);
      float ringRadius = 3.0f + fx.timer * 25.0f;
      Color ringColor = {255, 255, 200, (unsigned char)(ringAlpha * 120.0f)};
      DrawCircleLines((int)fx.worldX, (int)fx.worldY, ringRadius, ringColor);
    }
  }
}

void to_json(nlohmann::json &j, const World &w) {
  j = nlohmann::json{{"width", w.width},       {"height", w.height},
                     {"seed", w.seed_},        {"tiles", w.tiles},
                     {"entities", w.entities}, {"simulation", w.simulation}};
}

void from_json(const nlohmann::json &j, World &w) {
  j.at("width").get_to(w.width);
  j.at("height").get_to(w.height);
  j.at("seed").get_to(w.seed_);
  j.at("tiles").get_to(w.tiles);
  j.at("entities").get_to(w.entities);
  j.at("simulation").get_to(w.simulation);
}
