#include "WorldRenderer.h"
#include "../resources/ResourceManager.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>

WorldRenderer::WorldRenderer(World &world) : world(world) {}

void WorldRenderer::Draw() {
  int tileSize = 10;
  int width = world.GetWidth();
  int height = world.GetHeight();
  ResourceManager &resourceManager = world.GetResourceManager();

  // PASS 1: Draw Terrain and Shadows
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
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
          // Simple grass rendering with 3 variants
          int grassProb = tileHash % 100;
          if (grassProb < 20) {
            tex = &resourceManager.texGrass[0];
          } else if (grassProb < 60) {
            tex = &resourceManager.texGrass[1];
          } else {
            tex = &resourceManager.texGrass[2];
          }
          break;
        }
        case TileType::Forest: {
          // Forest uses dedicated textures: floresta1 (60%), floresta2 (40%)
          int forestProb = tileHash % 100;
          if (forestProb < 60) {
            tex = &resourceManager
                       .texForest[0]; // 60% floresta1 (can have graminhas)
          } else {
            tex = &resourceManager.texForest[1]; // 40% floresta2
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
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        Tile &tile = world.GetTile(x, y);

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
          scale = 3.0f;
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
          scale = 3.0f;
          break;
        case DecorationType::PalmTree:
          tex = &resourceManager
                     .texPalmTrees[v % ResourceManager::NUM_PALM_TREE_TYPES];
          scale = 3.0f;
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
        case DecorationType::Rock: // Mountain Rocks
          tex = &resourceManager.texMountainDecorations
                     [v % ResourceManager::NUM_MOUNTAIN_DECORATIONS];
          if ((v % ResourceManager::NUM_MOUNTAIN_DECORATIONS) == 0)
            scale = 2.0f; // User requested "Rock(decoration).png" (Index 0) to
                          // be bigger
          else
            scale = 1.0f;
          break;
        case DecorationType::BigRock:
          // Logic from Pass 6: Check shading based on biome
          if (tile.type == TileType::Snow &&
              ResourceManager::NUM_SNOW_ROCK_SHADOWS > 0)
            tex = &resourceManager.texSnowRockShadows
                       [v % ResourceManager::NUM_SNOW_ROCK_SHADOWS];
          else if (tile.type == TileType::DesertSand &&
                   ResourceManager::NUM_DESERT_ROCK_SHADOWS > 0)
            tex = &resourceManager.texDesertRockShadows
                       [v % ResourceManager::NUM_DESERT_ROCK_SHADOWS];
          else
            tex = &resourceManager
                       .texBigRocks[v % ResourceManager::NUM_BIG_ROCK_TYPES];
          scale = 1.0f;
          break;
        case DecorationType::SmallRock:
          tex = &resourceManager
                     .texSmallRocks[v % ResourceManager::NUM_SMALL_ROCK_TYPES];
          scale = 1.0f;
          break;
        case DecorationType::MediumRock:
          tex =
              &resourceManager
                   .texMediumRocks[v % ResourceManager::NUM_MEDIUM_ROCK_TYPES];
          scale = 1.0f;
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

          // Sort Y is the visual bottom of the object
          float sortY = dest.y + h / 2; // Approximate center/bottom
          if (tile.decoration == DecorationType::Tree ||
              tile.decoration == DecorationType::PineTree ||
              tile.decoration == DecorationType::PalmTree ||
              (tile.decoration == DecorationType::DesertPlant &&
               (tile.decorationVariant % 3) != 0)) // Cactus variants
            sortY = dest.y + h * 0.85f;

          items.push_back({*tex, src, dest, origin, tColor, sortY});
        }
      }
    }
  }

  // Entities
  const std::vector<Entity> &entities = world.GetEntities();
  for (const Entity &e : entities) {
    if (!resourceManager.IsLoaded())
      continue;

    Texture2D tex;
    // === HUMAN DRAWING ===
    if (e.type == EntityType::HumanUnarmed ||
        e.type == EntityType::HumanArmed) {
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

      if (e.state == EntityState::Swim) {
        if (e.type == EntityType::HumanUnarmed)
          tex = resourceManager.texHumanUnarmedSwim[dirIdx][frame];
        else
          tex = resourceManager.texHumanArmedSwim[dirIdx][frame];
      } else if (e.state == EntityState::Die) {
        tex = resourceManager.texHumanDeath[frame];
      } else if (e.state == EntityState::Block &&
                 e.type == EntityType::HumanArmed) {
        tex = resourceManager.texHumanArmedBlock[dirIdx];
      } else {
        // Standard States (Idle, Walk, Attack)
        int stateIdx = 0;
        if (e.state == EntityState::Walking)
          stateIdx = 1;
        else if (e.state == EntityState::Attack)
          stateIdx = 2;

        if (e.type == EntityType::HumanUnarmed)
          tex = resourceManager.texHumanUnarmed[stateIdx][dirIdx][frame];
        else
          tex = resourceManager.texHumanArmed[stateIdx][dirIdx][frame];
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
      float destW = tex.width * 0.5f;
      float destH = tex.height * 0.5f;
      // Maintain scale from original

      Rectangle src = {0, 0, (float)tex.width, (float)tex.height};

      // Flip Boar if facing Left (Direction 2)
      if (e.type == EntityType::Boar && e.facingDirection == 2) {
        src.width = -src.width;
      }
      Rectangle dest = {e.position.x * tileSize, e.position.y * tileSize, destW,
                        destH};
      Vector2 origin = {destW / 2, destH * 0.9f};

      items.push_back({tex, src, dest, origin, WHITE, dest.y + destH * 0.9f});
    } else {
      // Fallback circle drawn immediately (rare)
      DrawCircle((int)(e.position.x * tileSize), (int)(e.position.y * tileSize),
                 tileSize / 2, RED);
    }
  }

  // SORT
  std::sort(items.begin(), items.end(),
            [](const RenderItem &a, const RenderItem &b) {
              return a.sortY < b.sortY;
            });

  // DRAW
  for (const auto &item : items) {
    DrawTexturePro(item.texture, item.src, item.dest, item.origin, 0.0f,
                   item.tint);
  }

  // Draw Visual Boundaries
  int padding = 10;
  float startX = padding * tileSize;
  float startY = padding * tileSize;
  float endX = (width - padding) * tileSize;
  float endY = (height - padding) * tileSize;
  Color boundaryColor = (Color){200, 200, 200, 150};
  float dashLen = 20.0f;
  float gapLen = 10.0f;

  for (float py = startY; py < endY; py += dashLen + gapLen)
    DrawRectangle((int)endX, (int)py, 4, (int)std::min(dashLen, endY - py),
                  boundaryColor);
}

void WorldRenderer::DrawEntities() {
  ResourceManager &resourceManager = world.GetResourceManager();
  if (!resourceManager.IsLoaded())
    return;

  int tileSize = 10;
  const std::vector<Entity> &entities = world.GetEntities();

  for (const Entity &e : entities) {
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

      // Determine Frame (0-3)
      int frame = e.currentFrame % 4;

      Texture2D tex;
      if (e.type == EntityType::HumanUnarmed) {
        tex = resourceManager.texHumanUnarmed[stateIdx][dirIdx][frame];
      } else {
        tex = resourceManager.texHumanArmed[stateIdx][dirIdx][frame];
      }

      if (tex.id > 0) {
        float destW = tex.width * 0.7f; // Scale
        float destH = tex.height * 0.7f;

        // Center at position
        float screenX = e.position.x * tileSize;
        float screenY = e.position.y * tileSize;

        Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
        Rectangle dest = {screenX, screenY, destW, destH};
        Vector2 origin = {destW / 2, destH * 0.8f}; // Pivot near feet

        DrawTexturePro(tex, src, dest, origin, 0.0f, WHITE);
      }
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
