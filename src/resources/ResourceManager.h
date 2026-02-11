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
  static const int NUM_WATER_VARIANTS = 7;
  static const int NUM_GRASS_VARIANTS = 3;
  static const int NUM_SAND_VARIANTS = 3; // Updated: 3 sand variants
  static const int NUM_SNOW_VARIANTS = 4; // New: 4 snow variants
  static const int NUM_MOUNTAIN_VARIANTS = 2;
  static const int NUM_FOREST_VARIANTS = 2;

  // Tile Decorations
  static const int NUM_GRASS_DECORATIONS = 4;  // 2 mushrooms + 1 rock + 1 trunk
  static const int NUM_FOREST_DECORATIONS = 3; // bushes + little plants
  static const int NUM_SAND_DECORATIONS = 3;   // 1 rock + 2 cacti
  static const int NUM_SNOW_DECORATIONS = 2;   // snow rocks + snowman
  static const int NUM_MOUNTAIN_DECORATIONS = 2; // Rock + Rock2

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

  // Buildings (no color = neutral/first city)
  static const int NUM_CABANA_VARIANTS = 5;
  static const int NUM_CASA_VARIANTS = 12;
  static const int NUM_CASA2_VARIANTS = 5; // estimated
  static const int NUM_RECURSOS_VARIANTS = 15;

  // Textures
  Texture2D texDeepOcean[NUM_WATER_VARIANTS];
  Texture2D texOcean[NUM_WATER_VARIANTS];
  Texture2D texShallowOcean[NUM_WATER_VARIANTS];

  Texture2D texGrass[NUM_GRASS_VARIANTS];
  Texture2D texSand[NUM_SAND_VARIANTS];
  Texture2D texSnow[NUM_SNOW_VARIANTS]; // Updated: array of 4
  Texture2D texIce;                     // New: ice texture
  Texture2D texMountain[NUM_MOUNTAIN_VARIANTS];
  Texture2D texMountainRocks;

  Texture2D texForest[NUM_FOREST_VARIANTS];
  Texture2D texGraminhas;
  Texture2D texBedrock;

  // Tile-based decorations (simple overlays)
  Texture2D texGrassDecorations[NUM_GRASS_DECORATIONS];
  Texture2D texForestDecorations[NUM_FOREST_DECORATIONS];
  Texture2D texSandDecorations[NUM_SAND_DECORATIONS];
  Texture2D texSnowDecorations[NUM_SNOW_DECORATIONS];
  Texture2D texMountainDecorations[NUM_MOUNTAIN_DECORATIONS];

  // -- Decoration Textures --
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

  // Building Textures (neutral/no color)
  Texture2D texCabanas[NUM_CABANA_VARIANTS];
  Texture2D texCasas[NUM_CASA_VARIANTS];
  Texture2D texRecursos[NUM_RECURSOS_VARIANTS];
  Texture2D texStockpileStone; // New Stockpile Texture

  // New Rocks
  Texture2D texRock[4]; // Rock1, Rock2, Rock3, Rock4

  // Helpers
  Texture2D GetTextureForTile(TileType type);
  Texture2D GetTextureForUI(TileType type);
  Texture2D GetTextureForUI(DecorationType type);
  Texture2D GetHumanTexture(bool isWalking, int direction); // New Helper

  // Human Assets (3 States: Idle, Walk, Attack | 4 Directions | Variable
  // Frames) Directions: 0:Down, 1:Right, 2:Left, 3:Up (Matches standard order)
  std::vector<Texture2D> texHumanUnarmed[3][4];
  std::vector<Texture2D>
      texHumanArmed[3][4]; // Changed to vector for variable frames

  // Human Extras
  Texture2D texHumanUnarmedSwim[4][4];
  Texture2D texHumanArmedSwim[4][4];
  Texture2D texHumanArmedBlock[4];
  Texture2D texHumanDeath[4];

  // Boar
  std::vector<Texture2D> texBoarIdle;
  std::vector<Texture2D> texBoarWalk;
  std::vector<Texture2D> texBoarRun;
  std::vector<Texture2D> texBoarAttack;
  std::vector<Texture2D> texBoarHurt;
  std::vector<Texture2D> texBoarDeath;

  // Animal textures (24 frames each: 6 per direction - Down, Up, Left, Right)
  Texture2D texCow[24];
  Texture2D texChicken[24];
  Texture2D texSheep[24];
  Texture2D texBull[24];
  Texture2D texChicken2[24];
  Texture2D texLamb[24];
  Texture2D texPig[24];

  // Terrain Textures
  Texture2D texDirt[3];
  Texture2D texTurkey[24];

  // UI Icons
  Texture2D texUIWaterDeep;
  Texture2D texUIWaterMedium;
  Texture2D texUIWaterShallow;

private:
  bool texturesLoaded = false;
};
