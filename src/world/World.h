#pragma once
#include "../resources/ResourceManager.h"    // Include ResourceManager
#include "../simulation/SimulationManager.h" // Civilization simulation
#include "../utils/JsonHelpers.h"
#include "../utils/Random.h"
#include "Entity.h" // Added
#include "Tile.h"
#include "raylib.h"
#include <cstdint>
#include <vector>


class World {
public:
  // Constructor now accepts optional seed (default random)
  World(int width, int height, uint32_t seed = 0);
  ~World(); // Destructor to unload textures

  void Generate();
  void Reset(uint32_t seed);
  void Update();                 // For future simulation steps
  void UpdateEntities(float dt); // Added

  // Entities
  void AddEntity(EntityType type, Vector2 pos);
  const std::vector<Entity> &GetEntities() const { return entities; }
  std::vector<Entity> &GetEntitiesMutable() {
    return entities;
  } // For simulation

  void LoadTextures();                   // Load tile textures
  void UnloadTextures();                 // Cleanup textures
  void UpdateAnimation(float deltaTime); // Update GIF animation

  Tile &GetTile(int x, int y);
  const Tile &GetTileConst(int x, int y) const;
  int GetWidth() const { return width; }
  int GetHeight() const { return height; }
  uint32_t GetSeed() const { return seed_; }

  // User modification (for future UI)
  void SetTileBiome(int x, int y, BiomeType newBiome); // Interaction
  void SetTileType(int x, int y, TileType newType);
  void SetTileDecoration(int x, int y, DecorationType type);

  // Updates edge mask for a specific tile/neighbors
  void UpdateTileEdgeMask(int x, int y);
  void UpdateNeighborsEdgeMask(int x, int y);

  // UI Helpers
  Texture2D GetTextureForUI(TileType type);
  Texture2D GetTextureForUI(DecorationType type);
  Texture2D GetTextureForUI(EntityType type); // Added

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

  // JSON serialization
  friend void to_json(nlohmann::json &j, const World &w);
  friend void from_json(const nlohmann::json &j, World &w);

private:
  int width;
  int height;
  uint32_t seed_;
  Random rng_;
  std::vector<Tile> tiles;
  std::vector<Entity> entities; // Added entities vector

  // Resource Manager instance
  ResourceManager resourceManager;

  // Simulation Manager for civilizations
  SimulationManager simulation;

  // Helper to get texture for autotiling
  Texture2D *GetTextureForTile(TileType type);
};
