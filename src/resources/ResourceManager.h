#pragma once
#include "../world/Tile.h"
#include "raylib.h"
#include <vector>


class ResourceManager {
public:
  ResourceManager();
  ~ResourceManager();

  void Load();
  void Unload();
  bool IsLoaded() const { return texturesLoaded; }

  // Constants
  static const int NUM_WATER_VARIANTS = 2;
  static const int NUM_GRASS_VARIANTS = 3;
  static const int NUM_SAND_VARIANTS = 2;
  static const int NUM_MOUNTAIN_VARIANTS = 2;
  static const int NUM_FOREST_VARIANTS = 2;

  // Trees
  static const int NUM_TREE_TYPES = 4;
  static const int NUM_FRUIT_TREE_TYPES = 3;
  static const int NUM_NORMAL_TREE_TYPES = 3;
  static const int NUM_MOSS_TREE_TYPES = 3;
  static const int NUM_SNOW_TREE_TYPES = 3;
  static const int NUM_XMAS_TREE_TYPES = 3;
  static const int NUM_PALM_TREE_TYPES = 6;

  // Bushes & Cactus
  static const int NUM_BUSH_TYPES = 2;
  static const int NUM_SNOW_BUSH_TYPES = 3;
  static const int NUM_CACTUS_TYPES = 6;

  // Decorations
  static const int NUM_CRYSTAL_VARIANTS = 4;
  static const int NUM_BIG_ROCK_TYPES = 5;
  static const int NUM_SNOW_ROCK_SHADOWS = 5;
  static const int NUM_DESERT_ROCK_SHADOWS = 5;
  static const int NUM_FLOWER_TYPES = 4;
  static const int NUM_MUSHROOM_TYPES = 2;
  static const int NUM_SMALL_ROCK_TYPES = 5;
  static const int NUM_MEDIUM_ROCK_TYPES = 5;

  // Textures
  Texture2D texDeepOcean[NUM_WATER_VARIANTS];
  Texture2D texOcean[NUM_WATER_VARIANTS];
  Texture2D texShallowOcean[NUM_WATER_VARIANTS];

  Texture2D texGrass[NUM_GRASS_VARIANTS];
  Texture2D texSand[NUM_SAND_VARIANTS];
  Texture2D texSnow;
  Texture2D texMountain[NUM_MOUNTAIN_VARIANTS];
  Texture2D texMountainRocks;

  Texture2D texForest[NUM_FOREST_VARIANTS];
  Texture2D texGraminhas;

  Texture2D texTrees[NUM_TREE_TYPES];
  Texture2D texFruitTrees[NUM_FRUIT_TREE_TYPES];
  Texture2D texNormalTrees[NUM_NORMAL_TREE_TYPES];
  Texture2D texMossTrees[NUM_MOSS_TREE_TYPES];
  Texture2D texSnowTrees[NUM_SNOW_TREE_TYPES];
  Texture2D texXmasTrees[NUM_XMAS_TREE_TYPES];
  Texture2D texPalmTrees[NUM_PALM_TREE_TYPES];

  Texture2D texBushes[NUM_BUSH_TYPES];
  Texture2D texSnowBushes[NUM_SNOW_BUSH_TYPES];
  Texture2D texCactus[NUM_CACTUS_TYPES];

  Texture2D texCrystalsBlue[NUM_CRYSTAL_VARIANTS];
  Texture2D texCrystalsGreen[NUM_CRYSTAL_VARIANTS];
  Texture2D texCrystalsRed[NUM_CRYSTAL_VARIANTS];

  Texture2D texBigRocks[NUM_BIG_ROCK_TYPES];
  Texture2D texSnowRockShadows[NUM_SNOW_ROCK_SHADOWS];
  Texture2D texDesertRockShadows[NUM_DESERT_ROCK_SHADOWS];

  Texture2D texFlowers[NUM_FLOWER_TYPES];
  Texture2D texMushrooms[NUM_MUSHROOM_TYPES];

  Texture2D texSmallRocks[NUM_SMALL_ROCK_TYPES];
  Texture2D texMediumRocks[NUM_MEDIUM_ROCK_TYPES];

  // Helpers
  Texture2D GetTextureForTile(TileType type);
  Texture2D GetTextureForUI(TileType type);
  Texture2D GetTextureForUI(DecorationType type);

private:
  bool texturesLoaded = false;
};
