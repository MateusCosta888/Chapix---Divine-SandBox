#include "World.h"
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

  // Initial Calculation of Autotiling Transitions
  UpdateTileTransitions();
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
  UpdateEntities(GetFrameTime()); // Update entities here
}

void World::SimulateWater(float deltaTime) {
  // Water Flow (Cellular Automata) - Simple "Falling" logic
  // Iterate from bottom up to avoid cascading in one frame (optional)
  // Or just top down. Let's do random update or simple scan.
  // For simplicity: Scan all, build next state buffer?
  // Doing in-place for chaotic flow (Minecraft style)

  // Note: We are already iterating logic in main via Update().
  // Let's implement entity update here as well.

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
void World::UpdateTileTransition(int x, int y) {
  if (x < 0 || x >= width || y < 0 || y >= height)
    return;

  Tile &t = tiles[y * width + x];
  uint8_t mask = 0;

  // Check neighbors: N, E, S, W
  // If neighbor is different type (or out of bounds), set bit.
  // Bit 0: North
  // Bit 1: East
  // Bit 2: South
  // Bit 3: West

  auto check = [&](int dx, int dy, int bit) {
    int nx = x + dx;
    int ny = y + dy;

    if (nx < 0 || nx >= width || ny < 0 || ny >= height) {
      // Out of bounds counts as "different" (border)
      mask |= (1 << bit);
      return;
    }

    const Tile &neighbor = tiles[ny * width + nx];
    // Simple logic: different type = border
    // Refinement: DeepOcean and Ocean shouldn't draw borders between them?
    // For now, strict type equality.
    if (neighbor.type != t.type) {
      mask |= (1 << bit);
    }
  };

  check(0, -1, 0); // North
  check(1, 0, 1);  // East
  check(0, 1, 2);  // South
  check(-1, 0, 3); // West

  t.transitionMask = mask;
  t.transitionIndex = mask; // Direct mapping for now
}

void World::UpdateTileTransitions() {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      UpdateTileTransition(x, y);
    }
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
}

void World::SetTileType(int x, int y, TileType newType) {
  if (x < 0 || x >= width || y < 0 || y >= height)
    return;

  Tile &tile = GetTile(x, y);
  TraceLog(LOG_INFO, "WORLD: SetTileType %d,%d to Type %d", x, y, (int)newType);
  tile.type = newType;

  // Update autotiling for this tile and neighbors
  UpdateTileTransition(x, y);
  UpdateTileTransition(x, y - 1); // N
  UpdateTileTransition(x + 1, y); // E
  UpdateTileTransition(x, y + 1); // S
  UpdateTileTransition(x - 1, y); // W
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
  if (type == EntityType::Human) {
    return resourceManager.texHuman[0]; // First frame
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

void World::AddEntity(EntityType type, Vector2 pos) {
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

  // Set speed based on creature type
  if (type == EntityType::Human) {
    e.speed = 2.0f;
  } else if (type == EntityType::Cow || type == EntityType::Bull) {
    e.speed = 0.5f;
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

  entities.push_back(e);
  TraceLog(LOG_INFO, "WORLD: Added Entity Type %d at %.2f, %.2f", (int)type,
           pos.x, pos.y);
}

void World::UpdateEntities(float deltaTime) {
  for (auto &e : entities) {
    // Basic AI
    if (e.type == EntityType::Human) {
      if (e.hasTarget) {
        // Move towards target
        Vector2 dir = Vector2Subtract(e.targetPos, e.position);
        float dist = Vector2Length(dir);

        if (dist < 0.1f) {
          e.hasTarget = false;
          e.state = EntityState::Idle;
          e.currentFrame = 0; // Reset to standing frame
        } else {
          e.state = EntityState::Walking;
          float speed = 2.0f; // tiles per second
          Vector2 move = Vector2Scale(Vector2Normalize(dir), speed * deltaTime);
          e.position = Vector2Add(e.position, move);

          // Update Direction
          if (fabs(dir.x) > fabs(dir.y)) {
            // Horizontal preference
            e.facingDirection = (dir.x > 0) ? 1 : -1; // 1=Right, -1=Left
          } else {
            // Vertical preference
            e.facingDirection =
                (dir.y > 0) ? 0
                            : 2; // 0=Down, 2=Up (Mapped to Row 3 in Renderer)
          }
        }
      } else {
        // Idle Behavior
        e.state = EntityState::Idle;
        // Chance to pick new target
        // Increased to 5% for more activity
        if (rng_.Int(0, 100) < 5) {
          float range = 5.0f;
          float tx = e.position.x;
          float ty = e.position.y;

          int moveDir = rng_.Int(0, 4); // 0:Up, 1:Down, 2:Left, 3:Right
          float dist = 2.0f + (rng_.Float() * 3.0f); // Move 2-5 tiles

          if (moveDir == 0)
            ty -= dist; // Up
          else if (moveDir == 1)
            ty += dist; // Down
          else if (moveDir == 2)
            tx -= dist; // Left
          else if (moveDir == 3)
            tx += dist; // Right

          // Clamp
          tx = std::max(0.0f, std::min((float)width - 1, tx));
          ty = std::max(0.0f, std::min((float)height - 1, ty));
          e.targetPos = {tx, ty};
          e.hasTarget = true;
          TraceLog(LOG_INFO,
                   "AI: Entity Human picked CARDINAL target %.2f, %.2f", tx,
                   ty);
        }
      }

      // Animation
      if (e.state == EntityState::Walking) {
        e.animTime += deltaTime;
        if (e.animTime >= 0.15f) {
          e.animTime = 0.0f;
          e.currentFrame = (e.currentFrame + 1) % 4;
        }
      } else {
        e.currentFrame = 0; // Idle frame (col 0)
      }
    }

    // ========================================================================
    // ANIMAL AI - Simple wander behavior
    // ========================================================================
    else if (e.type == EntityType::Cow || e.type == EntityType::Chicken ||
             e.type == EntityType::Sheep || e.type == EntityType::Bull ||
             e.type == EntityType::Chicken2 || e.type == EntityType::Lamb ||
             e.type == EntityType::Pig || e.type == EntityType::Turkey) {
      if (e.hasTarget) {
        // Move towards target
        Vector2 dir = Vector2Subtract(e.targetPos, e.position);
        float dist = Vector2Length(dir);

        if (dist < 0.1f) {
          e.hasTarget = false;
          e.state = EntityState::Idle;
          e.currentFrame = 0;
        } else {
          e.state = EntityState::Walking;
          float speed = e.speed;
          Vector2 move = Vector2Scale(Vector2Normalize(dir), speed * deltaTime);
          e.position = Vector2Add(e.position, move);

          // Update Direction
          if (fabs(dir.x) > fabs(dir.y)) {
            e.facingDirection = (dir.x > 0) ? 1 : -1;
          } else {
            e.facingDirection = (dir.y > 0) ? 0 : 2;
          }
        }
      } else {
        // Idle - chance to pick new target (less frequent than humans)
        e.state = EntityState::Idle;
        if (rng_.Int(0, 100) < 2) { // 2% chance per frame
          float tx = e.position.x;
          float ty = e.position.y;

          int moveDir = rng_.Int(0, 4);
          float moveDist = 1.0f + (rng_.Float() * 2.0f); // 1-3 tiles

          if (moveDir == 0)
            ty -= moveDist;
          else if (moveDir == 1)
            ty += moveDist;
          else if (moveDir == 2)
            tx -= moveDist;
          else if (moveDir == 3)
            tx += moveDist;

          // Clamp to world bounds
          tx = std::max(0.0f, std::min((float)width - 1, tx));
          ty = std::max(0.0f, std::min((float)height - 1, ty));

          // Only set target if it's walkable (grass/forest)
          int tileX = (int)tx;
          int tileY = (int)ty;
          if (IsWalkable(tileX, tileY)) {
            e.targetPos = {tx, ty};
            e.hasTarget = true;
          }
        }
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
