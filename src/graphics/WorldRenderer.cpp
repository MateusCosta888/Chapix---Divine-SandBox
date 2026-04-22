#include "WorldRenderer.h"
#include "../resources/ResourceManager.h"
#include "../simulation/Citizen.h"
#include "../simulation/City.h"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

using namespace std;

WorldRenderer::WorldRenderer(World &world) : world(world) {}

void WorldRenderer::Draw(const Camera2D &camera,
                         const std::map<int, City> *cities) {
  int tileSize = 10;
  int width = world.GetWidth();
  int height = world.GetHeight();
  ResourceManager &resourceManager = world.GetResourceManager();

  // CAMERA CULLING SETUP
  Vector2 topLeft = GetScreenToWorld2D({0, 0}, camera);
  Vector2 bottomRight = GetScreenToWorld2D(
      {(float)GetScreenWidth(), (float)GetScreenHeight()}, camera);

  int cullStartX = std::max(0, (int)(topLeft.x / tileSize) - 2);
  int cullStartY = std::max(0, (int)(topLeft.y / tileSize) - 2);
  int cullEndX = std::min(width, (int)(bottomRight.x / tileSize) + 2);
  int cullEndY = std::min(height, (int)(bottomRight.y / tileSize) + 2);

  // PASS 1: Draw Terrain and Shadows
  for (int y = cullStartY; y < cullEndY; y++) {
    for (int x = cullStartX; x < cullEndX; x++) {
      Tile &tile = world.GetTile(x, y);

      // Check if this is an ocean tile with texture
      bool usedTexture = false;
      if (resourceManager.IsLoaded()) {
        Texture2D *tex = nullptr;

        // Use pre-calculated variant
        unsigned int tileHash = tile.variant;

        switch (tile.type) {
        // Water tiles: Use simple procedural rendering (shader disabled for now
        // - needs RenderTexture optimization)
        case TileType::DeepOcean:
        case TileType::Ocean:
        case TileType::ShallowOcean:
          // Skip texture, use color fallback + procedural effects
          tex = nullptr;
          break;
        case TileType::Grass: {
          // Simple grass rendering with 2 working variants (skip broken [0])
          int grassProb = tileHash % 100;
          if (grassProb < 50) {
            tex = &resourceManager.texGrass[1];
          } else {
            tex = &resourceManager.texGrass[2];
          }
          break;
        }
        case TileType::Forest: {
          // Forest uses dedicated textures: forest1 (60%), forest2 (40%)
          int forestProb = tileHash % 100;
          if (forestProb < 60) {
            tex = &resourceManager.texForest[0];
          } else {
            tex = &resourceManager.texForest[1];
          }
          break;
        }
        case TileType::Sand:
        case TileType::DesertSand: // Use same sand texture for now
          tex = &resourceManager
                     .texSand[tileHash % ResourceManager::NUM_SAND_VARIANTS];
          break;
        case TileType::Snow:
          tex = &resourceManager
                     .texSnow[tileHash % ResourceManager::NUM_SNOW_VARIANTS];
          break;
        case TileType::Mountain:
          tex = &resourceManager
                     .texMountain[tileHash %
                                  ResourceManager::NUM_MOUNTAIN_VARIANTS];
          break;
        case TileType::Bedrock:
          tex = &resourceManager.texBedrock;
          break;
        default:
          break;
        }

        if (tex != nullptr && tex->id > 0) {
          float srcW = (float)tex->width;
          float srcH = (float)tex->height;
          float srcX = 0;
          float srcY = 0;
          Color tint = WHITE; // Default tint

          // LOGIC: Slice textures based on their size
          // Individual 32x32 textures are used directly
          // Water tileset (96x32) needs slicing - 3 variants per row
          if (tex->width == 96 && tex->height == 32) {
            // Water tileset (96x32) - 3 variants in a horizontal row
            srcW = 32;
            srcH = 32;
            int var = tileHash % 3; // Random variant selection
            srcX = (float)(var * 32);
            srcY = 0;
          } else if (tex->width == 96 && tex->height == 192) {
            // Old water tileset (96x192) - kept for compatibility
            srcW = 32;
            srcH = 32;
            int var = tileHash % 3;
            srcX = (float)(var * 32);
            srcY = 160;
          } else if (tex->width > 64 || tex->height > 64) {
            // Other large tilesets
            srcW = 32;
            srcH = 32;
            if (tex->width >= 96 && tex->height >= 192) {
              int var = (x + y) % 3;
              srcX = (float)(var * 32);
              srcY = 32; // Row 1
            }
          } else if (tex->width == 64 && tex->height == 32) {
            // Rock texture (2 tiles)
            srcW = 32;
            srcH = 32;
            srcX = (float)((x % 2) * 32);
          }
          // Else: Use full texture for truly single tiles

          Rectangle src = {srcX, srcY, srcW, srcH};
          Rectangle dest = {(float)(x * tileSize + tileSize / 2),
                            (float)(y * tileSize + tileSize / 2),
                            (float)tileSize, (float)tileSize};
          Vector2 origin = {(float)tileSize / 2, (float)tileSize / 2};
          DrawTexturePro(*tex, src, dest, origin, 0.0f, tint);
          usedTexture = true;

          // === TILE DECORATIONS (simple overlays) ===
          // These appear with low probability on certain tile types
          int decorChance = (tileHash * 31) % 100;
          Texture2D *decorTex = nullptr;

          if (tile.type == TileType::Grass && decorChance < 6) {
            // 6% chance of decoration on grass
            // Index 0,1 = mushrooms (common), 2 = rock (common), 3 = trunk
            // (rare)
            int decorRoll = (tileHash / 100) % 10;
            int decorIdx;
            if (decorRoll < 4) {
              decorIdx = 0; // mushroom1 (40%)
            } else if (decorRoll < 7) {
              decorIdx = 1; // mushroom2 (30%)
            } else if (decorRoll < 9) {
              decorIdx = 2; // rock (20%)
            } else {
              decorIdx = 3; // trunk (10% - rare)
            }
            decorTex = &resourceManager.texGrassDecorations[decorIdx];
          } else if (tile.type == TileType::Forest && decorChance < 10) {
            // 10% chance of decoration on forest
            int decorIdx =
                (tileHash / 100) % ResourceManager::NUM_FOREST_DECORATIONS;
            decorTex = &resourceManager.texForestDecorations[decorIdx];
          } else if ((tile.type == TileType::Sand ||
                      tile.type == TileType::DesertSand) &&
                     decorChance < 1) {
            // 1% chance of decoration on sand (very rare)
            decorTex = &resourceManager.texSandDecorations[0];
          } else if (tile.type == TileType::Snow && decorChance < 4) {
            // 4% chance of decoration on snow
            int decorIdx =
                (tileHash / 100) % ResourceManager::NUM_SNOW_DECORATIONS;
            decorTex = &resourceManager.texSnowDecorations[decorIdx];
          }

          if (decorTex != nullptr && decorTex->id > 0) {
            Rectangle decorSrc = {0, 0, (float)decorTex->width,
                                  (float)decorTex->height};
            DrawTexturePro(*decorTex, decorSrc, dest, origin, 0.0f, WHITE);
          }

          // === EDGE SHADOWS (procedural depth) ===
          // Draw subtle shadows on tile edges where different terrain meets
          if (tile.edgeMask != 0) {
            int shadowSize = 2; // pixels (reduced)
            unsigned char shadowAlpha =
                15 + (unsigned char)((1.0f - tile.biomeDistance) * 10);

            // North edge
            if (tile.edgeMask & 0x01) {
              DrawRectangle(x * tileSize, y * tileSize, tileSize, shadowSize,
                            (Color){0, 0, 0, shadowAlpha});
            }
            // East edge
            if (tile.edgeMask & 0x02) {
              DrawRectangle(x * tileSize + tileSize - shadowSize, y * tileSize,
                            shadowSize, tileSize,
                            (Color){0, 0, 0, shadowAlpha});
            }
            // South edge
            if (tile.edgeMask & 0x04) {
              DrawRectangle(x * tileSize, y * tileSize + tileSize - shadowSize,
                            tileSize, shadowSize,
                            (Color){0, 0, 0, shadowAlpha});
            }
            // West edge
            if (tile.edgeMask & 0x08) {
              DrawRectangle(x * tileSize, y * tileSize, shadowSize, tileSize,
                            (Color){0, 0, 0, shadowAlpha});
            }
          }

          // Apply procedural water effects on top of texture
          if (IsWaterTile(tile.type)) {
            float time = (float)GetTime();
            DrawWaterEffects(x, y, tile.type, x * tileSize, y * tileSize,
                             tileSize, time);
          }
        }
      }

      // Fallback to color
      if (!usedTexture) {
        Color color;
        float variation = ((x * 7 + y * 13) % 20) / 100.0f;
        float heightVar = tile.height * 0.15f;

        switch (tile.type) {
        case TileType::DeepOcean:
          color = (Color){20, 40, (unsigned char)(100 + variation * 30), 255};
          break;
        case TileType::Ocean:
          color = (Color){30, (unsigned char)(90 + variation * 40),
                          (unsigned char)(160 + variation * 30), 255};
          break;
        case TileType::ShallowOcean:
          color = (Color){70, (unsigned char)(150 + variation * 30),
                          (unsigned char)(200 + variation * 20), 255};
          break;
        case TileType::DesertSand:
        case TileType::Sand:
          color = (Color){(unsigned char)(210 + variation * 20),
                          (unsigned char)(180 + variation * 20),
                          (unsigned char)(120 + variation * 15), 255};
          break;
        case TileType::Grass:
          color =
              (Color){(unsigned char)(60 + variation * 30),
                      (unsigned char)(140 + heightVar * 40 + variation * 20),
                      (unsigned char)(50 + variation * 20), 255};
          break;
        case TileType::Forest:
          color = (Color){(unsigned char)(20 + variation * 15),
                          (unsigned char)(80 + heightVar * 30 + variation * 20),
                          (unsigned char)(30 + variation * 15), 255};
          break;
        case TileType::Mountain:
          color = (Color){(unsigned char)(90 + heightVar * 40 + variation * 25),
                          (unsigned char)(85 + heightVar * 35 + variation * 20),
                          (unsigned char)(80 + heightVar * 30 + variation * 15),
                          255};
          break;
        case TileType::Snow:
          color = (Color){(unsigned char)(240 + variation * 15),
                          (unsigned char)(245 + variation * 10), 255, 255};
          break;
        case TileType::Bedrock:
          color = (Color){50, 50, 50, 255};
          break;
        default:
          color = BLACK;
        }

        DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize, color);

        // Apply procedural water effects on top of fallback color
        if (IsWaterTile(tile.type)) {
          float time = (float)GetTime();
          DrawWaterEffects(x, y, tile.type, x * tileSize, y * tileSize,
                           tileSize, time);
        }
      }

      // Shadows
      Color shadowColor = (Color){0, 0, 0, 50};
      if (x < width - 1) {
        if (world.GetTile(x + 1, y).type != tile.type)
          DrawRectangle(x * tileSize + tileSize - 1, y * tileSize, 1, tileSize,
                        shadowColor);
      }
      if (y < height - 1) {
        if (world.GetTile(x, y + 1).type != tile.type)
          DrawRectangle(x * tileSize, y * tileSize + tileSize - 1, tileSize, 1,
                        shadowColor);
      }
    }
  }

  // PASS 2: Collect Renderable Items (Decorations & Entities)
  std::vector<RenderItem> items;
  if (resourceManager.IsLoaded()) {
    int cullStartXpass2 = std::max(0, cullStartX - 2);
    int cullStartYpass2 = std::max(0, cullStartY - 2);
    int cullEndXpass2 = std::min(width, cullEndX + 2);
    int cullEndYpass2 = std::min(height, cullEndY + 2);

    for (int y = cullStartYpass2; y < cullEndYpass2; y++) {
      for (int x = cullStartXpass2; x < cullEndXpass2; x++) {
        Tile &tile = world.GetTile(x, y);

        // Render stump if tile has one and no decoration
        if (tile.hasStump && tile.decoration == DecorationType::None) {
          Texture2D *stumpTex =
              &resourceManager.texStumps[tile.stumpVariant %
                                         ResourceManager::NUM_STUMP_VARIANTS];
          if (stumpTex && stumpTex->id > 0) {
            float stumpScale = 1.0f;
            float sw = tileSize * stumpScale;
            float sh = sw * ((float)stumpTex->height / (float)stumpTex->width);
            Rectangle stumpSrc = {0, 0, (float)stumpTex->width,
                                  (float)stumpTex->height};
            Rectangle stumpDest = {(float)(x * tileSize + tileSize / 2),
                                   (float)(y * tileSize + tileSize / 2), sw,
                                   sh};
            Vector2 stumpOrigin = {sw / 2, sh / 2};
            float stumpSortY = stumpDest.y + sh / 2;
            items.push_back({*stumpTex, stumpSrc, stumpDest, stumpOrigin, WHITE,
                             stumpSortY});
          }
          continue;
        }

        if (tile.decoration == DecorationType::None)
          continue;

        Texture2D *tex = nullptr;
        float scale = 1.0f;
        int v = tile.decorationVariant;

        switch (tile.decoration) {
        case DecorationType::Tree:
          tex =
              &resourceManager
                   .texNormalTrees[v % ResourceManager::NUM_NORMAL_TREE_TYPES];
          // Use Fruit or Moss if we want variety? WorldRenderer pass 2 had
          // logic. Since we just stored "Tree", let's use the variant to pick
          // different sets? Or just stick to NormalTrees for simplicity as
          // "Generic Tree". For better fidelity, check variant range or map
          // consistently. Pass 2 logic: 0=Fruit, 1=Normal, 2=Moss.
          {
            int treeType = (v / 10) % 3;
            int treeIdx = v % 3;
            if (treeType == 0 && ResourceManager::NUM_FRUIT_TREE_TYPES > 0)
              tex = &resourceManager
                         .texFruitTrees[treeIdx %
                                        ResourceManager::NUM_FRUIT_TREE_TYPES];
            else if (treeType == 2 && ResourceManager::NUM_MOSS_TREE_TYPES > 0)
              tex = &resourceManager
                         .texMossTrees[treeIdx %
                                       ResourceManager::NUM_MOSS_TREE_TYPES];
            else
               tex =
                  &resourceManager
                       .texNormalTrees[v %
                                       ResourceManager::NUM_NORMAL_TREE_TYPES];
          }
          scale = 5.0f;
          break;
        case DecorationType::PineTree:
          // Check for Xmas variant? Pass 2: (seed % 1000) < 100 -> Xmas
          if ((v % 10) == 0 && ResourceManager::NUM_XMAS_TREE_TYPES > 0) {
            tex = &resourceManager
                       .texXmasTrees[(v / 10) %
                                     ResourceManager::NUM_XMAS_TREE_TYPES];
          } else {
            tex = &resourceManager
                       .texSnowTrees[v % ResourceManager::NUM_SNOW_TREE_TYPES];
          }
          scale = 5.0f;
          break;
        case DecorationType::PalmTree:
          tex = &resourceManager
                     .texPalmTrees[v % ResourceManager::NUM_PALM_TREE_TYPES];
          scale = 5.0f;
          break;
        case DecorationType::Bush:
          if (tile.type == TileType::Snow)
            tex = &resourceManager
                       .texSnowBushes[v % ResourceManager::NUM_SNOW_BUSH_TYPES];
          else
            tex =
                &resourceManager.texBushes[v % ResourceManager::NUM_BUSH_TYPES];
          scale = 1.0f;
          break;
        case DecorationType::Cactus:
          tex =
              &resourceManager.texCactus[v % ResourceManager::NUM_CACTUS_TYPES];
          scale = 1.0f;
          break;
        case DecorationType::DesertPlant: // New mixed desert deco
          tex = &resourceManager
                     .texSandDecorations[v %
                                         ResourceManager::NUM_SAND_DECORATIONS];
          scale = 1.0f;
          break;
        case DecorationType::Rock: // Generic Rock
          tex = &resourceManager.texRock[v % 4];
          scale = 2.0f;
          break;
        case DecorationType::SmallRock: 
          tex = &resourceManager.texSmallRocks[v % ResourceManager::NUM_SMALL_ROCK_TYPES];
          scale = 2.0f;
          break;
        case DecorationType::MediumRock:
          tex = &resourceManager.texMediumRocks[v % ResourceManager::NUM_MEDIUM_ROCK_TYPES];
          scale = 2.5f;
          break;
        case DecorationType::BigRock:
          tex = &resourceManager.texBigRocks[v % ResourceManager::NUM_BIG_ROCK_TYPES];
          scale = 3.0f;
          break;
        case DecorationType::Crystal: {
          int type = v % 3;
          if (type == 0)
            tex = &resourceManager
                       .texCrystalsBlue[v %
                                        ResourceManager::NUM_CRYSTAL_VARIANTS];
          else if (type == 1)
            tex = &resourceManager
                       .texCrystalsGreen[v %
                                         ResourceManager::NUM_CRYSTAL_VARIANTS];
          else
            tex =
                &resourceManager
                     .texCrystalsRed[v % ResourceManager::NUM_CRYSTAL_VARIANTS];
        }
          scale = 1.0f;
          break;
        case DecorationType::Ruins:
          tex = &resourceManager.texRuins[v % 5];
          scale = 2.0f;
          break;
        case DecorationType::GrassTuft:
          tex = &resourceManager.texGraminhas;
          scale = 0.8f;
          break;
        case DecorationType::Flower:
          tex = &resourceManager
                     .texFlowers[v % ResourceManager::NUM_FLOWER_TYPES];
          scale = 1.0f;
          break;
        case DecorationType::Mushroom:
          tex = &resourceManager
                     .texMushrooms[v % ResourceManager::NUM_MUSHROOM_TYPES];
          scale = 0.8f;
          break;
        default:
          break;
        }

        if (tex && tex->id > 0) {
          float w = tileSize * scale;
          float h = w * ((float)tex->height / (float)tex->width);

          // Randomized offset used in original renderer
          float offX = ((v % 20) - 10) / 10.0f * (tileSize * 0.2f);
          float offY = (((v / 20) % 20) - 10) / 10.0f * (tileSize * 0.2f);
          if (tile.decoration == DecorationType::Tree ||
              tile.decoration == DecorationType::PineTree ||
              tile.decoration == DecorationType::PalmTree) {
            offX = ((v % 20) - 10) / 10.0f * (tileSize * 0.3f);
            offY = (((v / 20) % 20) - 10) / 10.0f * (tileSize * 0.3f);
          }

          Rectangle src = {0, 0, (float)tex->width, (float)tex->height};
          Rectangle dest = {(float)(x * tileSize + tileSize / 2 + offX),
                            (float)(y * tileSize + tileSize / 2 + offY), w, h};

          // Origin sets the "pivot" point. For sorting, we want Y to be the
          // bottom. Trees: Pivot near bottom.
          Vector2 origin = {w / 2, h * 0.85f};
          if (tile.decoration == DecorationType::Rock ||
              tile.decoration == DecorationType::BigRock)
            origin = {w / 2, h / 2};
          else if (tile.decoration == DecorationType::DesertPlant &&
                   (tile.decorationVariant % 3) == 0) // Rock variant
            origin = {w / 2, h / 2};
          else if (tile.decoration == DecorationType::GrassTuft)
            origin = {w / 2, h / 2};
          else if (tile.decoration == DecorationType::Crystal)
            origin = {w / 2, h / 2};

          Color tColor = WHITE;
          if (tile.decoration == DecorationType::Rock)
            tColor = LIGHTGRAY;

          // Sort Y: anchor to TILE BASE (where trunk meets ground)
          // This ensures correct Z-ordering with entities on the same row
          float sortY;
          if (tile.decoration == DecorationType::Tree ||
              tile.decoration == DecorationType::PineTree ||
              tile.decoration == DecorationType::PalmTree ||
              (tile.decoration == DecorationType::DesertPlant &&
               (tile.decorationVariant % 3) != 0)) {
            // Tall objects: sortY = bottom of the TILE, not the sprite
            sortY = (float)(y * tileSize + tileSize);
          } else {
            sortY = dest.y + h / 2; // Small objects keep center sort
          }

          items.push_back({*tex, src, dest, origin, tColor, sortY});
        }
      }
    }
  }

  // BUILDINGS - Render city buildings (if cities pointer provided)
  if (cities) {
    const auto &cityMap = *cities;
    for (const auto &pair : cityMap) {
      const City &city = pair.second;
      if (!city.isAlive) continue; // Skip dead cities - no ghost buildings
      for (const Building &b : city.buildings) {
        // Construction site rendering
        bool isConstructing = !b.isComplete;

        Texture2D *tex = nullptr;
        float scale = 2.0f;

        switch (b.type) {
        case BuildingType::Cabana:
          tex = &resourceManager
                     .texCabanas[(b.variant %
                                  (ResourceManager::NUM_CABANA_VARIANTS - 1)) +
                                 1]; // Skip variant 0
          scale = 1.5f;
          break;
        case BuildingType::Casa:
          tex = &resourceManager
                     .texCasas[b.variant % ResourceManager::NUM_CASA_VARIANTS];
          scale = 2.0f;
          break;
        case BuildingType::Casa2:
          tex = &resourceManager
                     .texCasa2[b.variant % ResourceManager::NUM_CASA2_VARIANTS];
          scale = 2.0f;
          break;
        case BuildingType::Recursos:
          tex = &resourceManager
                     .texRecursos[b.variant %
                                  ResourceManager::NUM_RECURSOS_VARIANTS];
          scale = 1.5f;
          break;
        case BuildingType::StockpileStone:
          tex = &resourceManager.texStockpileStone;
          scale = 1.5f;
          break;
        case BuildingType::Mina:
          // Mina uses Recursos tiles 09-14
          // variant 0 -> tile 09, variant 5 -> tile 14
          {
            int rIndex = 9 + (b.variant % 6);
            if (rIndex >= ResourceManager::NUM_RECURSOS_VARIANTS)
              rIndex = ResourceManager::NUM_RECURSOS_VARIANTS - 1;
            tex = &resourceManager.texRecursos[rIndex];
            scale = 1.5f;
          }
          break;
        case BuildingType::Castelo:
          tex = &resourceManager
                     .texCastelo[b.variant %
                                 ResourceManager::NUM_CASTELO_VARIANTS];
          scale = 3.0f;
          break;
        case BuildingType::Mercado:
          tex = &resourceManager
                     .texMercado[b.variant %
                                 ResourceManager::NUM_MERCADO_VARIANTS];
          scale = 2.0f;
          break;
        case BuildingType::Quartel:
          tex = &resourceManager
                     .texQuartel[b.variant %
                                 ResourceManager::NUM_QUARTEL_VARIANTS];
          scale = 2.5f;
          break;
        case BuildingType::Taverna:
          tex = &resourceManager
                     .texTaverna[b.variant %
                                 ResourceManager::NUM_TAVERNA_VARIANTS];
          scale = 1.8f;
          break;
        case BuildingType::Workshop:
          tex = &resourceManager
                     .texWorkshop[b.variant %
                                  ResourceManager::NUM_WORKSHOP_VARIANTS];
          scale = 2.0f;
          break;
        default:
          continue;
        }

        if (tex && tex->id > 0) {
          float screenX = b.tileX * tileSize + tileSize / 2;
          float screenY = b.tileY * tileSize + tileSize / 2;
          float w = tex->width * scale;
          float h = tex->height * scale;

          Rectangle src = {0, 0, (float)tex->width, (float)tex->height};
          Rectangle dest = {screenX, screenY, w, h};
          Vector2 origin = {w / 2, h * 0.85f}; // Bottom center anchor

          float sortY = dest.y + h * 0.5f;
          Color bColor = isConstructing ? Fade(WHITE, 0.6f) : WHITE;
          items.push_back({*tex, src, dest, origin, bColor, sortY});

          // Draw progress bar for construction
          if (isConstructing) {
            DrawRectangle(screenX - 15, screenY - 10, 30, 4, BLACK);
            DrawRectangle(screenX - 15, screenY - 10, (int)(30 * b.constructionProgress), 4, ORANGE);
          }
        }
      }
    }
  } // end if (cities)

  // Entities
  const auto &entities = world.GetEntities();
  for (const auto &[eid, e] : entities) {
    if (!resourceManager.IsLoaded())
      continue;

    Texture2D tex;
    // === HUMAN DRAWING ===
    if (e.type == EntityType::HumanUnarmed ||
        e.type == EntityType::HumanArmed || e.type == EntityType::HumanWoman) {
      // Determine Direction Index (0:Down, 1:Right, 2:Left, 3:Up)
      int dirIdx = 0;
      if (e.facingDirection == 1)
        dirIdx = 1;
      else if (e.facingDirection == -1)
        dirIdx = 2;
      else if (e.facingDirection == 2)
        dirIdx = 3;
      else
        dirIdx = 0;

      int frame = e.currentFrame % 4; // Default 4 frames

      if (e.state == EntityState::Swim && e.type != EntityType::HumanWoman) {
        if (e.type == EntityType::HumanUnarmed)
          tex = resourceManager.texHumanUnarmedSwim[dirIdx][frame];
        else
          tex = resourceManager.texHumanArmedSwim[dirIdx][frame];
      } else if (e.state == EntityState::Die &&
                 e.type != EntityType::HumanWoman) {
        tex = resourceManager.texHumanDeath[frame];
      } else if (e.state == EntityState::Block &&
                 e.type == EntityType::HumanArmed) {
        tex = resourceManager.texHumanArmedBlock[dirIdx];
      } else {
        // Standard States (Idle, Walk, Attack/Farming)
        int stateIdx = 0;
        if (e.state == EntityState::Walking || e.state == EntityState::Run)
          stateIdx = 1;
        else if (e.state == EntityState::Attack)
          stateIdx = 2;

        if (e.type == EntityType::HumanWoman) {
          const auto &frames = resourceManager.texHumanWoman[stateIdx][dirIdx];
          if (!frames.empty()) {
            int frame = e.currentFrame % frames.size();
            tex = frames[frame];
          }
        } else if (e.type == EntityType::HumanUnarmed) {
          const auto &frames =
              resourceManager.texHumanUnarmed[stateIdx][dirIdx];
          if (!frames.empty()) {
            int frame = e.currentFrame % frames.size();
            tex = frames[frame];
          }
        } else {
          const auto &frames = resourceManager.texHumanArmed[stateIdx][dirIdx];
          if (!frames.empty()) {
            int frame = e.currentFrame % frames.size();
            tex = frames[frame];
          }
        }
      }
    }
    // === BOAR DRAWING ===
    else if (e.type == EntityType::Boar) {
      // Determine Direction Index: 0:Down, 1:Right, 2:Left, 3:Up
      int dirIdx = 0;
      if (e.facingDirection == 1)
        dirIdx = 1;
      else if (e.facingDirection == -1)
        dirIdx = 2;
      else if (e.facingDirection == 2)
        dirIdx = 3;
      else
        dirIdx = 0;

      // Helper to get safe index
      auto getIdx = [&](const std::vector<Texture2D> &vec, int framesPerDir) {
        if (vec.empty())
          return (Texture2D){0};
        int frame = e.currentFrame % framesPerDir;
        int idx = (dirIdx * framesPerDir) + frame;
        if (idx >= (int)vec.size())
          idx = 0; // Safety
        return vec[idx];
      };

      if (e.state == EntityState::Idle)
        tex = getIdx(resourceManager.texBoarIdle, 4);
      else if (e.state == EntityState::Walking)
        tex = getIdx(resourceManager.texBoarWalk, 6);
      else if (e.state == EntityState::Run)
        tex = getIdx(resourceManager.texBoarRun, 5);
      else if (e.state == EntityState::Attack)
        tex = getIdx(resourceManager.texBoarAttack, 5);
      else if (e.state == EntityState::Hurt)
        tex = getIdx(resourceManager.texBoarHurt, 4);
      else if (e.state == EntityState::Die)
        tex = getIdx(resourceManager.texBoarDeath, 6);
      else
        tex = getIdx(resourceManager.texBoarIdle, 4);
    }
    // === SLIME DRAWING ===
    else if (e.type == EntityType::Slime) {
      int dirIndex = 0; // 0=Down, 1=Right, 2=Up, 3=Left
      if (e.facingDirection == 0) dirIndex = 0; // Down
      else if (e.facingDirection == 1) dirIndex = 1; // Right
      else if (e.facingDirection == 2) dirIndex = 2; // Up
      else if (e.facingDirection == -1) dirIndex = 3; // Left

      const std::vector<Texture2D>* animFrames = nullptr;
      float speed = 0.15f; 

      if (e.state == EntityState::Die) {
          if (dirIndex < resourceManager.slimeDeath.size())
              animFrames = &resourceManager.slimeDeath[dirIndex];
          speed = 0.15f;
      } else if (e.state == EntityState::Attack) {
          if (dirIndex < resourceManager.slimeAttack.size())
              animFrames = &resourceManager.slimeAttack[dirIndex];
          speed = 0.1f;
      } else if (e.state == EntityState::Hurt) {
          if (dirIndex < resourceManager.slimeHurt.size())
              animFrames = &resourceManager.slimeHurt[dirIndex];
          speed = 0.1f;
      } else if (e.state == EntityState::Walking || e.state == EntityState::Run) {
          if (dirIndex < resourceManager.slimeWalk.size())
              animFrames = &resourceManager.slimeWalk[dirIndex];
          speed = 0.12f;
      } else {
          if (dirIndex < resourceManager.slimeIdle.size())
              animFrames = &resourceManager.slimeIdle[dirIndex];
          speed = 0.15f;
      }

      if (animFrames && !animFrames->empty()) {
          int frameCount = animFrames->size();
          int currentFrame = e.currentFrame % frameCount;
          
          if (e.state == EntityState::Die && frameCount > 0) {
              // Clamp death frame to last instead of looping
              currentFrame = std::min(e.currentFrame, frameCount - 1);
          }

          Texture2D texAnim = (*animFrames)[currentFrame];
          if (texAnim.id > 0) {
              float scale = 0.6f; 
              float destW = texAnim.width * scale;
              float destH = texAnim.height * scale;
              
              Rectangle src = {0, 0, (float)texAnim.width, (float)texAnim.height};
              Rectangle dest = {e.position.x * tileSize, e.position.y * tileSize, destW, destH};
              Vector2 origin = {destW / 2.0f, destH * 0.9f};
              
              Color tint = (e.state == EntityState::Hurt) ? (Color){255, 100, 100, 255} : WHITE;
              items.push_back({texAnim, src, dest, origin, tint, dest.y + destH * 0.9f});
              continue;
          }
      }
    }
    // === DRAGON DRAWING ===
    else if (e.type == EntityType::Dragon) {
      if (!resourceManager.texDragonFly.empty()) {
        int framesPerDir = 3; 
        int frameOffset = 0; // Base index
        
        // Possitions.txt:
        // 001-003 back (Up)     -> Indices 0, 1, 2
        // 004-006 right         -> Indices 3, 4, 5
        // 007-009 front (Down)  -> Indices 6, 7, 8
        // 010-012 left          -> Indices 9, 10, 11

        if (e.facingDirection == 0)      // Down (Front)
          frameOffset = 6;
        else if (e.facingDirection == 1) // Right
          frameOffset = 3;
        else if (e.facingDirection == 2) // Up (Back)
          frameOffset = 0;
        else if (e.facingDirection == -1) // Left
          frameOffset = 9;

        int frame = e.currentFrame % framesPerDir;
        int finalIdx = frameOffset + frame;
        
        // Prevent out of bounds
        if (finalIdx >= resourceManager.texDragonFly.size()) finalIdx = 0;
        
        tex = resourceManager.texDragonFly[finalIdx];

        float scale = 0.5f; // Decreased scale based on user feedback
        float destW = tex.width * scale;
        float destH = tex.height * scale;

        Rectangle src = {0, 0, (float)tex.width, (float)tex.height};

        // Add a smooth vertical floating motion using a sine wave
        float floatOffset = sin(GetTime() * 2.0f) * 15.0f; // Scale amplitude for tilesize

        Rectangle dest = {e.position.x * tileSize, (e.position.y * tileSize) + floatOffset,
                          destW, destH};
        Vector2 origin = {destW / 2, destH * 0.9f};

        Color tint = (e.state == EntityState::Hurt)
                         ? (Color){255, 150, 150, 255}
                         : WHITE;
        items.push_back({tex, src, dest, origin, tint, dest.y + destH * 0.9f});

        // Animated Fire Attack
        if (e.state == EntityState::Attack) {
          // Adjust position to spawn from mouth area
          Vector2 mouthOffset = {destW * 0.7f, -destH * 0.1f};
          if (e.facingDirection == 2 || e.facingDirection == -1) mouthOffset.x = -mouthOffset.x;
          
          Vector2 spawnPos = {dest.x + origin.x + mouthOffset.x, dest.y + origin.y + mouthOffset.y};
          
          Texture2D fireTex;
          bool hasFire = false;
          
          int fireFrameOffset = e.currentFrame * 2; // Speed up fire animation slightly 
          int totalStart = resourceManager.texFireStart.size();
          int totalLoop = resourceManager.texFireLoop.size();

          if (totalStart > 0 && totalLoop > 0) {
              if (fireFrameOffset < totalStart) {
                  fireTex = resourceManager.texFireStart[fireFrameOffset];
                  hasFire = true;
              } else {
                  int loopFrame = (fireFrameOffset - totalStart) % totalLoop;
                  fireTex = resourceManager.texFireLoop[loopFrame];
                  hasFire = true;
              }
          }

          if (hasFire) {
              float fireScale = 1.2f;
              Rectangle fireSrc = {0, 0, (float)fireTex.width, (float)fireTex.height};
              
              // Direct Fire Based on Facing Direction (Flame pointing outward)
              if (e.facingDirection == 2 || e.facingDirection == -1) {
                  fireSrc.width = -fireSrc.width; // Flip fire visually
              }

              Rectangle fireDest = {spawnPos.x, spawnPos.y, fireTex.width * fireScale, fireTex.height * fireScale};
              Vector2 fireOrig = {(fireTex.width * fireScale) / 2.0f, (fireTex.height * fireScale) / 2.0f};

              items.push_back({fireTex, fireSrc, fireDest, fireOrig, WHITE, dest.y + destH * 0.99f});
          }
        }
        continue;
      }
    } else {
      // Animals (Cow, Chicken, Sheep, etc)
      int base = 0;
      if (e.facingDirection == 2)
        base = 6;
      else if (e.facingDirection == -1)
        base = 12;
      else if (e.facingDirection == 1)
        base = 18;
      int idx = base + (e.currentFrame % 6);

      if (e.type == EntityType::Cow)
        tex = resourceManager.texCow[idx];
      else if (e.type == EntityType::Chicken)
        tex = resourceManager.texChicken[idx];
      else if (e.type == EntityType::Sheep)
        tex = resourceManager.texSheep[idx];
      else if (e.type == EntityType::Bull)
        tex = resourceManager.texBull[idx];
      else if (e.type == EntityType::Chicken2)
        tex = resourceManager.texChicken2[idx];
      else if (e.type == EntityType::Lamb)
        tex = resourceManager.texLamb[idx];
      else if (e.type == EntityType::Pig)
        tex = resourceManager.texPig[idx];
      else if (e.type == EntityType::Turkey)
        tex = resourceManager.texTurkey[idx];
    }

    if (tex.id > 0) {
      float destW;
      float destH;

      if (e.type == EntityType::HumanUnarmed ||
          e.type == EntityType::HumanArmed) {
        destW = tex.width * 0.40f; // Adjusted
        destH = tex.height * 0.40f;
      } else if (e.type == EntityType::HumanWoman) {
        destW = tex.width * 0.35f; // Slightly smaller than men
        destH = tex.height * 0.35f;
      } else {
        destW = tex.width * 0.5f;
        destH = tex.height * 0.5f;
      }

      Rectangle src = {0, 0, (float)tex.width, (float)tex.height};

      // Flip Boar if facing Left (Direction 2)
      if (e.type == EntityType::Boar && e.facingDirection == 2) {
        src.width = -src.width;
      }
      Rectangle dest = {e.position.x * tileSize, e.position.y * tileSize, destW,
                        destH};
      Vector2 origin = {destW / 2, destH * 0.9f};
      
      float sortY = dest.y; // Correctly sort by the feet/base (where origin.y is)

      if (e.isGrabbed) {
          dest.y += 18.0f + sin(GetTime() * 2.5f) * 3.0f; // Softer animation, visually below cursor
      }

      items.push_back({tex, src, dest, origin, WHITE, sortY});
    } else {
      // Fallback circle drawn immediately (rare)
      DrawCircle((int)(e.position.x * tileSize), (int)(e.position.y * tileSize),
                 tileSize / 2, RED);
    }
  }

  // SORT (stable sort + X tiebreaker to prevent Z-fighting flicker)
  std::stable_sort(items.begin(), items.end(),
                   [](const RenderItem &a, const RenderItem &b) {
                     if (a.sortY != b.sortY)
                       return a.sortY < b.sortY;
                     return a.dest.x < b.dest.x; // Tiebreaker
                   });

  // DRAW SHADOWS FOR GRABBED ENTITIES
  for (const auto &[eid, e] : entities) {
    if (e.isGrabbed) {
      DrawEllipse((int)(e.position.x * tileSize), (int)(e.position.y * tileSize) + 35, 7.0f, 3.5f, (Color){0, 0, 0, 100});
    }
  }

  // DRAW
  for (const auto &item : items) {
    DrawTexturePro(item.texture, item.src, item.dest, item.origin, 0.0f,
                   item.tint);
  }

  // === COMBAT VFX & HEALTH BARS (drawn above entities) ===
  for (const auto &[eid, e] : entities) {
    if (e.health <= 0)
      continue;

    float screenX = e.position.x * tileSize;
    float screenY = e.position.y * tileSize;

    // --- Combat VFX: Attack flash ---
    if (e.state == EntityState::Attack) {
      float effectX = screenX;
      float effectY = screenY - 5.0f;
      if (e.facingDirection == 1)
        effectX += 5.0f;
      else if (e.facingDirection == -1)
        effectX -= 5.0f;
      else if (e.facingDirection == 2)
        effectY -= 5.0f;
      else
        effectY += 5.0f;
      DrawCircle((int)effectX, (int)effectY, 3.0f, Fade(WHITE, 0.7f));
      DrawCircle((int)effectX, (int)effectY, 1.5f, Fade(YELLOW, 0.5f));
    }

    // --- Combat VFX: Hurt red flash ---
    if (e.state == EntityState::Hurt) {
      DrawCircle((int)screenX, (int)screenY - 3, 4.0f, Fade(RED, 0.4f));
    }

    // --- Health Bar ---
    // Determine max HP based on entity type
    // Determine max HP based on entity struct or fallback
    float maxHP = e.maxHP > 0 ? e.maxHP : 20.0f;

    // Only show bar when damaged or selected (not always)
    bool isDamaged = (e.health < maxHP);
    bool isSelected = (e.citizenID >= 0 && e.citizenID == selectedCitizenID);
    if (!isDamaged && !isSelected)
      continue;

    float ratio = e.health / maxHP;
    if (ratio > 1.0f)
      ratio = 1.0f;
    if (ratio < 0.0f)
      ratio = 0.0f;

    float barW = 20.0f, barH = 3.0f;
    float barX = screenX - barW / 2.0f;
    float barY = screenY - 12.0f; // Above entity sprite

    Color barColor = (ratio > 0.5f) ? GREEN : (ratio > 0.25f) ? YELLOW : RED;

    // Background
    DrawRectangle((int)barX - 1, (int)barY - 1, (int)barW + 2, (int)barH + 2,
                  (Color){10, 10, 10, 180});
    // Filled portion
    DrawRectangle((int)barX, (int)barY, (int)(barW * ratio), (int)barH,
                  barColor);
  }



  // === CITY TERRITORY VISUALIZATION ===
  // Draw colored overlays for city territories
  SimulationManager &sim = world.GetSimulation();
  const auto &cities = sim.GetAllCities();

  // Get mouse position in world coordinates for hover detection
  Vector2 screenMousePos = GetMousePosition();
  Vector2 worldMousePos = GetScreenToWorld2D(screenMousePos, camera);
  int hoveredCityID = -1;

  for (const auto &pair : cities) {
    const City &city = pair.second;
    if (!city.isAlive)
      continue;

    // Draw territory tiles with semi-transparent city color
    Color territoryColor = city.color;
    territoryColor.a = 60; // Very transparent

    for (const Vector2 &tile : city.territory) {
      int tx = static_cast<int>(tile.x);
      int ty = static_cast<int>(tile.y);
      DrawRectangle(tx * tileSize, ty * tileSize, tileSize, tileSize,
                    territoryColor);
    }

    // Draw city center marker (brighter)
    Color centerColor = city.color;
    centerColor.a = 200;
    int cx = static_cast<int>(city.center.x) * tileSize;
    int cy = static_cast<int>(city.center.y) * tileSize;
    DrawRectangle(cx - 2, cy - 2, tileSize + 4, tileSize + 4, centerColor);

    // Draw city name above center
    DrawText(city.name.c_str(), cx, cy - 15, 10, WHITE);

    // Check if mouse is hovering over city center (in WORLD coordinates)
    float distToMouse = std::hypot(worldMousePos.x - cx, worldMousePos.y - cy);
    if (distToMouse < tileSize * 3) {
      hoveredCityID = city.id;
    }
  }

  // === CITY INFO TOOLTIP ===
  // Draw tooltip for hovered city
  if (hoveredCityID >= 0) {
    const City *hoveredCity = sim.GetCity(hoveredCityID);
    if (hoveredCity) {
      int tooltipX = static_cast<int>(worldMousePos.x) + 15;
      int tooltipY = static_cast<int>(worldMousePos.y) - 60;

      // Tooltip background
      int boxWidth = 150;
      int boxHeight = 70;
      DrawRectangle(tooltipX - 5, tooltipY - 5, boxWidth, boxHeight,
                    (Color){30, 30, 40, 220});
      DrawRectangleLines(tooltipX - 5, tooltipY - 5, boxWidth, boxHeight,
                         hoveredCity->color);

      // City name
      DrawText(hoveredCity->name.c_str(), tooltipX, tooltipY, 14,
               hoveredCity->color);

      // Population
      char popText[64];
      snprintf(popText, sizeof(popText), "Pop: %d / %d",
               hoveredCity->GetPopulation(), hoveredCity->populationCap);
      DrawText(popText, tooltipX, tooltipY + 18, 12, WHITE);

      // Food
      char foodText[64];
      snprintf(foodText, sizeof(foodText), "Food: %d / %d",
               hoveredCity->resources.food, hoveredCity->maxStorage);
      DrawText(foodText, tooltipX, tooltipY + 34, 12, GREEN);

      // Wood
      char woodText[64];
      snprintf(woodText, sizeof(woodText), "Wood: %d",
               hoveredCity->resources.wood);
      DrawText(woodText, tooltipX, tooltipY + 50, 12, BROWN);

      // Stone
      char stoneText[64];
      snprintf(stoneText, sizeof(stoneText), "Stone: %d",
               hoveredCity->resources.stone);
      DrawText(stoneText, tooltipX, tooltipY + 65, 12, DARKGRAY);
    }
  }

  // === CITIZEN SELECTION (Right-click) ===
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    // Check if clicking on a human entity
    const auto &entities = world.GetEntities();
    int closestCitizenID = -1;
    float closestDist = 999999.0f;
    Vector2 closestPos = {0, 0};

    for (const auto &[eid, e] : entities) {
      if (e.type == EntityType::HumanUnarmed ||
          e.type == EntityType::HumanArmed) {
        float ex = e.position.x * tileSize;
        float ey = e.position.y * tileSize;
        float dist = std::hypot(worldMousePos.x - ex, worldMousePos.y - ey);

        if (dist < tileSize * 2 && dist < closestDist) {
          closestDist = dist;
          closestCitizenID = e.citizenID;
          closestPos = {ex, ey};
        }
      }
    }

    if (closestCitizenID >= 0) {
      selectedCitizenID = closestCitizenID;
      selectedCitizenScreenPos = closestPos;
    } else {
      selectedCitizenID = -1; // Clicked empty space, deselect
    }
  }

  // Left-click anywhere deselects
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    selectedCitizenID = -1;
  }

  // === CITIZEN INFO POPUP ===
  if (selectedCitizenID >= 0) {
    Citizen *citizen = sim.GetCitizen(selectedCitizenID);
    if (citizen && citizen->isAlive) {
      // Find entity for position update in O(1)
      const Entity *e = world.GetEntityByCitizenID(selectedCitizenID);
      if (e) {
        selectedCitizenScreenPos = {e->position.x * tileSize,
                                    e->position.y * tileSize};
      }

      // Draw popup above the citizen
      int popupX = static_cast<int>(selectedCitizenScreenPos.x) - 50;
      int popupY = static_cast<int>(selectedCitizenScreenPos.y) - 55;

      // Get profession string
      const char *professionStr = "Unemployed";
      switch (citizen->profession) {
      case Profession::Gatherer:
        professionStr = "Gatherer";
        break;
      case Profession::Lumberjack:
        professionStr = "Lumberjack";
        break;
      case Profession::Farmer:
        professionStr = "Farmer";
        break;
      case Profession::Miner:
        professionStr = "Miner";
        break;
      case Profession::Builder:
        professionStr = "Builder";
        break;
      case Profession::Soldier:
        professionStr = "Soldier";
        break;
      case Profession::Leader:
        professionStr = "Leader";
        break;
      default:
        if (!citizen->isAdult())
          professionStr = "Child";
        break;
      }

      // Calculate box width based on name length
      int nameWidth = MeasureText(citizen->name.c_str(), 12);
      int profWidth = MeasureText(professionStr, 10);
      int boxWidth = (nameWidth > profWidth ? nameWidth : profWidth) + 20;
      if (boxWidth < 100)
        boxWidth = 100;
      int boxHeight = 40;

      // Background
      DrawRectangle(popupX, popupY, boxWidth, boxHeight,
                    (Color){20, 20, 30, 230});
      DrawRectangleLinesEx((Rectangle){(float)popupX, (float)popupY,
                                       (float)boxWidth, (float)boxHeight},
                           2, GOLD);

      // Name
      DrawText(citizen->name.c_str(), popupX + 5, popupY + 5, 12, WHITE);

      // Profession with color
      Color profColor = GRAY;
      if (citizen->profession == Profession::Lumberjack)
        profColor = BROWN;
      else if (citizen->profession == Profession::Farmer)
        profColor = GREEN;
      else if (citizen->profession == Profession::Soldier)
        profColor = RED;

      DrawText(professionStr, popupX + 5, popupY + 22, 10, profColor);

      // PROGESSION (Level + XP Bar) - Only for adults with jobs
      if (citizen->isAdult() && citizen->profession != Profession::None) {
        float skill = 0.0f;
        if (citizen->profession == Profession::Lumberjack)
          skill = citizen->skillWoodcutting;
        else if (citizen->profession == Profession::Farmer)
          skill = citizen->skillFarming;
        // Add others as needed

        int level = std::min(10, static_cast<int>(skill / 10) + 1);
        float xpPct = (skill - (level - 1) * 10) / 10.0f;
        if (level == 10)
          xpPct = 1.0f;

        char levelText[32];
        snprintf(levelText, sizeof(levelText), "Lv. %d", level);
        DrawText(levelText, popupX + boxWidth - 35, popupY + 22, 10, WHITE);

        // XP Bar
        DrawRectangle(popupX + 5, popupY + 36, boxWidth - 10, 3, BLACK);
        DrawRectangle(popupX + 5, popupY + 36,
                      static_cast<int>((boxWidth - 10) * xpPct), 3,
                      (Color){100, 200, 255, 255});
      }
    } else {
      selectedCitizenID = -1; // Citizen died, deselect
    }
  }
}

void WorldRenderer::DrawEntities() {
  ResourceManager &resourceManager = world.GetResourceManager();
  if (!resourceManager.IsLoaded())
    return;

  int tileSize = 10;
  const auto &entities = world.GetEntities();

  for (const auto &[eid, e] : entities) {
    if (e.type == EntityType::HumanUnarmed ||
        e.type == EntityType::HumanArmed) {
      // Determine State Index (0:Idle, 1:Walk, 2:Attack)
      int stateIdx = 0;
      if (e.state == EntityState::Walking)
        stateIdx = 1;
      else if (e.state == EntityState::Attack)
        stateIdx = 2;

      // Determine Direction Index (0:Down, 1:Right, 2:Left, 3:Up)
      int dirIdx = 0;
      if (e.facingDirection == 1)
        dirIdx = 1; // Right
      else if (e.facingDirection == -1)
        dirIdx = 2; // Left
      else if (e.facingDirection == 2)
        dirIdx = 3; // Up
      else
        dirIdx = 0; // Down

      // Determine Frame
      int frame = 0;
      Texture2D tex;

      if (e.type == EntityType::HumanUnarmed) {
        const auto &frames = resourceManager.texHumanUnarmed[stateIdx][dirIdx];
        if (!frames.empty()) {
          frame = e.currentFrame % frames.size();
          tex = frames[frame];
        } else {
          tex = {0};
        }
      } else {
        const auto &frames = resourceManager.texHumanArmed[stateIdx][dirIdx];
        if (!frames.empty()) {
          frame = e.currentFrame % frames.size();
          tex = frames[frame];
        } else {
          tex = {0};
        }
      }

      if (tex.id > 0) {
        float destW;
        float destH;
        float ageScale = 1.0f;

        // Apply visual scaling for children
        if (e.citizenID >= 0) {
          const Citizen *c = world.GetSimulation().GetCitizen(e.citizenID);
          if (c && !c->isAdult()) {
            ageScale = 0.5f + std::min(1.0f, c->age / 18.0f) * 0.5f;
          }
        }

        if (e.type == EntityType::HumanUnarmed ||
            e.type == EntityType::HumanArmed) {
          destW = tex.width * 0.22f * ageScale; // Adjusted
          destH = tex.height * 0.22f * ageScale;
        } else {
          destW = tex.width * 0.7f;
          destH = tex.height * 0.7f;
        }

        // Center at position
        float screenX = e.position.x * tileSize;
        float screenY = e.position.y * tileSize;

        Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
        Rectangle dest = {screenX, screenY, destW, destH};
        Vector2 origin = {destW / 2, destH * 0.8f}; // Pivot near feet

        DrawTexturePro(tex, src, dest, origin, 0.0f, WHITE);
      }
    } else if (e.type == EntityType::Boar) {
      // Boar specific logic
      const std::vector<Texture2D> *frames = nullptr;
      int framesPerDir = 1;

      if (e.state == EntityState::Idle) {
        frames = &resourceManager.texBoarIdle;
        framesPerDir = 4;
      } else if (e.state == EntityState::Walking) {
        frames = &resourceManager.texBoarWalk;
        framesPerDir = 6;
      } else if (e.state == EntityState::Attack) {
        frames = &resourceManager.texBoarAttack;
        framesPerDir = 5;
      } else if (e.state == EntityState::Hurt) {
        frames = &resourceManager.texBoarHurt;
        framesPerDir = 4;
      } else if (e.state == EntityState::Die) {
        frames = &resourceManager.texBoarDeath;
        framesPerDir = 6;
      } else {
        frames = &resourceManager.texBoarWalk; // Fallback
        framesPerDir = 6;
      }

      if (frames && !frames->empty()) {
        int dirIdx = 0; // Down, Right, Left, Up
        if (e.facingDirection == 1)
          dirIdx = 1;
        else if (e.facingDirection == -1)
          dirIdx = 2;
        else if (e.facingDirection == 2)
          dirIdx = 3;

        int frame = e.currentFrame % framesPerDir;
        int finalIdx = dirIdx * framesPerDir + frame;
        if (finalIdx >= frames->size())
          finalIdx = 0;

        Texture2D tex = (*frames)[finalIdx];
        if (tex.id > 0) {
          float destW = tex.width * 0.5f;
          float destH = tex.height * 0.5f;
          float screenX = e.position.x * tileSize;
          float screenY = e.position.y * tileSize;

          Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
          Rectangle dest = {screenX, screenY, destW, destH};
          Vector2 origin = {destW / 2.0f, destH * 0.8f};
          DrawTexturePro(tex, src, dest, origin, 0.0f, WHITE);
        }
      }
    } else if (e.type == EntityType::Slime) {
      // Slime logic - procedural rendering since sprite is missing
      DrawCircle(e.position.x * tileSize, e.position.y * tileSize, 5.0f * (tileSize/10.0f), GREEN);
    } else if (e.type == EntityType::Cow || e.type == EntityType::Chicken ||
               e.type == EntityType::Sheep || e.type == EntityType::Bull ||
               e.type == EntityType::Chicken2 || e.type == EntityType::Lamb ||
               e.type == EntityType::Pig || e.type == EntityType::Turkey) {
      // Animal Logic
      int baseIndex = 0;
      if (e.facingDirection == 1)
        baseIndex = 18; // Right (Row 3)
      else if (e.facingDirection == -1)
        baseIndex = 12; // Left (Row 2)
      else if (e.facingDirection == 2)
        baseIndex = 6; // Up (Row 1)
      else
        baseIndex = 0; // Down (Row 0)

      int frame = e.currentFrame % 6;
      int finalIndex = baseIndex + frame;

      Texture2D tex;
      if (e.type == EntityType::Cow)
        tex = resourceManager.texCow[finalIndex];
      else if (e.type == EntityType::Chicken)
        tex = resourceManager.texChicken[finalIndex];
      else if (e.type == EntityType::Sheep)
        tex = resourceManager.texSheep[finalIndex];
      else if (e.type == EntityType::Bull)
        tex = resourceManager.texBull[finalIndex];
      else if (e.type == EntityType::Chicken2)
        tex = resourceManager.texChicken2[finalIndex];
      else if (e.type == EntityType::Lamb)
        tex = resourceManager.texLamb[finalIndex];
      else if (e.type == EntityType::Pig)
        tex = resourceManager.texPig[finalIndex];
      else
        tex = resourceManager.texTurkey[finalIndex];

      if (tex.id > 0) {
        float scale =
            (e.type == EntityType::Chicken || e.type == EntityType::Chicken2)
                ? 0.4f
                : 0.5f;
        float destW = tex.width * scale;
        float destH = tex.height * scale;
        float screenX = e.position.x * tileSize;
        float screenY = e.position.y * tileSize;

        Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
        Rectangle dest = {screenX, screenY, destW, destH};
        Vector2 origin = {destW / 2, destH * 0.9f};
        DrawTexturePro(tex, src, dest, origin, 0.0f, WHITE);
      }
    } else {
      // Fallback
      DrawCircle((int)(e.position.x * tileSize), (int)(e.position.y * tileSize),
                 tileSize / 2, BLUE);
    }
  }
} // End DrawEntities

// ============================================================================
// WATER EFFECTS - Procedural rendering for waves, sparkles, and foam
// ============================================================================

bool WorldRenderer::IsWaterTile(TileType type) const {
  return type == TileType::DeepOcean || type == TileType::Ocean ||
         type == TileType::ShallowOcean;
}

void WorldRenderer::DrawWaterEffects(int tileX, int tileY, TileType type,
                                     int screenX, int screenY, int tileSize,
                                     float time) {
  // Get tile's unique seed for consistent randomness
  unsigned int seed = world.GetTile(tileX, tileY).variant;

  // 1. Wave overlay (subtle brightness variation)
  DrawWaterWaves(screenX, screenY, tileSize, time, type);

  // 2. Sparkles (random glints)
  DrawWaterSparkles(screenX, screenY, tileSize, time, seed);

  // 3. Foam on edges (where water meets land)
  DrawWaterFoam(tileX, tileY, screenX, screenY, tileSize, time);
}

void WorldRenderer::DrawWaterWaves(int screenX, int screenY, int tileSize,
                                   float time, TileType type) {
  // ==========================================================================
  // LIGHTWEIGHT TEXTURED WATER - Rectangle overlays (fast!)
  // ==========================================================================

  float worldX = (float)screenX;
  float worldY = (float)screenY;

  // Wave 1: Large slow diagonal wave
  float wave1 = sinf(worldX * 0.02f + worldY * 0.015f + time * 0.3f);
  if (wave1 > 0.4f) {
    unsigned char alpha = (unsigned char)((wave1 - 0.4f) / 0.6f * 35);
    Color highlight = {180, 220, 255, alpha};
    DrawRectangle(screenX, screenY, tileSize, tileSize, highlight);
  } else if (wave1 < -0.4f) {
    unsigned char alpha = (unsigned char)((-wave1 - 0.4f) / 0.6f * 25);
    Color shadow = {20, 50, 100, alpha};
    DrawRectangle(screenX, screenY, tileSize, tileSize, shadow);
  }

  // Wave 2: Smaller faster perpendicular wave (half tile strips)
  float wave2 = sinf(worldX * 0.04f - worldY * 0.03f + time * 0.5f);
  int halfTile = tileSize / 2;
  if (wave2 > 0.5f) {
    unsigned char alpha = (unsigned char)((wave2 - 0.5f) / 0.5f * 25);
    Color highlight = {200, 240, 255, alpha};
    if (((int)(worldX / halfTile) + (int)(worldY / halfTile)) % 2 == 0) {
      DrawRectangle(screenX, screenY, halfTile, halfTile, highlight);
    } else {
      DrawRectangle(screenX + halfTile, screenY + halfTile, halfTile, halfTile,
                    highlight);
    }
  }
}

void WorldRenderer::DrawWaterSparkles(int screenX, int screenY, int tileSize,
                                      float time, unsigned int seed) {
  // ==========================================================================
  // SLOW, SMOOTH SPARKLES - Fade in/out gently instead of blinking
  // ==========================================================================

  // Very slow time progression (sparkles last ~2-3 seconds)
  float slowTime = time * 0.3f;
  int sparklePhase = (int)(slowTime) % 20; // Cycle every 20 phases

  // Deterministic sparkle based on seed
  unsigned int sparkleHash = seed ^ (unsigned int)(sparklePhase * 1234);

  // Only ~10% of tiles have sparkles at any time
  if ((sparkleHash % 100) < 10) {
    // Sparkle position within tile
    int sparkleX = screenX + (int)((sparkleHash >> 8) % (tileSize - 2)) + 1;
    int sparkleY = screenY + (int)((sparkleHash >> 16) % (tileSize - 2)) + 1;

    // Smooth fade using fractional time
    float fadePhase = fmodf(slowTime, 1.0f); // 0 to 1
    float fade = sinf(fadePhase * 3.14159f); // Smooth bell curve: 0 -> 1 -> 0

    // Alpha based on fade (0 to 255)
    unsigned char alpha = (unsigned char)(200 * fade);

    if (alpha > 20) { // Only draw if visible
      Color sparkleColor = {255, 255, 255, alpha};
      // Small sparkle cross pattern (more natural than square)
      DrawPixel(sparkleX, sparkleY, sparkleColor);
      if (alpha > 100) {
        DrawPixel(sparkleX - 1, sparkleY, sparkleColor);
        DrawPixel(sparkleX + 1, sparkleY, sparkleColor);
        DrawPixel(sparkleX, sparkleY - 1, sparkleColor);
        DrawPixel(sparkleX, sparkleY + 1, sparkleColor);
      }
    }
  }
}

void WorldRenderer::DrawWaterFoam(int tileX, int tileY, int screenX,
                                  int screenY, int tileSize, float time) {
  int width = world.GetWidth();
  int height = world.GetHeight();

  // Check each neighbor for land
  bool hasLandNorth =
      (tileY > 0) && !IsWaterTile(world.GetTile(tileX, tileY - 1).type);
  bool hasLandSouth = (tileY < height - 1) &&
                      !IsWaterTile(world.GetTile(tileX, tileY + 1).type);
  bool hasLandEast =
      (tileX < width - 1) && !IsWaterTile(world.GetTile(tileX + 1, tileY).type);
  bool hasLandWest =
      (tileX > 0) && !IsWaterTile(world.GetTile(tileX - 1, tileY).type);

  // Also check diagonal corners for better corner handling
  bool hasLandNW = (tileY > 0 && tileX > 0) &&
                   !IsWaterTile(world.GetTile(tileX - 1, tileY - 1).type);
  bool hasLandNE = (tileY > 0 && tileX < width - 1) &&
                   !IsWaterTile(world.GetTile(tileX + 1, tileY - 1).type);
  bool hasLandSW = (tileY < height - 1 && tileX > 0) &&
                   !IsWaterTile(world.GetTile(tileX - 1, tileY + 1).type);
  bool hasLandSE = (tileY < height - 1 && tileX < width - 1) &&
                   !IsWaterTile(world.GetTile(tileX + 1, tileY + 1).type);

  // No foam if no land neighbors at all
  if (!hasLandNorth && !hasLandSouth && !hasLandEast && !hasLandWest &&
      !hasLandNW && !hasLandNE && !hasLandSW && !hasLandSE) {
    return;
  }

  // ==========================================================================
  // ORGANIC FOAM - Pixel-by-pixel with gradient fade from edges
  // ==========================================================================

  // Count how many cardinal directions have land
  int landSides = 0;
  if (hasLandNorth)
    landSides++;
  if (hasLandSouth)
    landSides++;
  if (hasLandEast)
    landSides++;
  if (hasLandWest)
    landSides++;

  // Slow wave animation
  float wavePhase = sinf(time * 0.4f);                 // Very slow
  float waveOffset = (wavePhase * 0.5f + 0.5f) * 2.0f; // 0 to 2

  // Maximum foam distance - MUCH smaller for surrounded tiles
  float maxFoamDist = 4.0f;
  if (landSides >= 4) {
    // Fully surrounded - minimal foam (just 2px)
    maxFoamDist = 2.0f;
    waveOffset = 0.5f; // Minimal wave movement
  } else if (landSides >= 3) {
    // 3 sides surrounded - reduce foam
    maxFoamDist = 3.0f;
  }

  // Main foam color (bright white-cyan)
  Color foamBright = {250, 255, 255, 230};
  Color foamMid = {220, 245, 255, 180};
  Color foamSoft = {190, 230, 250, 100};

  // Draw foam pixel by pixel for organic look
  for (int py = 0; py < tileSize; py++) {
    for (int px = 0; px < tileSize; px++) {
      // Calculate distance to each edge
      float distToNorth = (float)py;
      float distToSouth = (float)(tileSize - 1 - py);
      float distToWest = (float)px;
      float distToEast = (float)(tileSize - 1 - px);

      // Find minimum distance to any land edge
      float minDist = 999.0f;

      if (hasLandNorth && distToNorth < minDist)
        minDist = distToNorth;
      if (hasLandSouth && distToSouth < minDist)
        minDist = distToSouth;
      if (hasLandWest && distToWest < minDist)
        minDist = distToWest;
      if (hasLandEast && distToEast < minDist)
        minDist = distToEast;

      // Corner handling - distance to diagonal corners
      if (hasLandNW && !hasLandNorth && !hasLandWest) {
        float cornerDist = sqrtf((float)(px * px + py * py));
        if (cornerDist < minDist)
          minDist = cornerDist;
      }
      if (hasLandNE && !hasLandNorth && !hasLandEast) {
        float cornerDist =
            sqrtf((float)((tileSize - 1 - px) * (tileSize - 1 - px) + py * py));
        if (cornerDist < minDist)
          minDist = cornerDist;
      }
      if (hasLandSW && !hasLandSouth && !hasLandWest) {
        float cornerDist =
            sqrtf((float)(px * px + (tileSize - 1 - py) * (tileSize - 1 - py)));
        if (cornerDist < minDist)
          minDist = cornerDist;
      }
      if (hasLandSE && !hasLandSouth && !hasLandEast) {
        float cornerDist =
            sqrtf((float)((tileSize - 1 - px) * (tileSize - 1 - px) +
                          (tileSize - 1 - py) * (tileSize - 1 - py)));
        if (cornerDist < minDist)
          minDist = cornerDist;
      }

      // Skip if no land edge found or too far from any edge
      if (minDist > maxFoamDist)
        continue;

      // Apply wave offset to distance (foam moves in/out)
      float adjustedDist = minDist - waveOffset;

      // Draw foam based on distance
      if (adjustedDist < 0) {
        // Very close to edge - bright
        DrawPixel(screenX + px, screenY + py, foamBright);
      } else if (adjustedDist < 1.5f) {
        // Medium distance - mid tone
        DrawPixel(screenX + px, screenY + py, foamMid);
      } else if (adjustedDist < 3.0f) {
        // Far from edge - soft fade
        DrawPixel(screenX + px, screenY + py, foamSoft);
      }
    }
  }
}
