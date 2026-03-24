#pragma once
#include "../resources/ResourceManager.h"    // Include ResourceManager
#include "../simulation/SimulationManager.h" // Civilization simulation
#include "../utils/JsonHelpers.h"
#include "../utils/Random.h"
#include "Entity.h" // Added
#include "Tile.h"
#include "raylib.h"
#include <cstdint>
#include <functional> // Added
#include <unordered_map>
#include <vector>

// === SPAWN EFFECT SYSTEM ===
enum class VfxType { Default, Lightning, Fire, Tornado, FireBomb, DarkBolt, Thunder2 };

struct SpawnParticle {
  Vector2 pos;      // World position (pixels)
  Vector2 velocity; // Movement direction
  float lifetime;   // Remaining life
  float maxLife;    // Initial life (for alpha calc)
  Color color;
  float size;
};

struct SpawnEffect {
  float worldX, worldY; // Tile center in world pixels
  float timer;          // Time since spawn
  float duration;       // Total effect duration
  float scale;          // Current scale (0->1 for pop-in)
  VfxType type = VfxType::Default;
  int currentFrame = 0;
  std::vector<SpawnParticle> particles;
};

class World {
public:
  // Constructor now accepts optional seed (default random)
  World(int width, int height, uint32_t seed = 0);
  ~World(); // Destructor to unload textures

  void Generate(std::function<void(const char *)> loadingCallback = nullptr);
  void ResizeAndGenerate(
      int newWidth, int newHeight,
      std::function<void(const char *)> loadingCallback = nullptr);
  void Reset(uint32_t seed);
  void Update();                 // For future simulation steps
  void UpdateEntities(float dt); // Added
  void UpdateWorldEvents(float dt);

  // Entities
  void AddEntity(EntityType type, Vector2 pos, bool skipGenderRandom = false);
  const std::vector<Entity> &GetEntities() const { return entities; }
  std::vector<Entity> &GetEntitiesMutable() {
    return entities;
  } // For simulation

  // O(1) Lookup cache
  Entity *GetEntityByCitizenID(int citizenID);
  const Entity *GetEntityByCitizenID(int citizenID) const;
  void RebuildEntityCache();

  void LoadTextures(std::function<void(const char *)> loadingCallback =
                        nullptr);        // Load tile textures
  void UnloadTextures();                 // Cleanup textures
  void UpdateAnimation(float deltaTime); // Update GIF animation

  Tile &GetTile(int x, int y);
  const Tile &GetTileConst(int x, int y) const;

  // Helper to resolve kingdom based on tile ownership (city owner -> kingdom)
  int GetKingdomAtTile(int tx, int ty) const;

  int GetWidth() const { return width; }
  int GetHeight() const { return height; }
  uint32_t GetSeed() const { return seed_; }

  // User modification (for future UI)
  void SetTileBiome(int x, int y, BiomeType newBiome); // Interaction
  void SetTileType(int x, int y, TileType newType);
  void SetTileDecoration(int x, int y, DecorationType type);
  void ClearTileContents(int x,
                         int y); // Remove all entities/buildings/decorations

  // God Powers
  void TriggerGodPower(int powerIndex, int tx, int ty);

  // Updates edge mask for a specific tile/neighbors
  void UpdateTileEdgeMask(int x, int y);
  void UpdateNeighborsEdgeMask(int x, int y);

  // UI Helpers
  Texture2D GetTextureForUI(TileType type) const;
  Texture2D GetTextureForUI(DecorationType type) const;
  Texture2D GetTextureForUI(EntityType type) const; // Added

  // Collision & Physics
  bool IsWalkable(int x, int y, bool ignoreBuildings = false) const;
  bool IsSwimmable(int x, int y) const;
  float GetHeight(int x, int y) const;

  void SimulateWater(float deltaTime);

  // Access to ResourceManager for WorldRenderer
  ResourceManager &GetResourceManager() { return resourceManager; }
  const ResourceManager &GetResourceManager() const { return resourceManager; }

  // Access to SimulationManager for civilization system
  SimulationManager &GetSimulation() { return simulation; }
  const SimulationManager &GetSimulation() const { return simulation; }

  // === SPAWN EFFECTS ===
  void AddSpawnEffect(int tileX, int tileY, Color color,
                      VfxType type = VfxType::Default);
  void UpdateSpawnEffects(float dt);
  void DrawSpawnEffects(const class ResourceManager &resourceManager) const;
  const std::vector<SpawnEffect> &GetSpawnEffects() const {
    return spawnEffects;
  }

  // JSON serialization
  friend void to_json(nlohmann::json &j, const World &w);
  friend void from_json(const nlohmann::json &j, World &w);

private:
  int width;
  int height;
  uint32_t seed_;
  Random rng_;
  std::vector<Tile> tiles;
  int nextEntityID = 0;         // Globally unique ID for all new entities
  std::vector<Entity> entities; // Added entities vector
  std::unordered_map<int, Entity *>
      citizenEntityMap; // O(1) cache for citizen lookup

  // Spawn Effects
  std::vector<SpawnEffect> spawnEffects;

  // Resource Manager instance
  ResourceManager resourceManager;

  // Simulation Manager for civilizations
  SimulationManager simulation;

  // Helper to get texture for autotiling
  Texture2D *GetTextureForTile(TileType type);
};
