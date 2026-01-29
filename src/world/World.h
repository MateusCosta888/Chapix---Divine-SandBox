#pragma once
#include "../resources/ResourceManager.h" // Include ResourceManager
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

  void LoadTextures();                   // Load tile textures
  void UnloadTextures();                 // Cleanup textures
  void UpdateAnimation(float deltaTime); // Update GIF animation

  Tile &GetTile(int x, int y);
  int GetWidth() const { return width; }
  int GetHeight() const { return height; }
  uint32_t GetSeed() const { return seed_; }

  // User modification (for future UI)
  void SetTileBiome(int x, int y, BiomeType newBiome); // Interaction
  void SetTileType(int x, int y, TileType newType);
  void SetTileDecoration(int x, int y, DecorationType type);

  // UI Helpers
  Texture2D GetTextureForUI(TileType type);
  Texture2D GetTextureForUI(DecorationType type);
  Texture2D GetTextureForUI(EntityType type); // Added

  // Collision & Physics
  bool IsWalkable(int x, int y) const;
  bool IsSwimmable(int x, int y) const;
  float GetHeight(int x, int y) const;

  void SimulateWater(float deltaTime);

  // Access to ResourceManager for WorldRenderer
  ResourceManager &GetResourceManager() { return resourceManager; }
  const ResourceManager &GetResourceManager() const { return resourceManager; }

private:
  int width;
  int height;
  uint32_t seed_;
  Random rng_;
  std::vector<Tile> tiles;
  std::vector<Entity> entities; // Added entities vector

  // Resource Manager instance
  ResourceManager resourceManager;

  // Helper to get texture for autotiling
  Texture2D *GetTextureForTile(TileType type);
};
