#include "World.h"
#include "../utils/Noise.h"
#include "raylib.h"
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

void World::LoadTextures() { resourceManager.Load(); }

void World::UnloadTextures() { resourceManager.Unload(); }

void World::UpdateAnimation(float deltaTime) {
  // No animation currently - static textures
  (void)deltaTime;
}

// Helper: Determine biome from temperature, humidity
// Latitude influence is now baked into the temperature map in Generate()
BiomeType DetermineBiome(float temp, float humidity, float latitude) {
  // Snow biome: Cold regions
  if (temp < 0.30f) {
    return BiomeType::Snow;
  }

  // Desert biome: Hot and Dry
  if (temp > 0.65f && humidity < 0.45f) {
    return BiomeType::Desert;
  }

  // Mountain: very cold (handled mostly by height, but also temp)
  if (temp < 0.20f) {
    return BiomeType::Mountain;
  }

  // Forest: Temperate and Humid
  if (humidity > 0.50f) {
    return BiomeType::Forest;
  }

  // Plains: Default temperate
  return BiomeType::Plains;
}

// Helper: Get terrain type based on height and biome
TileType GetTerrainForBiome(BiomeType biome, float height) {
  // STRICT LAYER SYSTEM
  // 1. Deep Water
  if (height < 0.20f)
    return TileType::DeepOcean;
  // 2. Medium Water
  if (height < 0.30f)
    return TileType::Ocean;
  // 3. Shallow Water
  if (height < 0.40f)
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
    if (height < 0.45f) {
      return TileType::Sand; // Beach just above water
    }
    if (biome == BiomeType::Forest) {
      return TileType::Forest;
    }
    return TileType::Grass; // Default land
  }

  // 5. Mountain Layer (Highest)
  // This is the blocking layer
  return TileType::Mountain;
}

bool World::IsWalkable(int x, int y) const {
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
  // Create noise generators with different seeds for each layer
  Noise heightNoise(seed_);
  Noise tempNoise(seed_ + 1000);     // Offset seed for variation
  Noise humidityNoise(seed_ + 2000); // Different offset

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
      } else {
        tile.biome = DetermineBiome(
            temp, humid, ny); // ny is ignored inside but kept for signature
                              // compatibility if not changed
      }

      // 5. Get terrain type based on biome and height
      tile.type = GetTerrainForBiome(tile.biome, h);

      // Pre-calculate visual variants
      tile.variant = x * 73856093 ^ y * 19349663;
      tile.decorationVariant = (tile.variant ^ seed_) % 4;
    }
  }
}

Tile &World::GetTile(int x, int y) {
  if (x < 0 || x >= width || y < 0 || y >= height) {
    static Tile empty;
    return empty;
  }
  return tiles[y * width + x];
}

void World::Update() {
  // Run water physics (fixed time step or every frame? Every frame is fine for
  // now)
  SimulateWater(GetFrameTime());
}

void World::SimulateWater(float deltaTime) {
  // Simple cellular automata for water flow
  // Water above sea level (0.40) tries to flow downhill

  // Create a copy or just partial updates?
  // Partial updates might cause cascading in one frame (teleporting water),
  // but for simple visual "sliding" it might be okay.
  // To be safe and prevent infinite flow in one frame, we might limit updates
  // or iterate randomly. Let's iterate forward for now.

  static float timer = 0.0f;
  timer += deltaTime;
  if (timer < 0.1f)
    return; // Update 10 times per second to simulate viscosity
  timer = 0.0f;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      Tile &tile = GetTile(x, y);

      // Only process water that is ABOVE sea level (placed by user or "stuck")
      // Sea level is 0.40f as per GetTerrainForBiome logic
      if (IsSwimmable(x, y) && tile.height >= 0.40f) {

        // Find lowest neighbor
        float currentH = tile.height;
        float lowestH = currentH;
        int targetX = -1;
        int targetY = -1;

        int dx[] = {0, 0, -1, 1};
        int dy[] = {-1, 1, 0, 0};

        for (int i = 0; i < 4; i++) {
          int nx = x + dx[i];
          int ny = y + dy[i];

          if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            const Tile &neighbor = GetTile(nx, ny);
            // Can flow into non-water or water that is lower?
            // Usually flow into Empty (Air), but here we flow into Terrain.
            // We want to flow DOWNHILL.
            if (neighbor.height < lowestH) {
              lowestH = neighbor.height;
              targetX = nx;
              targetY = ny;
            }
          }
        }

        // If found a lower spot
        if (targetX != -1) {
          // Move water
          Tile &target = GetTile(targetX, targetY);

          // If target is already water, it just merges (no height change logic
          // yet) If target is land, it becomes water

          TileType waterType = tile.type; // Carry the type (e.g. DeepOcean)

          // Restore current tile to its natural state (Dry)
          tile.type = GetTerrainForBiome(tile.biome, tile.height);

          // Set target to water
          target.type = waterType;

          // Stop processing this drop for this frame (optional, but good)
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

// Helper: Get texture for tile type (Updated with new sprites)
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
}

void World::SetTileType(int x, int y, TileType newType) {
  if (x < 0 || x >= width || y < 0 || y >= height)
    return;

  Tile &tile = GetTile(x, y);
  TraceLog(LOG_INFO, "WORLD: SetTileType %d,%d to Type %d", x, y, (int)newType);
  tile.type = newType;
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
