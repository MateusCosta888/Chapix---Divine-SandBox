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
  // Water takes priority regardless of biome (3 depths)
  if (height < 0.15f)
    return TileType::DeepOcean; // Deepest
  if (height < 0.28f)
    return TileType::Ocean; // Medium depth
  if (height < 0.38f)
    return TileType::ShallowOcean; // Coastal/shallow

  switch (biome) {
  case BiomeType::Desert:
    if (height < 0.65f)
      return TileType::DesertSand; // Mostly desert sand
    return TileType::Mountain;     // Only rare rocky outcrops

  case BiomeType::Snow:
    if (height < 0.55f)
      return TileType::Snow; // Snow covered ground
    if (height < 0.75f)
      return TileType::Mountain; // Snowy mountains
    return TileType::Snow;       // High altitude snow peaks

  case BiomeType::Plains:
    if (height < 0.40f)
      return TileType::Sand; // Beach
    if (height < 0.60f)
      return TileType::Grass;
    return TileType::Mountain;

  case BiomeType::Forest:
    if (height < 0.40f)
      return TileType::Sand; // Beach
    if (height < 0.55f)
      return TileType::Forest;
    return TileType::Mountain;

  case BiomeType::Mountain:
    if (height < 0.42f)
      return TileType::Grass; // Foothills
    if (height < 0.65f)
      return TileType::Mountain;
    return TileType::Snow; // Snow peaks

  default:
    return TileType::Grass;
  }
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

void World::Update() {}

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
  tile.type = newType;
}

void World::SetTileDecoration(int x, int y, DecorationType type) {
  if (x < 0 || x >= width || y < 0 || y >= height)
    return;
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
