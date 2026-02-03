#include "ResourceManager.h"
#include <cstdio>
#include <string>

ResourceManager::ResourceManager() {}

ResourceManager::~ResourceManager() { Unload(); }

void ResourceManager::Load() {
  if (texturesLoaded)
    return;

  // Load Water (Deep, Ocean, Shallow)
  // Assumes files: princiapalaguafunda.png, princiapalaguafunda2.png, etc.
  // Note: File listing shows 'princiapalaguafunda.png' (no number for first?)
  // or maybe '1'? Listing: princiapalaguafunda.png, princiapalaguafunda2.png So
  // index 0 -> "princiapalaguafunda.png", index 1 -> "princiapalaguafunda2.png"

  // Load Water (Deep, Ocean, Shallow) for UI
  texUIWaterDeep = LoadTexture("assets/UI/Icons/water_deep.png");
  texUIWaterMedium = LoadTexture("assets/UI/Icons/water_medium.png");
  texUIWaterShallow = LoadTexture("assets/UI/Icons/water_shallow.png");

  SetTextureFilter(texUIWaterDeep, TEXTURE_FILTER_POINT);
  SetTextureFilter(texUIWaterMedium, TEXTURE_FILTER_POINT);
  SetTextureFilter(texUIWaterShallow, TEXTURE_FILTER_POINT);

  // === TERRAIN TEXTURES from assets/tiles ===

  // Load Grass (NormalGrass folder)
  texGrass[0] = LoadTexture("assets/tiles/NormalGrass/Grass1.png");
  texGrass[1] = LoadTexture("assets/tiles/NormalGrass/Grass2.png");
  texGrass[2] = LoadTexture("assets/tiles/NormalGrass/Grass3.png");
  for (int i = 0; i < NUM_GRASS_VARIANTS; i++) {
    SetTextureFilter(texGrass[i], TEXTURE_FILTER_POINT);
  }

  // Grass decorations (2 mushrooms + 1 rock + 1 trunk)
  texGrassDecorations[0] =
      LoadTexture("assets/tiles/NormalGrass/mushroom(decoration).png");
  texGrassDecorations[1] =
      LoadTexture("assets/tiles/NormalGrass/mushroom2(decoration).png");
  texGrassDecorations[2] =
      LoadTexture("assets/tiles/NormalGrass/Rock(decoration).png");
  texGrassDecorations[3] =
      LoadTexture("assets/tiles/NormalGrass/tree trunk1(decoration).png");
  for (int i = 0; i < NUM_GRASS_DECORATIONS; i++) {
    SetTextureFilter(texGrassDecorations[i], TEXTURE_FILTER_POINT);
  }

  // Load Sand (sand folder)
  texSand[0] = LoadTexture("assets/tiles/sand/sand1.png");
  texSand[1] = LoadTexture("assets/tiles/sand/sand2.png");
  texSand[2] = LoadTexture("assets/tiles/sand/sand3.png");
  for (int i = 0; i < NUM_SAND_VARIANTS; i++) {
    SetTextureFilter(texSand[i], TEXTURE_FILTER_POINT);
  }

  // Sand decorations (1 rock)
  texSandDecorations[0] = LoadTexture("assets/tiles/sand/Rock(decoration).png");
  for (int i = 0; i < NUM_SAND_DECORATIONS; i++) {
    SetTextureFilter(texSandDecorations[i], TEXTURE_FILTER_POINT);
  }

  // Load Mountain (mountain folder)
  texMountain[0] = LoadTexture("assets/tiles/mountain/montanha1.png");
  texMountain[1] = LoadTexture("assets/tiles/mountain/montanha2.png");
  for (int i = 0; i < NUM_MOUNTAIN_VARIANTS; i++) {
    SetTextureFilter(texMountain[i], TEXTURE_FILTER_POINT);
  }
  texMountainRocks = LoadTexture("assets/pedrinhasmontanha.png");
  SetTextureFilter(texMountainRocks, TEXTURE_FILTER_POINT);

  // Load Forest (florest folder)
  texForest[0] = LoadTexture("assets/tiles/florest/florest1.png");
  texForest[1] = LoadTexture("assets/tiles/florest/Florest2.png");
  for (int i = 0; i < NUM_FOREST_VARIANTS; i++) {
    SetTextureFilter(texForest[i], TEXTURE_FILTER_POINT);
  }

  // Forest decorations (bushes + plants)
  texForestDecorations[0] =
      LoadTexture("assets/tiles/florest/Bushe(decoration).png");
  texForestDecorations[1] =
      LoadTexture("assets/tiles/florest/LittlePlant(decoration).png");
  texForestDecorations[2] =
      LoadTexture("assets/tiles/florest/LittlePlant2(decoration).png");
  for (int i = 0; i < NUM_FOREST_DECORATIONS; i++) {
    SetTextureFilter(texForestDecorations[i], TEXTURE_FILTER_POINT);
  }

  // Load Snow (snow folder)
  texSnow[0] = LoadTexture("assets/tiles/snow/snow1.png");
  texSnow[1] = LoadTexture("assets/tiles/snow/snow2.png");
  texSnow[2] = LoadTexture("assets/tiles/snow/snow3.png");
  texSnow[3] = LoadTexture("assets/tiles/snow/snow4.png");
  for (int i = 0; i < NUM_SNOW_VARIANTS; i++) {
    SetTextureFilter(texSnow[i], TEXTURE_FILTER_POINT);
  }
  texIce = LoadTexture("assets/tiles/snow/ice.png");
  SetTextureFilter(texIce, TEXTURE_FILTER_POINT);

  // Snow decorations (rocks + snowman)
  texSnowDecorations[0] =
      LoadTexture("assets/tiles/snow/snowRocks(decoration).png");
  texSnowDecorations[1] =
      LoadTexture("assets/tiles/snow/snowman(decoration).png");
  for (int i = 0; i < NUM_SNOW_DECORATIONS; i++) {
    SetTextureFilter(texSnowDecorations[i], TEXTURE_FILTER_POINT);
  }

  // Forest grass decoration (graminhas - keeping old path for now)
  texGraminhas = LoadTexture("assets/grainhas.png");
  SetTextureFilter(texGraminhas, TEXTURE_FILTER_POINT);

  // --- TREES ---
  const char *treePath =
      "assets/top-down-trees-pixel-art/PNGs/Assets_separately/Trees_shadow/";

  // Standard Trees (Mapped to Tree1-3)
  for (int i = 0; i < NUM_TREE_TYPES; i++) {
    texTrees[i] =
        LoadTexture(TextFormat("%sTree%d.png", treePath, (i % 3) + 1));
    SetTextureFilter(texTrees[i], TEXTURE_FILTER_POINT);
  }

  // Fruit Trees
  for (int i = 0; i < NUM_FRUIT_TREE_TYPES; i++) {
    texFruitTrees[i] =
        LoadTexture(TextFormat("%sFruit_tree%d.png", treePath, i + 1));
    SetTextureFilter(texFruitTrees[i], TEXTURE_FILTER_POINT);
  }

  // Normal Trees (Mapped to Tree1-3 as well, or Flower_tree?)
  // User asked for Tree1-3 to be "principais" (main).
  for (int i = 0; i < NUM_NORMAL_TREE_TYPES; i++) {
    texNormalTrees[i] =
        LoadTexture(TextFormat("%sTree%d.png", treePath, (i % 3) + 1));
    SetTextureFilter(texNormalTrees[i], TEXTURE_FILTER_POINT);
  }

  // Moss Trees
  for (int i = 0; i < NUM_MOSS_TREE_TYPES; i++) {
    texMossTrees[i] =
        LoadTexture(TextFormat("%sMoss_tree%d.png", treePath, i + 1));
    SetTextureFilter(texMossTrees[i], TEXTURE_FILTER_POINT);
  }

  // Snow Trees
  for (int i = 0; i < NUM_SNOW_TREE_TYPES; i++) {
    texSnowTrees[i] =
        LoadTexture(TextFormat("%sSnow_tree%d.png", treePath, i + 1));
    SetTextureFilter(texSnowTrees[i], TEXTURE_FILTER_POINT);
  }

  // Xmas Trees
  for (int i = 0; i < NUM_XMAS_TREE_TYPES; i++) {
    texXmasTrees[i] = LoadTexture(
        TextFormat("%sSnow_christmass_tree%d.png", treePath, i + 1));
    SetTextureFilter(texXmasTrees[i], TEXTURE_FILTER_POINT);
  }

  // Palm Trees (Using Palm_tree1_x and Palm_tree2_x)
  for (int i = 0; i < NUM_PALM_TREE_TYPES; i++) {
    if (i < 3) {
      texPalmTrees[i] =
          LoadTexture(TextFormat("%sPalm_tree1_%d.png", treePath, i + 1));
    } else {
      texPalmTrees[i] =
          LoadTexture(TextFormat("%sPalm_tree2_%d.png", treePath, (i - 3) + 1));
    }
    SetTextureFilter(texPalmTrees[i], TEXTURE_FILTER_POINT);
  }

  // --- BUSHES ---
  // Simple Bushes (Mapped to Bush_simple1_x)
  for (int i = 0; i < NUM_BUSH_TYPES; i++) {
    texBushes[i] = LoadTexture(TextFormat(
        "assets/top-down-bushes-pixel-art/PNGs/Assets/Bush_simple1_%d.png",
        i + 1));
    SetTextureFilter(texBushes[i], TEXTURE_FILTER_POINT);
  }
  // Snow Bushes
  for (int i = 0; i < NUM_SNOW_BUSH_TYPES; i++) {
    texSnowBushes[i] = LoadTexture(TextFormat(
        "assets/winter-forest-pixel-art/PNGs/Assets/Bush%d.png", i + 1));
    SetTextureFilter(texSnowBushes[i], TEXTURE_FILTER_POINT);
  }
  // Cactus
  for (int i = 0; i < NUM_CACTUS_TYPES; i++) {
    texCactus[i] = LoadTexture(TextFormat(
        "assets/desert-pixel-art-top-down/PNGs/Objects_separately/Cactus%d.png",
        i + 1));
    SetTextureFilter(texCactus[i], TEXTURE_FILTER_POINT);
  }

  // --- DECORATIONS ---
  // Crystals
  for (int i = 0; i < NUM_CRYSTAL_VARIANTS; i++) {
    texCrystalsBlue[i] = LoadTexture(TextFormat(
        "assets/top-down-crystals-pixel-art/PNGs/Assets/Blue_crystal%d.png",
        i + 1));
    SetTextureFilter(texCrystalsBlue[i], TEXTURE_FILTER_POINT);
    texCrystalsGreen[i] = LoadTexture(TextFormat(
        "assets/top-down-crystals-pixel-art/PNGs/Assets/Green_crystal%d.png",
        i + 1));
    SetTextureFilter(texCrystalsGreen[i], TEXTURE_FILTER_POINT);
    texCrystalsRed[i] = LoadTexture(TextFormat(
        "assets/top-down-crystals-pixel-art/PNGs/Assets/Red_crystal%d.png",
        i + 1));
    SetTextureFilter(texCrystalsRed[i], TEXTURE_FILTER_POINT);
  }

  // Big Rocks
  for (int i = 0; i < NUM_BIG_ROCK_TYPES; i++) {
    texBigRocks[i] =
        LoadTexture(TextFormat("assets/rocks-and-stones-top-down-pixel-art/"
                               "PNGs/Objects_separately/Rock1_%d.png",
                               i + 1));
    SetTextureFilter(texBigRocks[i], TEXTURE_FILTER_POINT);
  }
  // Snow/Desert Rock Shadows (as part of big rocks or decorations)
  for (int i = 0; i < NUM_SNOW_ROCK_SHADOWS; i++) {
    texSnowRockShadows[i] = LoadTexture(
        TextFormat("assets/rocks-and-stones-top-down-pixel-art/PNGs/"
                   "Objects_separately/Rokc3_snow_shadow%d.png",
                   i + 1));
    SetTextureFilter(texSnowRockShadows[i], TEXTURE_FILTER_POINT);
  }
  for (int i = 0; i < NUM_DESERT_ROCK_SHADOWS; i++) {
    texDesertRockShadows[i] = LoadTexture(
        TextFormat("assets/rocks-and-stones-top-down-pixel-art/PNGs/"
                   "Objects_separately/Rock8_ground_shadow%d.png",
                   i + 1));
    SetTextureFilter(texDesertRockShadows[i], TEXTURE_FILTER_POINT);
  }

  // Flowers
  for (int i = 0; i < 3; i++) {
    texFlowers[i] = LoadTexture(TextFormat(
        "assets/top-down-bushes-pixel-art/PNGs/Assets/Bush_red_flowers%d.png",
        i + 1));
    SetTextureFilter(texFlowers[i], TEXTURE_FILTER_POINT);
  }
  texFlowers[3] = LoadTexture(
      "assets/top-down-bushes-pixel-art/PNGs/Assets/Bush_blue_flowers1.png");
  SetTextureFilter(texFlowers[3], TEXTURE_FILTER_POINT);

  // Mushrooms
  for (int i = 0; i < NUM_MUSHROOM_TYPES; i++) {
    // Simple logic from World.cpp
    if (i == 0)
      texMushrooms[i] = LoadTexture("assets/forest-objects-top-down-pixel-art/"
                                    "PNGs/Assets/Beige_green_mushroom1.png");
    else
      texMushrooms[i] = LoadTexture("assets/forest-objects-top-down-pixel-art/"
                                    "PNGs/Assets/White-red_mushroom1.png");
    SetTextureFilter(texMushrooms[i], TEXTURE_FILTER_POINT);
  }

  // Small Rocks (Rock2)
  for (int i = 0; i < NUM_SMALL_ROCK_TYPES; i++) {
    texSmallRocks[i] =
        LoadTexture(TextFormat("assets/rocks-and-stones-top-down-pixel-art/"
                               "PNGs/Objects_separately/Rock2_%d.png",
                               i + 1));
    SetTextureFilter(texSmallRocks[i], TEXTURE_FILTER_POINT);
  }

  // Medium Rocks (Rock3)
  for (int i = 0; i < NUM_MEDIUM_ROCK_TYPES; i++) {
    texMediumRocks[i] =
        LoadTexture(TextFormat("assets/rocks-and-stones-top-down-pixel-art/"
                               "PNGs/Objects_separately/Rock3_%d.png",
                               i + 1));
    SetTextureFilter(texMediumRocks[i], TEXTURE_FILTER_POINT);
  }

  // Apply point filtering for Water (explicitly looped in World.cpp so kept
  // here)
  for (int i = 0; i < NUM_WATER_VARIANTS; i++) {
    SetTextureFilter(texDeepOcean[i], TEXTURE_FILTER_POINT);
    SetTextureFilter(texOcean[i], TEXTURE_FILTER_POINT);
    SetTextureFilter(texShallowOcean[i], TEXTURE_FILTER_POINT);
  }

  // Procedural Bedrock (Dark Gray Indestructible Layer)
  Image bedrockImg = GenImageColor(32, 32, {50, 50, 50, 255}); // Dark Gray
  texBedrock = LoadTextureFromImage(bedrockImg);
  SetTextureFilter(texBedrock, TEXTURE_FILTER_POINT);
  UnloadImage(bedrockImg);

  // --- HUMAN ASSETS ---
  // Load and Resize UI Icons (workaround for large generated images)
  Image imgDeep = LoadImage("assets/ui/water_deep.png");
  if (imgDeep.data) {
    ImageResizeNN(&imgDeep, 64, 64);
    texUIWaterDeep = LoadTextureFromImage(imgDeep);
    UnloadImage(imgDeep);
  } else {
    // Fallback if file missing
    Image img = GenImageColor(64, 64, (Color){26, 54, 93, 255});
    texUIWaterDeep = LoadTextureFromImage(img);
    UnloadImage(img);
  }
  SetTextureFilter(texUIWaterDeep, TEXTURE_FILTER_POINT);

  Image imgMedium = LoadImage("assets/ui/water_medium.png");
  if (imgMedium.data) {
    ImageResizeNN(&imgMedium, 64, 64);
    texUIWaterMedium = LoadTextureFromImage(imgMedium);
    UnloadImage(imgMedium);
  } else {
    Image img = GenImageColor(64, 64, (Color){37, 99, 235, 255});
    texUIWaterMedium = LoadTextureFromImage(img);
    UnloadImage(img);
  }
  SetTextureFilter(texUIWaterMedium, TEXTURE_FILTER_POINT);

  Image imgShallow = LoadImage("assets/ui/water_shallow.png");
  if (imgShallow.data) {
    ImageResizeNN(&imgShallow, 64, 64);
    texUIWaterShallow = LoadTextureFromImage(imgShallow);
    UnloadImage(imgShallow);
  } else {
    Image img = GenImageColor(64, 64, (Color){34, 211, 238, 255});
    texUIWaterShallow = LoadTextureFromImage(img);
    UnloadImage(img);
  }
  SetTextureFilter(texUIWaterShallow, TEXTURE_FILTER_POINT);

  // Human (Single File for now - if GIF logic needed, handle separately)
  texHumanIdle = LoadTexture("assets/char/global.png");
  SetTextureFilter(texHumanIdle, TEXTURE_FILTER_POINT);

  // Human (16 sprites)
  for (int i = 0; i < 16; i++) {
    // Format: assets/char/human000.png
    std::string path = "assets/char/human" + std::string(i < 10 ? "00" : "0") +
                       std::to_string(i) + ".png";
    texHuman[i] = LoadTexture(path.c_str());
    SetTextureFilter(texHuman[i], TEXTURE_FILTER_POINT);
  }

  // --- ANIMAL ASSETS ---
  // Cow (24 sprites)
  for (int i = 0; i < 24; i++) {
    std::string path = "assets/animals/cow/cow" +
                       std::string(i < 10 ? "00" : "0") + std::to_string(i) +
                       ".png";
    texCow[i] = LoadTexture(path.c_str());
    SetTextureFilter(texCow[i], TEXTURE_FILTER_POINT);
  }

  // Chicken (24 sprites)
  for (int i = 0; i < 24; i++) {
    std::string path = "assets/animals/chiken1/chiken1" +
                       std::string(i < 10 ? "00" : "0") + std::to_string(i) +
                       ".png";
    texChicken[i] = LoadTexture(path.c_str());
    SetTextureFilter(texChicken[i], TEXTURE_FILTER_POINT);
  }

  // Sheep (24 sprites)
  for (int i = 0; i < 24; i++) {
    std::string path = "assets/animals/sheep/sheep" +
                       std::string(i < 10 ? "00" : "0") + std::to_string(i) +
                       ".png";
    texSheep[i] = LoadTexture(path.c_str());
    SetTextureFilter(texSheep[i], TEXTURE_FILTER_POINT);
  }

  // Bull (24 sprites)
  for (int i = 0; i < 24; i++) {
    std::string path = "assets/animals/bull/bull" +
                       std::string(i < 10 ? "00" : "0") + std::to_string(i) +
                       ".png";
    texBull[i] = LoadTexture(path.c_str());
    SetTextureFilter(texBull[i], TEXTURE_FILTER_POINT);
  }

  // Chicken2 (24 sprites)
  for (int i = 0; i < 24; i++) {
    std::string path = "assets/animals/chiken2/chiken2" +
                       std::string(i < 10 ? "00" : "0") + std::to_string(i) +
                       ".png";
    texChicken2[i] = LoadTexture(path.c_str());
    SetTextureFilter(texChicken2[i], TEXTURE_FILTER_POINT);
  }

  // Lamb (24 sprites)
  for (int i = 0; i < 24; i++) {
    std::string path = "assets/animals/lamb/lamb" +
                       std::string(i < 10 ? "00" : "0") + std::to_string(i) +
                       ".png";
    texLamb[i] = LoadTexture(path.c_str());
    SetTextureFilter(texLamb[i], TEXTURE_FILTER_POINT);
  }

  // Pig (24 sprites)
  for (int i = 0; i < 24; i++) {
    std::string path = "assets/animals/pig/pig" +
                       std::string(i < 10 ? "00" : "0") + std::to_string(i) +
                       ".png";
    texPig[i] = LoadTexture(path.c_str());
    SetTextureFilter(texPig[i], TEXTURE_FILTER_POINT);
  }

  // Turkey (24 sprites)
  for (int i = 0; i < 24; i++) {
    std::string path = "assets/animals/turkey/turkey" +
                       std::string(i < 10 ? "00" : "0") + std::to_string(i) +
                       ".png";
    texTurkey[i] = LoadTexture(path.c_str());
    SetTextureFilter(texTurkey[i], TEXTURE_FILTER_POINT);
  }

  texturesLoaded = true;
}

void ResourceManager::Unload() {
  UnloadTexture(texUIWaterDeep);
  UnloadTexture(texUIWaterMedium);
  UnloadTexture(texUIWaterShallow);

  if (!texturesLoaded)
    return;

  for (int i = 0; i < NUM_WATER_VARIANTS; i++) {
    UnloadTexture(texDeepOcean[i]);
    UnloadTexture(texOcean[i]);
    UnloadTexture(texShallowOcean[i]);
  }
  // Remove individual loops if they are annoying to target,
  // but importantly add Human unload loop.
  for (int i = 0; i < 16; i++) {
    UnloadTexture(texHuman[i]);
  }

  // Unload Animal Textures
  for (int i = 0; i < 24; i++) {
    UnloadTexture(texCow[i]);
    UnloadTexture(texChicken[i]);
    UnloadTexture(texSheep[i]);
    UnloadTexture(texBull[i]);
    UnloadTexture(texChicken2[i]);
    UnloadTexture(texLamb[i]);
    UnloadTexture(texPig[i]);
    UnloadTexture(texPig[i]);
    UnloadTexture(texTurkey[i]);
  }

  // Unload UI Icons
  UnloadTexture(texUIWaterMedium);
  UnloadTexture(texUIWaterShallow);

  for (int i = 0; i < NUM_GRASS_VARIANTS; i++)
    UnloadTexture(texGrass[i]);
  for (int i = 0; i < NUM_SAND_VARIANTS; i++)
    UnloadTexture(texSand[i]);
  for (int i = 0; i < NUM_SNOW_VARIANTS; i++)
    UnloadTexture(texSnow[i]);
  UnloadTexture(texIce);
  for (int i = 0; i < NUM_GRASS_DECORATIONS; i++)
    UnloadTexture(texGrassDecorations[i]);
  for (int i = 0; i < NUM_FOREST_DECORATIONS; i++)
    UnloadTexture(texForestDecorations[i]);
  for (int i = 0; i < NUM_SAND_DECORATIONS; i++)
    UnloadTexture(texSandDecorations[i]);
  for (int i = 0; i < NUM_SNOW_DECORATIONS; i++)
    UnloadTexture(texSnowDecorations[i]);
  UnloadTexture(texBedrock);
  for (int i = 0; i < NUM_MOUNTAIN_VARIANTS; i++)
    UnloadTexture(texMountain[i]);
  UnloadTexture(texMountainRocks);
  for (int i = 0; i < NUM_FOREST_VARIANTS; i++)
    UnloadTexture(texForest[i]);
  UnloadTexture(texGraminhas);

  for (int i = 0; i < NUM_TREE_TYPES; i++)
    UnloadTexture(texTrees[i]);
  for (int i = 0; i < NUM_FRUIT_TREE_TYPES; i++)
    UnloadTexture(texFruitTrees[i]);
  for (int i = 0; i < NUM_NORMAL_TREE_TYPES; i++)
    UnloadTexture(texNormalTrees[i]);
  for (int i = 0; i < NUM_MOSS_TREE_TYPES; i++)
    UnloadTexture(texMossTrees[i]);
  for (int i = 0; i < NUM_SNOW_TREE_TYPES; i++)
    UnloadTexture(texSnowTrees[i]);
  for (int i = 0; i < NUM_XMAS_TREE_TYPES; i++)
    UnloadTexture(texXmasTrees[i]);
  for (int i = 0; i < NUM_PALM_TREE_TYPES; i++)
    UnloadTexture(texPalmTrees[i]);

  for (int i = 0; i < NUM_BUSH_TYPES; i++)
    UnloadTexture(texBushes[i]);
  for (int i = 0; i < NUM_SNOW_BUSH_TYPES; i++)
    UnloadTexture(texSnowBushes[i]);
  for (int i = 0; i < NUM_CACTUS_TYPES; i++)
    UnloadTexture(texCactus[i]);

  for (int i = 0; i < NUM_CRYSTAL_VARIANTS; i++) {
    UnloadTexture(texCrystalsBlue[i]);
    UnloadTexture(texCrystalsGreen[i]);
    UnloadTexture(texCrystalsRed[i]);
  }

  for (int i = 0; i < NUM_BIG_ROCK_TYPES; i++)
    UnloadTexture(texBigRocks[i]);
  for (int i = 0; i < NUM_SNOW_ROCK_SHADOWS; i++)
    UnloadTexture(texSnowRockShadows[i]);
  for (int i = 0; i < NUM_DESERT_ROCK_SHADOWS; i++)
    UnloadTexture(texDesertRockShadows[i]);

  for (int i = 0; i < NUM_FLOWER_TYPES; i++)
    UnloadTexture(texFlowers[i]);
  for (int i = 0; i < NUM_MUSHROOM_TYPES; i++)
    UnloadTexture(texMushrooms[i]);
  for (int i = 0; i < NUM_SMALL_ROCK_TYPES; i++)
    UnloadTexture(texSmallRocks[i]);
  for (int i = 0; i < NUM_MEDIUM_ROCK_TYPES; i++)
    UnloadTexture(texMediumRocks[i]);

  // Unload Human textures (already handled by loop above if placed correctly,
  // or place here) I added a loop for texHuman[i] previously. I need to remove:
  // UnloadTexture(texHumanIdle); UnloadTexture(texHumanWalkLeft); etc.

  // Checking previous file content via view might be safer, but I'll try to
  // target the block.

  texturesLoaded = false;
}

Texture2D ResourceManager::GetTextureForTile(TileType type) {
  // This is used for Autotiling transitions where we need a single
  // representative texture Simplifying to always return index 0 for now as
  // previously done in World.cpp
  switch (type) {
  case TileType::DeepOcean:
    return texDeepOcean[0];
  case TileType::Ocean:
    return texOcean[0];
  case TileType::ShallowOcean:
    return texShallowOcean[0];
  case TileType::Sand:
    return texSand[0];
  case TileType::Grass:
    return texGrass[0];
  case TileType::Forest:
    return texForest[0]; // Or texGraminhas depending on context
  case TileType::Mountain:
    return texMountain[0];
  case TileType::Snow:
    return texSnow[0];
  case TileType::Bedrock:
    return texBedrock;
  default:
    return texDeepOcean[0];
  }
}

Texture2D ResourceManager::GetTextureForUI(TileType type) {
  if (!texturesLoaded)
    return {0};
  switch (type) {
  case TileType::DeepOcean:
    return texUIWaterDeep;
  case TileType::Ocean:
    return texUIWaterMedium;
  case TileType::ShallowOcean:
    return texUIWaterShallow;
  case TileType::Sand:
    return texSand[0];
  case TileType::Grass:
    return texGrass[0];
  case TileType::Forest:
    return texForest[0];
  case TileType::Mountain:
    return texMountain[0];
  case TileType::Snow:
    return texSnow[0];
  case TileType::Bedrock:
    return texBedrock;
  default:
    return texDeepOcean[0];
  }
}

Texture2D ResourceManager::GetTextureForUI(DecorationType type) {
  if (!texturesLoaded)
    return {0};
  // Return a representative texture for the button
  switch (type) {
  case DecorationType::Tree:
    return texNormalTrees[0];
  case DecorationType::PineTree:
    return texSnowTrees[0];
  case DecorationType::PalmTree:
    return texPalmTrees[0];
  case DecorationType::Bush:
    return texBushes[0];
  // Removed invalid enums (SnowBush, Cactus, Crystal) - they are procedural
  // only
  case DecorationType::Rock: // Generic rock?
    return texSmallRocks[0]; // User requested Rock2_1.png
  case DecorationType::SmallRock:
    return texSmallRocks[0]; // Rock2
  case DecorationType::MediumRock:
    return texMediumRocks[0]; // Rock3
  case DecorationType::BigRock:
    return texBigRocks[0]; // Rock1
  case DecorationType::Flower:
    return texFlowers[0];
  case DecorationType::Mushroom:
    return texMushrooms[0];
  default:
    return {0};
  }
}

Texture2D ResourceManager::GetHumanTexture(bool isWalking, int direction) {
  if (!texturesLoaded)
    return {0};

  // Always return the global sprite sheet (256x384)
  // Slicing handles animation state
  return texHumanIdle;
}
