#include "ResourceManager.h"
#include <algorithm>
#include <cstdint>
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

  // Load City Flags (1-80)
  cityFlags.reserve(80);
  for (int i = 1; i <= 80; i++) {
    char filename[64];
    // Assuming filenames are 01.png, 02.png, ..., 80.png
    snprintf(filename, sizeof(filename), "assets/UI/Cities Flags/%02d.png", i);
    Texture2D tex = LoadTexture(filename);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    cityFlags.push_back(tex);
  }
  TraceLog(LOG_INFO, "RESOURCE: Loaded %zu city flags.", cityFlags.size());

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
    TraceLog(LOG_INFO, "DEBUG: texGrass[%d].id = %d, size = %dx%d", i,
             texGrass[i].id, texGrass[i].width, texGrass[i].height);
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

  // Sand decorations (Rock + 2 Cacti)
  texSandDecorations[0] = LoadTexture("assets/tiles/sand/Rock(decoration).png");
  texSandDecorations[1] =
      LoadTexture("assets/tiles/sand/Cactus(decoration).png");
  texSandDecorations[2] =
      LoadTexture("assets/tiles/sand/Cactus2(decoration).png");
  for (int i = 0; i < NUM_SAND_DECORATIONS; i++) {
    SetTextureFilter(texSandDecorations[i], TEXTURE_FILTER_POINT);
  }

  // Load Mountain (mountain folder)
  // Load Mountain (mountain folder)
  texMountain[0] = LoadTexture("assets/tiles/mountain/Mountain1.png");
  texMountain[1] = LoadTexture("assets/tiles/mountain/mountain2.png");
  for (int i = 0; i < NUM_MOUNTAIN_VARIANTS; i++) {
    SetTextureFilter(texMountain[i], TEXTURE_FILTER_POINT);
  }

  // Mountain Decorations (Rocks)
  texMountainDecorations[0] =
      LoadTexture("assets/tiles/mountain/Rock(decoration).png");
  texMountainDecorations[1] =
      LoadTexture("assets/tiles/mountain/Rock2(decoration).png");
  for (int i = 0; i < NUM_MOUNTAIN_DECORATIONS; i++) {
    SetTextureFilter(texMountainDecorations[i], TEXTURE_FILTER_POINT);
  }

  texMountainRocks = LoadTexture(
      "assets/pedrinhasmontanha.png"); // Keeping old single sheet just in case,
                                       // but likely unused?
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
  // New location: assets/Trees/ with subfolders

  // === Forest Trees (from Florest Trees folder) ===
  // Tree1.png, Tree2.png (standard trees)
  texNormalTrees[0] = LoadTexture("assets/Trees/Florest Trees/Tree1.png");
  texNormalTrees[1] = LoadTexture("assets/Trees/Florest Trees/Tree2.png");
  texNormalTrees[2] = LoadTexture(
      "assets/Trees/Florest Trees/Tree1.png"); // Repeat for 3rd slot
  for (int i = 0; i < NUM_NORMAL_TREE_TYPES; i++) {
    SetTextureFilter(texNormalTrees[i], TEXTURE_FILTER_POINT);
  }

  // Fruit Tree
  texFruitTrees[0] = LoadTexture("assets/Trees/Florest Trees/Fruit_tree1.png");
  texFruitTrees[1] = LoadTexture("assets/Trees/Florest Trees/Fruit_tree1.png");
  texFruitTrees[2] = LoadTexture("assets/Trees/Florest Trees/Fruit_tree1.png");
  for (int i = 0; i < NUM_FRUIT_TREE_TYPES; i++) {
    SetTextureFilter(texFruitTrees[i], TEXTURE_FILTER_POINT);
  }

  // texTrees (generic array uses the same normal trees)
  for (int i = 0; i < NUM_TREE_TYPES; i++) {
    texTrees[i] = texNormalTrees[i % NUM_NORMAL_TREE_TYPES];
    SetTextureFilter(texTrees[i], TEXTURE_FILTER_POINT);
  }

  // Moss Trees - using same as normal since moss not available
  for (int i = 0; i < NUM_MOSS_TREE_TYPES; i++) {
    texMossTrees[i] = texNormalTrees[i % NUM_NORMAL_TREE_TYPES];
    SetTextureFilter(texMossTrees[i], TEXTURE_FILTER_POINT);
  }

  // === Snow Trees (from Snow Trees folder) ===
  texSnowTrees[0] = LoadTexture("assets/Trees/Snow Trees/Snow_tree1.png");
  texSnowTrees[1] = LoadTexture("assets/Trees/Snow Trees/Snow_tree1.png");
  texSnowTrees[2] = LoadTexture("assets/Trees/Snow Trees/Snow_tree1.png");
  for (int i = 0; i < NUM_SNOW_TREE_TYPES; i++) {
    SetTextureFilter(texSnowTrees[i], TEXTURE_FILTER_POINT);
  }

  // Xmas Trees
  texXmasTrees[0] =
      LoadTexture("assets/Trees/Snow Trees/Snow_christmass_tree2.png");
  texXmasTrees[1] =
      LoadTexture("assets/Trees/Snow Trees/Snow_christmass_tree2.png");
  texXmasTrees[2] =
      LoadTexture("assets/Trees/Snow Trees/Snow_christmass_tree2.png");
  for (int i = 0; i < NUM_XMAS_TREE_TYPES; i++) {
    SetTextureFilter(texXmasTrees[i], TEXTURE_FILTER_POINT);
  }

  // === Palm Trees (from Praia Tree folder) ===
  texPalmTrees[0] = LoadTexture("assets/Trees/Praia Tree/Palm_tree2_1.png");
  texPalmTrees[1] = LoadTexture("assets/Trees/Praia Tree/Palm_tree2_3.png");
  texPalmTrees[2] = LoadTexture("assets/Trees/Praia Tree/Palm_tree2_1.png");
  texPalmTrees[3] = LoadTexture("assets/Trees/Praia Tree/Palm_tree2_3.png");
  texPalmTrees[4] = LoadTexture("assets/Trees/Praia Tree/Palm_tree2_1.png");
  texPalmTrees[5] = LoadTexture("assets/Trees/Praia Tree/Palm_tree2_3.png");
  for (int i = 0; i < NUM_PALM_TREE_TYPES; i++) {
    SetTextureFilter(texPalmTrees[i], TEXTURE_FILTER_POINT);
  }

  // --- BUILDINGS (no color = neutral/first city) ---
  for (int i = 0; i < NUM_CABANA_VARIANTS; i++) {
    texCabanas[i] = LoadTexture(TextFormat(
        "assets/builds/no color constructions/cabanas/tile%03d.png", i));
    SetTextureFilter(texCabanas[i], TEXTURE_FILTER_POINT);
  }

  for (int i = 0; i < NUM_CASA_VARIANTS; i++) {
    texCasas[i] = LoadTexture(TextFormat(
        "assets/builds/no color constructions/casas/tile%03d.png", i));
    SetTextureFilter(texCasas[i], TEXTURE_FILTER_POINT);
  }

  // Load Casa2 (Stone Mansions) - Files are 003, 004, 005
  for (int i = 0; i < NUM_CASA2_VARIANTS; i++) {
    texCasa2[i] = LoadTexture(TextFormat(
        "assets/builds/no color constructions/casas2/tile%03d.png", i + 3));
    SetTextureFilter(texCasa2[i], TEXTURE_FILTER_POINT);
  }

  for (int i = 0; i < NUM_RECURSOS_VARIANTS; i++) {
    texRecursos[i] = LoadTexture(TextFormat(
        "assets/builds/no color constructions/recursos/tile%03d.png", i));
    SetTextureFilter(texRecursos[i], TEXTURE_FILTER_POINT);
  }

  // Stockpile Stone
  texStockpileStone = LoadTexture("assets/builds/recursos/006.png");
  SetTextureFilter(texStockpileStone, TEXTURE_FILTER_POINT);

  // Load New Rocks (Rock1 - Rock4)
  for (int i = 0; i < 4; i++) {
    texRock[i] = LoadTexture(TextFormat("assets/tiles/Rock/Rock%d.png", i + 1));
    SetTextureFilter(texRock[i], TEXTURE_FILTER_POINT);
  }

  // Load Tree Stumps
  for (int i = 0; i < NUM_STUMP_VARIANTS; i++) {
    texStumps[i] =
        LoadTexture(TextFormat("assets/Trees/stump/stump%d.png", i + 1));
    SetTextureFilter(texStumps[i], TEXTURE_FILTER_POINT);
  }

  // Load Castle textures (tile000-002)
  for (int i = 0; i < NUM_CASTELO_VARIANTS; i++) {
    texCastelo[i] = LoadTexture(TextFormat(
        "assets/builds/no color constructions/castelo/tile%03d.png", i));
    SetTextureFilter(texCastelo[i], TEXTURE_FILTER_POINT);
  }

  // Load Market textures (tile000-011)
  for (int i = 0; i < NUM_MERCADO_VARIANTS; i++) {
    texMercado[i] = LoadTexture(TextFormat(
        "assets/builds/no color constructions/mercados/tile%03d.png", i));
    SetTextureFilter(texMercado[i], TEXTURE_FILTER_POINT);
  }

  // Load Barracks textures (non-sequential: 1-7,12,15,17-19)
  {
    int quartelFiles[] = {1, 2, 3, 4, 5, 6, 7, 12, 15, 17, 18, 19};
    for (int i = 0; i < NUM_QUARTEL_VARIANTS; i++) {
      texQuartel[i] = LoadTexture(TextFormat(
          "assets/builds/no color constructions/quertel/tile%03d.png",
          quartelFiles[i]));
      SetTextureFilter(texQuartel[i], TEXTURE_FILTER_POINT);
    }
  }

  // Load Tavern textures (non-sequential: 0-2,4-5,7-8,10-11)
  {
    int tavernaFiles[] = {0, 1, 2, 4, 5, 7, 8, 10, 11};
    for (int i = 0; i < NUM_TAVERNA_VARIANTS; i++) {
      texTaverna[i] = LoadTexture(TextFormat(
          "assets/builds/no color constructions/tavernas/tile%03d.png",
          tavernaFiles[i]));
      SetTextureFilter(texTaverna[i], TEXTURE_FILTER_POINT);
    }
  }

  // Load Workshop textures (tile000-008)
  for (int i = 0; i < NUM_WORKSHOP_VARIANTS; i++) {
    texWorkshop[i] = LoadTexture(TextFormat(
        "assets/builds/no color constructions/workshops/tile%03d.png", i));
    SetTextureFilter(texWorkshop[i], TEXTURE_FILTER_POINT);
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

  // Ruins
  for (int i = 0; i < 5; i++) {
    texRuins[i] = LoadTexture(TextFormat(
        "assets/top-down-ruins-pixel-art/PNGs/Assets/Brown_ruins%d.png",
        i + 1));
    SetTextureFilter(texRuins[i], TEXTURE_FILTER_POINT);
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

  // --- HUMAN ASSETS ---
  // Paths:
  // Unarmed: assets/char/Normal Charcter FULL animations/Character without
  // weapon/ Armed: assets/char/Normal Charcter FULL animations/Character with
  // sword and shield/ Subfolders: idle, walk, attack Files: [state]
  // [direction][frame].png Directions: down, right, left, up (matches index
  // 0,1,2,3)

  const std::string basePath = "assets/char/Normal Charcter FULL animations/";
  const std::string dirs[] = {"down", "right", "left", "up"};
  const std::string states[] = {"idle", "walk", "attack"};

  // UNARMED (New Main Human)
  // Directions: 0:Down, 1:Right, 2:Left, 3:Up

  // 1. IDLE
  // Front (Down): 27-28
  // Right: 40-41
  // Left: 14-15
  // Back (Up): 01-02
  const char *idlePrefix = "assets/char/New Main Human/No Sword/IDLE/Idle";
  // Down
  texHumanUnarmed[0][0].push_back(
      LoadTexture(TextFormat("%s027.png", idlePrefix)));
  texHumanUnarmed[0][0].push_back(
      LoadTexture(TextFormat("%s028.png", idlePrefix)));
  // Right
  texHumanUnarmed[0][1].push_back(
      LoadTexture(TextFormat("%s040.png", idlePrefix)));
  texHumanUnarmed[0][1].push_back(
      LoadTexture(TextFormat("%s041.png", idlePrefix)));
  // Left
  texHumanUnarmed[0][2].push_back(
      LoadTexture(TextFormat("%s014.png", idlePrefix)));
  texHumanUnarmed[0][2].push_back(
      LoadTexture(TextFormat("%s015.png", idlePrefix)));
  // Up
  texHumanUnarmed[0][3].push_back(
      LoadTexture(TextFormat("%s001.png", idlePrefix)));
  texHumanUnarmed[0][3].push_back(
      LoadTexture(TextFormat("%s002.png", idlePrefix)));

  // 2. WALK
  // Front (Down): 27-34
  // Right: 40-48
  // Left: 14-22
  // Back (Up): 01-09
  const char *walkPrefix = "assets/char/New Main Human/No Sword/walk/walk";
  // Down
  for (int i = 27; i <= 34; i++)
    texHumanUnarmed[1][0].push_back(
        LoadTexture(TextFormat("%s%03d.png", walkPrefix, i)));
  // Right
  for (int i = 40; i <= 48; i++)
    texHumanUnarmed[1][1].push_back(
        LoadTexture(TextFormat("%s%03d.png", walkPrefix, i)));
  // Left
  for (int i = 14; i <= 22; i++)
    texHumanUnarmed[1][2].push_back(
        LoadTexture(TextFormat("%s%03d.png", walkPrefix, i)));
  // Up
  for (int i = 1; i <= 9; i++)
    texHumanUnarmed[1][3].push_back(
        LoadTexture(TextFormat("%s%03d.png", walkPrefix, i)));

  // 3. ATTACK (Punch) - Mapped to State 2
  // Front (Down): 28-31
  // Right: 40-45
  // Left: 14-19
  // Back (Up): 01-06
  const char *punchPrefix = "assets/char/New Main Human/No Sword/Punch/punch";
  // Down
  for (int i = 28; i <= 31; i++)
    texHumanUnarmed[2][0].push_back(
        LoadTexture(TextFormat("%s%03d.png", punchPrefix, i)));
  // Right
  for (int i = 40; i <= 45; i++)
    texHumanUnarmed[2][1].push_back(
        LoadTexture(TextFormat("%s%03d.png", punchPrefix, i)));
  // Left
  for (int i = 14; i <= 19; i++)
    texHumanUnarmed[2][2].push_back(
        LoadTexture(TextFormat("%s%03d.png", punchPrefix, i)));
  // Up
  for (int i = 1; i <= 6; i++)
    texHumanUnarmed[2][3].push_back(
        LoadTexture(TextFormat("%s%03d.png", punchPrefix, i)));

  // Apply Filters
  for (int s = 0; s < 3; s++) {
    for (int d = 0; d < 4; d++) {
      for (auto &tex : texHumanUnarmed[s][d]) {
        SetTextureFilter(tex, TEXTURE_FILTER_POINT);
      }
    }
  }

  // === WOMAN HUMAN ===
  // Same directional pattern: Up=001, Left=014, Down=027, Right=040
  // 1. IDLE (2 frames per direction)
  const char *wIdlePrefix = "assets/char/New Main Human/Woman/Idle/Idle";
  texHumanWoman[0][0].push_back(
      LoadTexture(TextFormat("%s027.png", wIdlePrefix)));
  texHumanWoman[0][0].push_back(
      LoadTexture(TextFormat("%s028.png", wIdlePrefix)));
  texHumanWoman[0][1].push_back(
      LoadTexture(TextFormat("%s040.png", wIdlePrefix)));
  texHumanWoman[0][1].push_back(
      LoadTexture(TextFormat("%s041.png", wIdlePrefix)));
  texHumanWoman[0][2].push_back(
      LoadTexture(TextFormat("%s014.png", wIdlePrefix)));
  texHumanWoman[0][2].push_back(
      LoadTexture(TextFormat("%s015.png", wIdlePrefix)));
  texHumanWoman[0][3].push_back(
      LoadTexture(TextFormat("%s001.png", wIdlePrefix)));
  texHumanWoman[0][3].push_back(
      LoadTexture(TextFormat("%s002.png", wIdlePrefix)));

  // 2. WALK (8-9 frames per direction)
  const char *wWalkPrefix = "assets/char/New Main Human/Woman/Walk/walk";
  for (int i = 27; i <= 34; i++)
    texHumanWoman[1][0].push_back(
        LoadTexture(TextFormat("%s%03d.png", wWalkPrefix, i)));
  for (int i = 40; i <= 48; i++)
    texHumanWoman[1][1].push_back(
        LoadTexture(TextFormat("%s%03d.png", wWalkPrefix, i)));
  for (int i = 14; i <= 22; i++)
    texHumanWoman[1][2].push_back(
        LoadTexture(TextFormat("%s%03d.png", wWalkPrefix, i)));
  for (int i = 1; i <= 9; i++)
    texHumanWoman[1][3].push_back(
        LoadTexture(TextFormat("%s%03d.png", wWalkPrefix, i)));

  // 3. FARMING (8 frames per direction) - used as "Attack" state (state 2)
  const char *wFarmPrefix = "assets/char/New Main Human/Woman/Farming/Farming";
  for (int i = 27; i <= 34; i++)
    texHumanWoman[2][0].push_back(
        LoadTexture(TextFormat("%s%03d.png", wFarmPrefix, i)));
  for (int i = 40; i <= 47; i++)
    texHumanWoman[2][1].push_back(
        LoadTexture(TextFormat("%s%03d.png", wFarmPrefix, i)));
  for (int i = 14; i <= 21; i++)
    texHumanWoman[2][2].push_back(
        LoadTexture(TextFormat("%s%03d.png", wFarmPrefix, i)));
  for (int i = 1; i <= 8; i++)
    texHumanWoman[2][3].push_back(
        LoadTexture(TextFormat("%s%03d.png", wFarmPrefix, i)));

  // Apply Filters for Woman
  for (int s = 0; s < 3; s++) {
    for (int d = 0; d < 4; d++) {
      for (auto &tex : texHumanWoman[s][d]) {
        SetTextureFilter(tex, TEXTURE_FILTER_POINT);
      }
    }
  }

  // ARMED (With Sword - New Main Human)
  // Directions: 0:Down (Front), 1:Right, 2:Left, 3:Up (Back)
  // Files numbered: Back (001-xxx), Left (014-xxx), Front (027-xxx), Right
  // (040-xxx)

  // 1. IDLE (2 frames per direction)
  // Front (Down): 27-28, Right: 40-41, Left: 14-15, Back (Up): 01-02
  const char *armedIdlePrefix =
      "assets/char/New Main Human/With Sword/Idle/Idle";
  // Down
  texHumanArmed[0][0].push_back(
      LoadTexture(TextFormat("%s027.png", armedIdlePrefix)));
  texHumanArmed[0][0].push_back(
      LoadTexture(TextFormat("%s028.png", armedIdlePrefix)));
  // Right
  texHumanArmed[0][1].push_back(
      LoadTexture(TextFormat("%s040.png", armedIdlePrefix)));
  texHumanArmed[0][1].push_back(
      LoadTexture(TextFormat("%s041.png", armedIdlePrefix)));
  // Left
  texHumanArmed[0][2].push_back(
      LoadTexture(TextFormat("%s014.png", armedIdlePrefix)));
  texHumanArmed[0][2].push_back(
      LoadTexture(TextFormat("%s015.png", armedIdlePrefix)));
  // Up
  texHumanArmed[0][3].push_back(
      LoadTexture(TextFormat("%s001.png", armedIdlePrefix)));
  texHumanArmed[0][3].push_back(
      LoadTexture(TextFormat("%s002.png", armedIdlePrefix)));

  // 2. WALK (9 frames per direction)
  // Front (Down): 27-35, Right: 40-48, Left: 14-22, Back (Up): 01-09
  const char *armedWalkPrefix =
      "assets/char/New Main Human/With Sword/walk/Walk";
  // Down
  for (int i = 27; i <= 35; i++)
    texHumanArmed[1][0].push_back(
        LoadTexture(TextFormat("%s%03d.png", armedWalkPrefix, i)));
  // Right
  for (int i = 40; i <= 48; i++)
    texHumanArmed[1][1].push_back(
        LoadTexture(TextFormat("%s%03d.png", armedWalkPrefix, i)));
  // Left
  for (int i = 14; i <= 22; i++)
    texHumanArmed[1][2].push_back(
        LoadTexture(TextFormat("%s%03d.png", armedWalkPrefix, i)));
  // Up
  for (int i = 1; i <= 9; i++)
    texHumanArmed[1][3].push_back(
        LoadTexture(TextFormat("%s%03d.png", armedWalkPrefix, i)));

  // 3. ATTACK (Slash - 6 frames per direction)
  // Back (Up): 01-06, Left: 07-12, Front (Down): 13-18, Right: 19-24
  const char *armedSlashPrefix =
      "assets/char/New Main Human/With Sword/Slash/Slash";
  // Down (Front) - frames 13-18
  for (int i = 13; i <= 18; i++)
    texHumanArmed[2][0].push_back(
        LoadTexture(TextFormat("%s%03d.png", armedSlashPrefix, i)));
  // Right - frames 19-24
  for (int i = 19; i <= 24; i++)
    texHumanArmed[2][1].push_back(
        LoadTexture(TextFormat("%s%03d.png", armedSlashPrefix, i)));
  // Left - frames 07-12
  for (int i = 7; i <= 12; i++)
    texHumanArmed[2][2].push_back(
        LoadTexture(TextFormat("%s%03d.png", armedSlashPrefix, i)));
  // Up (Back) - frames 01-06
  for (int i = 1; i <= 6; i++)
    texHumanArmed[2][3].push_back(
        LoadTexture(TextFormat("%s%03d.png", armedSlashPrefix, i)));

  // Apply Filters for Armed
  for (int s = 0; s < 3; s++) {
    for (int d = 0; d < 4; d++) {
      for (auto &tex : texHumanArmed[s][d]) {
        SetTextureFilter(tex, TEXTURE_FILTER_POINT);
      }
    }
  }

  // BOAR LOADING
  // Order: Down, Right, Left, Up (to match DirIdx 0,1,2,3)
  std::string boarDirs[] = {"Down", "Right", "Left", "Up"};

  // Idle (4 frames)
  for (int d = 0; d < 4; d++) {
    for (int f = 1; f <= 4; f++) {
      std::string path = "assets/animals/Boar/BoarIDLE/Idle" + boarDirs[d] +
                         std::to_string(f) + ".png";
      Texture2D t = LoadTexture(path.c_str());
      SetTextureFilter(t, TEXTURE_FILTER_POINT);
      texBoarIdle.push_back(t);
    }
  }

  // Walk (6 frames) - Weird naming: WalkDown1.png, WalkDown (2).png
  for (int d = 0; d < 4; d++) {
    for (int f = 1; f <= 6; f++) {
      std::string fname = "Walk" + boarDirs[d];
      if (f == 1)
        fname += "1.png";
      else
        fname += " (" + std::to_string(f) + ").png";

      std::string path = "assets/animals/Boar/BoarWalk/" + fname;
      Texture2D t = LoadTexture(path.c_str());
      SetTextureFilter(t, TEXTURE_FILTER_POINT);
      texBoarWalk.push_back(t);
    }
  }

  // Run (5 frames) - Typo: RunDonw2.png
  for (int d = 0; d < 4; d++) {
    for (int f = 1; f <= 5; f++) {
      std::string fname = "Run" + boarDirs[d] + std::to_string(f) + ".png";
      // Fix Typo
      if (d == 0 && f == 2)
        fname = "RunDonw2.png";

      std::string path = "assets/animals/Boar/BoarRun/" + fname;
      Texture2D t = LoadTexture(path.c_str());
      SetTextureFilter(t, TEXTURE_FILTER_POINT);
      texBoarRun.push_back(t);
    }
  }

  // Attack (5 frames)
  for (int d = 0; d < 4; d++) {
    for (int f = 1; f <= 5; f++) {
      std::string path = "assets/animals/Boar/BoarAttack/Attack" + boarDirs[d] +
                         std::to_string(f) + ".png";
      Texture2D t = LoadTexture(path.c_str());
      SetTextureFilter(t, TEXTURE_FILTER_POINT);
      texBoarAttack.push_back(t);
    }
  }

  // Hurt (4 frames)
  for (int d = 0; d < 4; d++) {
    for (int f = 1; f <= 4; f++) {
      std::string path = "assets/animals/Boar/BoarHurt/Hurt" + boarDirs[d] +
                         std::to_string(f) + ".png";
      Texture2D t = LoadTexture(path.c_str());
      SetTextureFilter(t, TEXTURE_FILTER_POINT);
      texBoarHurt.push_back(t);
    }
  }

  // Death (6 frames)
  for (int d = 0; d < 4; d++) {
    for (int f = 1; f <= 6; f++) {
      std::string path = "assets/animals/Boar/BoarDeath/Death" + boarDirs[d] +
                         std::to_string(f) + ".png";
      Texture2D t = LoadTexture(path.c_str());
      SetTextureFilter(t, TEXTURE_FILTER_POINT);
      texBoarDeath.push_back(t);
    }
  }

  // HUMAN EXTRAS
  // Swim (Unarmed)
  // std::string dirs[] = ... (Using existing)
  for (int d = 0; d < 4; d++) {
    for (int f = 1; f <= 4; f++) {
      std::string path = basePath + "Character without weapon/swim/swim " +
                         dirs[d] + std::to_string(f) + ".png";
      texHumanUnarmedSwim[d][f - 1] = LoadTexture(path.c_str());
      SetTextureFilter(texHumanUnarmedSwim[d][f - 1], TEXTURE_FILTER_POINT);
    }
  }
  // Swim (Armed)
  for (int d = 0; d < 4; d++) {
    for (int f = 1; f <= 4; f++) {
      std::string path = basePath +
                         "Character with sword and shield/swim/swim " +
                         dirs[d] + std::to_string(f) + ".png";
      texHumanArmedSwim[d][f - 1] = LoadTexture(path.c_str());
      SetTextureFilter(texHumanArmedSwim[d][f - 1], TEXTURE_FILTER_POINT);
    }
  }
  // Block (Armed)
  for (int d = 0; d < 4; d++) {
    std::string path = basePath +
                       "Character with sword and shield/block/block " +
                       dirs[d] + ".png";
    texHumanArmedBlock[d] = LoadTexture(path.c_str());
    SetTextureFilter(texHumanArmedBlock[d], TEXTURE_FILTER_POINT);
  }
  // Death
  for (int f = 1; f <= 4; f++) {
    std::string path = "assets/char/New Main Human/death animation/death" +
                       std::to_string(f) + ".png";
    texHumanDeath[f - 1] = LoadTexture(path.c_str());
    SetTextureFilter(texHumanDeath[f - 1], TEXTURE_FILTER_POINT);
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

  // UI Notifications
  texNotifLeft = LoadTexture(
      "assets/UI/Pup-Up Backgroud/AvisoPupUp/AvisoPupUp-Esquerda.png");
  texNotifMid =
      LoadTexture("assets/UI/Pup-Up Backgroud/AvisoPupUp/AvisoPupUp-Meio.png");
  texNotifRight = LoadTexture(
      "assets/UI/Pup-Up Backgroud/AvisoPupUp/AvisoPupUp-Direita.png");
  SetTextureFilter(texNotifLeft, TEXTURE_FILTER_POINT);
  SetTextureFilter(texNotifMid, TEXTURE_FILTER_POINT);
  SetTextureFilter(texNotifRight, TEXTURE_FILTER_POINT);

  // --- SLIME MOB ---
  auto loadSlimeAnim = [](const std::string& prefix, int framesPerDir, std::vector<std::vector<Texture2D>>& anim) {
      anim.resize(4);
      for (int dir = 0; dir < 4; ++dir) {
          int startFrame = 0;
          if (dir == 0) startFrame = 1;                     // Down
          else if (dir == 1) startFrame = 1 + 3 * framesPerDir; // Right
          else if (dir == 2) startFrame = 1 + 1 * framesPerDir; // Up
          else if (dir == 3) startFrame = 1 + 2 * framesPerDir; // Left
          
          for (int f = 0; f < framesPerDir; ++f) {
              int frameNumber = startFrame + f;
              std::string path = prefix + std::to_string(frameNumber) + ".png";
              Texture2D tex = LoadTexture(path.c_str());
              if (tex.id > 0) {
                  SetTextureFilter(tex, TEXTURE_FILTER_POINT);
                  anim[dir].push_back(tex);
              }
          }
      }
  };

  loadSlimeAnim("assets/Mobs/Slime mob/PNG/Slime1/Idle/SlimeIDLE-", 6, slimeIdle);
  loadSlimeAnim("assets/Mobs/Slime mob/PNG/Slime1/Walk/SlimeWalk-", 8, slimeWalk);
  loadSlimeAnim("assets/Mobs/Slime mob/PNG/Slime1/Attack/SlimeAttack-", 10, slimeAttack);
  loadSlimeAnim("assets/Mobs/Slime mob/PNG/Slime1/Hurt/SlimeHurt-", 5, slimeHurt);
  loadSlimeAnim("assets/Mobs/Slime mob/PNG/Slime1/Death/SlimeDeath-", 10, slimeDeath);

  // --- DRAGON BOSS ---
  for (int i = 1; i <= 12; i++) {
    char filename[128];
    snprintf(filename, sizeof(filename), "assets/Mobs/Dragon/Fly Sprites/Dragon%03d.png", i);
    Texture2D tex = LoadTexture(filename);
    if (tex.id == 0) { // Check for failure
      TraceLog(LOG_ERROR, "Failed to load texture: %s", filename);
    } else { // Success
      SetTextureFilter(tex, TEXTURE_FILTER_POINT);
      texDragonFly.push_back(tex);
    }
  }

  // --- FIRE EFFECT (ORANGE) ---
  char filename[128]; // Declare filename once for the fire effects
  for (int i = 1; i <= 4; i++) {
    snprintf(filename, sizeof(filename), "assets/Fire Sprites/png/orange/start/burning_start_1/burning_start_%d.png", i);
    Texture2D tex = LoadTexture(filename);
    if (tex.id == 0) TraceLog(LOG_ERROR, "Failed to load fire start texture: %s", filename);
    else {
      SetTextureFilter(tex, TEXTURE_FILTER_POINT);
      texFireStart.push_back(tex);
    }
  }
  for (int i = 1; i <= 8; i++) {
    snprintf(filename, sizeof(filename), "assets/Fire Sprites/png/orange/loops/burning_loop_1/burning_loop_%d.png", i);
    Texture2D tex = LoadTexture(filename);
    if (tex.id == 0) TraceLog(LOG_ERROR, "Failed to load fire loop texture: %s", filename);
    else {
      SetTextureFilter(tex, TEXTURE_FILTER_POINT);
      texFireLoop.push_back(tex);
    }
  }
  for (int i = 1; i <= 5; i++) {
    snprintf(filename, sizeof(filename), "assets/Fire Sprites/png/orange/end/burning_end_1/burning_end_%d.png", i);
    Texture2D tex = LoadTexture(filename);
    if (tex.id == 0) TraceLog(LOG_ERROR, "Failed to load fire end texture: %s", filename);
    else {
      SetTextureFilter(tex, TEXTURE_FILTER_POINT);
      texFireEnd.push_back(tex);
    }
  }

  // --- THUNDER STRIKE ---
  for (int i = 1; i <= 12; i++) {
    snprintf(filename, sizeof(filename), "assets/Spells/Thunder Effect 02/Thunder Strike/Thunder%d.png", i);
    Texture2D tex = LoadTexture(filename);
    if (tex.id == 0) TraceLog(LOG_ERROR, "Failed to load thunder texture: %s", filename);
    else {
      SetTextureFilter(tex, TEXTURE_FILTER_POINT);
      texThunderStrike.push_back(tex);
    }
  }

  // --- TORNADO ---
  for (int i = 1; i <= 9; i++) {
    snprintf(filename, sizeof(filename), "assets/Spells/Tornado/%03d.png", i);
    Texture2D tex = LoadTexture(filename);
    if (tex.id == 0) TraceLog(LOG_ERROR, "Failed to load tornado texture: %s", filename);
    else {
      SetTextureFilter(tex, TEXTURE_FILTER_POINT);
      texTornado.push_back(tex);
    }
  }

  // --- FIRE BOMB ---
  for (int i = 1; i <= 14; i++) {
    snprintf(filename, sizeof(filename), "assets/Spells/FireBomb/Fire-bomb%d.png", i);
    Texture2D tex = LoadTexture(filename);
    if (tex.id == 0) TraceLog(LOG_ERROR, "Failed to load firebomb texture: %s", filename);
    else {
      SetTextureFilter(tex, TEXTURE_FILTER_POINT);
      texFireBomb.push_back(tex);
    }
  }

  // --- DARK BOLT ---
  for (int i = 1; i <= 11; i++) {
    snprintf(filename, sizeof(filename), "assets/Spells/DarkBolt/Dark-Bolt%d.png", i);
    Texture2D tex = LoadTexture(filename);
    if (tex.id == 0) TraceLog(LOG_ERROR, "Failed to load darkbolt texture: %s", filename);
    else {
      SetTextureFilter(tex, TEXTURE_FILTER_POINT);
      texDarkBolt.push_back(tex);
    }
  }

  // --- THUNDER EFFECT 2 ---
  for (int i = 1; i <= 10; i++) {
    snprintf(filename, sizeof(filename), "assets/Spells/Thunder Effect/Lightning%d.png", i);
    Texture2D tex = LoadTexture(filename);
    if (tex.id == 0) TraceLog(LOG_ERROR, "Failed to load thunder2 texture: %s", filename);
    else {
      SetTextureFilter(tex, TEXTURE_FILTER_POINT);
      texThunder2.push_back(tex);
    }
  }

  // --- VFX ASSETS ---
  // Lightning (fx2_electric_burst_large_violet - 16 frames)
  for (int i = 0; i < 16; i++) {
    // char filename[128]; // Already declared above for fire effects, reuse it
    snprintf(filename, sizeof(filename),
             "assets/Effects/Pixel Effects some "
             "spells/PNG/fx2_electric_burst_large_violet/frame%04d.png",
             i);
    Texture2D tex = LoadTexture(filename);
    if (tex.id > 0) {
      SetTextureFilter(tex, TEXTURE_FILTER_POINT);
      texVfxLightning.push_back(tex);
    }
  }
  TraceLog(LOG_INFO, "RESOURCE: Loaded %zu lightning frames.",
           texVfxLightning.size());

  // Fire (burning_loop_1 - 8 frames)
  for (int i = 1; i <= 8; i++) {
    char filename[128];
    snprintf(filename, sizeof(filename),
             "assets/Fire "
             "Sprites/png/orange/loops/burning_loop_1/burning_loop_%d.png",
             i);
    Texture2D tex = LoadTexture(filename);
    if (tex.id > 0) {
      SetTextureFilter(tex, TEXTURE_FILTER_POINT);
      texVfxFire.push_back(tex);
    }
  }
  TraceLog(LOG_INFO, "RESOURCE: Loaded %zu fire frames.", texVfxFire.size());

  texturesLoaded = true;
}

void ResourceManager::Unload() {
  if (!texturesLoaded)
    return;

  UnloadTexture(texNotifLeft);
  UnloadTexture(texNotifMid);
  UnloadTexture(texNotifRight);

  // Slime
  for (auto& dir : slimeIdle) for (auto& tex : dir) UnloadTexture(tex);
  slimeIdle.clear();
  for (auto& dir : slimeWalk) for (auto& tex : dir) UnloadTexture(tex);
  slimeWalk.clear();
  for (auto& dir : slimeAttack) for (auto& tex : dir) UnloadTexture(tex);
  slimeAttack.clear();
  for (auto& dir : slimeHurt) for (auto& tex : dir) UnloadTexture(tex);
  slimeHurt.clear();
  for (auto& dir : slimeDeath) for (auto& tex : dir) UnloadTexture(tex);
  slimeDeath.clear();

  // Dragon
  for (auto &tex : texDragonFly) UnloadTexture(tex);
  texDragonFly.clear();

  // Fire Effect
  for (auto &tex : texFireStart) UnloadTexture(tex);
  texFireStart.clear();
  for (auto &tex : texFireLoop) UnloadTexture(tex);
  texFireLoop.clear();
  for (auto &tex : texFireEnd) UnloadTexture(tex);
  texFireEnd.clear();

  // Thunder Strike
  for (auto &tex : texThunderStrike) UnloadTexture(tex);
  texThunderStrike.clear();

  // New Spells
  for (auto &tex : texTornado) UnloadTexture(tex);
  texTornado.clear();
  for (auto &tex : texFireBomb) UnloadTexture(tex);
  texFireBomb.clear();
  for (auto &tex : texDarkBolt) UnloadTexture(tex);
  texDarkBolt.clear();
  for (auto &tex : texThunder2) UnloadTexture(tex);
  texThunder2.clear();

  // VFX
  for (auto &tex : texVfxLightning)
    UnloadTexture(tex);
  texVfxLightning.clear();
  for (auto &tex : texVfxFire)
    UnloadTexture(tex);
  texVfxFire.clear();

  UnloadTexture(texUIWaterDeep);
  UnloadTexture(texUIWaterMedium);
  UnloadTexture(texUIWaterShallow);

  for (int i = 0; i < NUM_WATER_VARIANTS; i++) {
    UnloadTexture(texDeepOcean[i]);
    UnloadTexture(texOcean[i]);
    UnloadTexture(texShallowOcean[i]);
  }
  // Remove individual loops if they are annoying to target,
  // but importantly add Human unload loop.
  // Unload Human Arrays (vectors)
  for (int s = 0; s < 3; s++) {
    for (int d = 0; d < 4; d++) {
      for (auto &tex : texHumanUnarmed[s][d]) {
        UnloadTexture(tex);
      }
      for (auto &tex : texHumanArmed[s][d]) {
        UnloadTexture(tex);
      }
    }
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

  UnloadTexture(texStockpileStone);
  for (int i = 0; i < 4; i++)
    UnloadTexture(texRock[i]);

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

  for (int i = 0; i < 5; i++)
    UnloadTexture(texRuins[i]);

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

Texture2D ResourceManager::GetTextureForTile(TileType type) const {
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
    return texGrass[1];
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

Texture2D ResourceManager::GetTextureForUI(TileType type) const {
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
    return texGrass[1];
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

Texture2D ResourceManager::GetTextureForUI(DecorationType type) const {
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
  case DecorationType::Rock: // Rock1
    return texRock[0];
  case DecorationType::SmallRock: // Rock2
    return texRock[1];
  case DecorationType::MediumRock: // Rock3
    return texRock[2];
  case DecorationType::BigRock: // Rock4
    return texRock[3];
  case DecorationType::Flower:
    return texFlowers[0];
  case DecorationType::Mushroom:
    return texMushrooms[0];
  case DecorationType::Crystal:
    return texCrystalsBlue[0];
  case DecorationType::Ruins:
    return texRuins[0];
  default:
    return {0};
  }
}
