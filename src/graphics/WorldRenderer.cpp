#include "WorldRenderer.h"
#include "../resources/ResourceManager.h"
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
        case TileType::DeepOcean: {
          // Weighted: 80% solid color, 20% waves
          int waterProb = tileHash % 100;
          tex = (waterProb < 20) ? &resourceManager.texDeepOcean[0]
                                 : &resourceManager.texDeepOcean[1];
          break;
        }
        case TileType::Ocean: {
          int waterProb = tileHash % 100;
          tex = (waterProb < 20) ? &resourceManager.texOcean[0]
                                 : &resourceManager.texOcean[1];
          break;
        }
        case TileType::ShallowOcean: {
          int waterProb = tileHash % 100;
          tex = (waterProb < 20) ? &resourceManager.texShallowOcean[0]
                                 : &resourceManager.texShallowOcean[1];
          break;
        }
        case TileType::Grass: {
          // Weighted selection: grass1=20%, grass2=40%, grass3=40%
          int grassProb = tileHash % 100;
          if (grassProb < 20) {
            tex =
                &resourceManager.texGrass[0]; // 20% chance for grassprincipal1
          } else if (grassProb < 60) {
            tex =
                &resourceManager.texGrass[1]; // 40% chance for grassprincipal2
          } else {
            tex =
                &resourceManager.texGrass[2]; // 40% chance for grassprincipal3
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
          tex = &resourceManager.texSnow; // Use real snow texture
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

  // PASS 2: Draw Trees (Objects)
  if (resourceManager.IsLoaded()) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        Tile &tile = world.GetTile(x, y);
        if (tile.decoration != DecorationType::None)
          continue; // Skip procedural if manual set
        unsigned int seed = tile.variant ^ world.GetSeed() ^ 9284387;

        Texture2D *treeTex = nullptr;
        bool hasTree = false;

        // Forest Trees - Use mix of tree types for variety
        if (tile.type == TileType::Forest) {
          hasTree = (seed % 100) < 30;
          if (hasTree) {
            int treeType = (seed / 100) % 3; // 0=Fruit, 1=Normal, 2=Moss
            int treeIdx = (seed / 1000) % 3; // Variant within each type

            switch (treeType) {
            case 0:
              treeTex =
                  &resourceManager
                       .texFruitTrees[treeIdx %
                                      ResourceManager::NUM_FRUIT_TREE_TYPES];
              break;
            case 1:
              treeTex =
                  &resourceManager
                       .texNormalTrees[treeIdx %
                                       ResourceManager::NUM_NORMAL_TREE_TYPES];
              break;
            case 2:
              treeTex =
                  &resourceManager
                       .texMossTrees[treeIdx %
                                     ResourceManager::NUM_MOSS_TREE_TYPES];
              break;
            }
          }
        }
        // Snow Trees
        else if (tile.type == TileType::Snow) {
          hasTree = (seed % 100) < 15; // Lower density in snow
          if (hasTree) {
            // 10% chance for xmas tree, 90% normal snow tree
            if ((seed % 1000) < 100) {
              treeTex =
                  &resourceManager
                       .texXmasTrees[(seed / 100) %
                                     ResourceManager::NUM_XMAS_TREE_TYPES];
            } else {
              treeTex =
                  &resourceManager
                       .texSnowTrees[(seed / 10) %
                                     ResourceManager::NUM_SNOW_TREE_TYPES];
            }
          }
        }
        // Desert Trees (Palms)
        else if (tile.type == TileType::DesertSand) {
          hasTree = (seed % 100) < 2; // Very rare (oasis feel)
          if (hasTree) {
            treeTex =
                &resourceManager
                     .texPalmTrees[seed % ResourceManager::NUM_PALM_TREE_TYPES];
          }
        }

        if (hasTree && treeTex && treeTex->id > 0) {
          Rectangle treeSrc = {0, 0, (float)treeTex->width,
                               (float)treeTex->height};
          float treeScale = 3.0f;
          float treeW = tileSize * treeScale;
          float treeH =
              treeW * ((float)treeTex->height / (float)treeTex->width);
          float offX = ((seed % 20) - 10) / 10.0f * (tileSize * 0.3f);
          float offY = (((seed / 20) % 20) - 10) / 10.0f * (tileSize * 0.3f);

          Rectangle treeDest = {(float)(x * tileSize + tileSize / 2 + offX),
                                (float)(y * tileSize + tileSize / 2 + offY),
                                treeW, treeH};
          Vector2 treeOrigin = {treeW / 2, treeH * 0.85f};
          DrawTexturePro(*treeTex, treeSrc, treeDest, treeOrigin, 0.0f, WHITE);
        }
      }
    }
  }

  // PASS 3: Draw Mountain Rocks (Decorations)
  if (resourceManager.IsLoaded()) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        Tile &tile = world.GetTile(x, y);
        if (tile.decoration != DecorationType::None)
          continue;
        if (tile.type == TileType::Mountain) {
          unsigned int seed = tile.variant ^ world.GetSeed() ^ 1234567;
          bool hasRock = (seed % 100) < 20; // 20% chance as requested
          if (hasRock) {
            Texture2D &rockTex = resourceManager.texMountainRocks;
            Rectangle rockSrc = {0, 0, (float)rockTex.width,
                                 (float)rockTex.height};
            float rockScale = 1.0f;
            float rockW = tileSize * rockScale;
            float rockH =
                rockW * ((float)rockTex.height / (float)rockTex.width);

            float offX = ((seed % 10) - 5) / 10.0f * (tileSize * 0.2f);
            float offY = (((seed / 10) % 10) - 5) / 10.0f * (tileSize * 0.2f);

            Rectangle rockDest = {(float)(x * tileSize + tileSize / 2 + offX),
                                  (float)(y * tileSize + tileSize / 2 + offY),
                                  rockW, rockH};
            Vector2 rockOrigin = {rockW / 2, rockH / 2};
            DrawTexturePro(rockTex, rockSrc, rockDest, rockOrigin, 0.0f,
                           LIGHTGRAY); // Lighter tint for contrast
          }
        }
      }
    }
  }

  // PASS 3.5: Graminhas (Forest decoration, only on floresta1 tiles)
  if (resourceManager.IsLoaded() && resourceManager.texGraminhas.id > 0) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        Tile &tile = world.GetTile(x, y);
        if (tile.type == TileType::Forest) {
          unsigned int tileHash = tile.variant;
          int forestProb = tileHash % 100;
          // Only spawn graminhas on floresta1 tiles (first 60%)
          if (forestProb < 60) {
            unsigned int seed = tile.variant ^ 0x444 ^ world.GetSeed() ^ 66666;

            // Skip if this tile has a tree (using same tree seed logic)
            unsigned int treeSeed = tile.variant ^ world.GetSeed() ^ 9284387;
            bool hasTree = (treeSeed % 100) < 30;
            if (hasTree)
              continue; // Don't draw graminhas under trees

            if ((seed % 100) < 25) { // 25% chance on floresta1
              float w = tileSize * 0.8f;
              float h = w * ((float)resourceManager.texGraminhas.height /
                             (float)resourceManager.texGraminhas.width);
              float offX = ((seed % 20) - 10) / 10.0f * (tileSize * 0.2f);
              float offY =
                  (((seed / 20) % 20) - 10) / 10.0f * (tileSize * 0.2f);

              Rectangle src = {0, 0, (float)resourceManager.texGraminhas.width,
                               (float)resourceManager.texGraminhas.height};
              Rectangle dest = {(float)(x * tileSize + tileSize / 2 + offX),
                                (float)(y * tileSize + tileSize / 2 + offY), w,
                                h};
              Vector2 origin = {w / 2, h / 2};
              DrawTexturePro(resourceManager.texGraminhas, src, dest, origin,
                             0.0f, WHITE);
            }
          }
        }
      }
    }
  }

  // PASS 4: Bushes & Cacti
  if (resourceManager.IsLoaded()) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        Tile &tile = world.GetTile(x, y);
        if (tile.decoration != DecorationType::None)
          continue;
        unsigned int seed = tile.variant ^ 0x876 ^ world.GetSeed() ^ 55555;
        Texture2D *bushTex = nullptr;

        // Normal Bushes (Forest/Grass)
        if (tile.type == TileType::Forest || tile.type == TileType::Grass) {
          if ((seed % 100) < 20) { // 20% chance for better visibility
            bushTex = &resourceManager
                           .texBushes[seed % ResourceManager::NUM_BUSH_TYPES];
          }
        }
        // Snow Bushes
        else if (tile.type == TileType::Snow) {
          if ((seed % 100) < 5) { // 5% chance
            bushTex = &resourceManager
                           .texSnowBushes[seed %
                                          ResourceManager::NUM_SNOW_BUSH_TYPES];
          }
        }
        // Desert Cacti
        else if (tile.type == TileType::DesertSand) {
          if ((seed % 100) < 3) { // 3% chance
            bushTex = &resourceManager
                           .texCactus[seed % ResourceManager::NUM_CACTUS_TYPES];
          }
        }

        if (bushTex && bushTex->id > 0) {
          Rectangle src = {0, 0, (float)bushTex->width, (float)bushTex->height};
          float scale = 1.0f;
          float w = tileSize * scale;
          float h = w * ((float)bushTex->height / (float)bushTex->width);

          // Random offset
          float offX = ((seed % 20) - 10) / 10.0f * (tileSize * 0.2f);
          float offY = (((seed / 20) % 20) - 10) / 10.0f * (tileSize * 0.2f);

          Rectangle dest = {(float)(x * tileSize + tileSize / 2 + offX),
                            (float)(y * tileSize + tileSize / 2 + offY), w, h};
          Vector2 origin = {w / 2, h * 0.8f}; // Pivot at bottom center
          DrawTexturePro(*bushTex, src, dest, origin, 0.0f, WHITE);
        }
      }
    }
  }

  // PASS 5: Crystals (Rare, Mountains)
  if (resourceManager.IsLoaded()) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        Tile &tile = world.GetTile(x, y);
        if (tile.decoration != DecorationType::None)
          continue;
        if (tile.type == TileType::Mountain &&
            tile.height > 0.65f) { // High mountains
          unsigned int seed = tile.variant ^ 0x111 ^ world.GetSeed() ^ 33333;
          if ((seed % 1000) < 15) { // 1.5% chance
            Texture2D *crystalTex = nullptr;
            int type = seed % 3;
            if (type == 0)
              crystalTex =
                  &resourceManager
                       .texCrystalsBlue[seed %
                                        ResourceManager::NUM_CRYSTAL_VARIANTS];
            else if (type == 1)
              crystalTex =
                  &resourceManager
                       .texCrystalsGreen[seed %
                                         ResourceManager::NUM_CRYSTAL_VARIANTS];
            else
              crystalTex =
                  &resourceManager
                       .texCrystalsRed[seed %
                                       ResourceManager::NUM_CRYSTAL_VARIANTS];

            if (crystalTex && crystalTex->id > 0) {
              Rectangle src = {0, 0, (float)crystalTex->width,
                               (float)crystalTex->height};
              float scale = 1.0f;
              float w = tileSize * scale;
              float h =
                  w * ((float)crystalTex->height / (float)crystalTex->width);

              Rectangle dest = {(float)(x * tileSize + tileSize / 2),
                                (float)(y * tileSize + tileSize / 2), w, h};
              Vector2 origin = {w / 2, h / 2};
              DrawTexturePro(*crystalTex, src, dest, origin, 0.0f, WHITE);
            }
          }
        }
      }
    }
  }

  // PASS 6: Big Rocks (Scattered)
  if (resourceManager.IsLoaded()) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        Tile &tile = world.GetTile(x, y);
        if (tile.decoration != DecorationType::None)
          continue;
        unsigned int seed = tile.variant ^ 0x999 ^ world.GetSeed() ^ 77777;
        Texture2D *rockTex = nullptr;

        // Skip tiles that have trees (using same seed logic as Pass 2)
        unsigned int treeSeed = tile.variant ^ world.GetSeed() ^ 9284387;
        bool hasTree = false;
        if (tile.type == TileType::Forest) {
          hasTree = (treeSeed % 100) < 30;
        } else if (tile.type == TileType::Snow) {
          hasTree = (treeSeed % 100) < 15;
        } else if (tile.type == TileType::DesertSand) {
          hasTree = (treeSeed % 100) < 2;
        }

        if (hasTree)
          continue; // Skip this tile if it has a tree

        if (tile.type == TileType::Grass || tile.type == TileType::Forest) {
          if ((seed % 1000) < 5) { // 0.5% chance
            rockTex =
                &resourceManager
                     .texBigRocks[seed % ResourceManager::NUM_BIG_ROCK_TYPES];
          }
        } else if (tile.type == TileType::Snow) {
          if ((seed % 1000) < 5) {
            rockTex = &resourceManager.texSnowRockShadows
                           [seed % ResourceManager::NUM_SNOW_ROCK_SHADOWS];
          }
        } else if (tile.type == TileType::DesertSand) {
          if ((seed % 1000) < 5) {
            rockTex = &resourceManager.texDesertRockShadows
                           [seed % ResourceManager::NUM_DESERT_ROCK_SHADOWS];
          }
        }

        if (rockTex && rockTex->id > 0) {
          Rectangle src = {0, 0, (float)rockTex->width, (float)rockTex->height};
          float scale = 1.0f; // Reduced to prevent overlap
          float w = tileSize * scale;
          float h = w * ((float)rockTex->height / (float)rockTex->width);

          Rectangle dest = {(float)(x * tileSize + tileSize / 2),
                            (float)(y * tileSize + tileSize / 2), w, h};
          Vector2 origin = {w / 2, h / 2};
          DrawTexturePro(*rockTex, src, dest, origin, 0.0f, WHITE);
        }
      }
    }
  }

  // PASS 7: Manual Decorations
  if (resourceManager.IsLoaded()) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        Tile &tile = world.GetTile(x, y);
        if (tile.decoration == DecorationType::None)
          continue;

        Texture2D *decTex = nullptr;
        int v = tile.decorationVariant;
        float scale = 1.0f;

        switch (tile.decoration) {
        case DecorationType::Tree:
          decTex =
              &resourceManager
                   .texNormalTrees[v % ResourceManager::NUM_NORMAL_TREE_TYPES];
          scale = 3.0f;
          break;
        case DecorationType::PineTree: // Map to Snow/Xmas
          decTex = &resourceManager
                        .texSnowTrees[v % ResourceManager::NUM_SNOW_TREE_TYPES];
          scale = 3.0f;
          break;
        case DecorationType::PalmTree:
          decTex = &resourceManager
                        .texPalmTrees[v % ResourceManager::NUM_PALM_TREE_TYPES];
          scale = 3.0f;
          break;
        case DecorationType::Bush:
          decTex =
              &resourceManager.texBushes[v % ResourceManager::NUM_BUSH_TYPES];
          scale = 1.0f;
          break;
        case DecorationType::Rock: // User wants "pedrinhas" removed from UI
                                   // but if kept, use small rock tex
          decTex =
              &resourceManager
                   .texSmallRocks[v % ResourceManager::NUM_SMALL_ROCK_TYPES];
          scale = 1.0f;
          break; // Use new small rock
        case DecorationType::SmallRock:
          decTex =
              &resourceManager
                   .texSmallRocks[v % ResourceManager::NUM_SMALL_ROCK_TYPES];
          scale = 1.0f;
          break;
        case DecorationType::MediumRock:
          // Use MediumRock textures (Rock3)
          decTex =
              &resourceManager
                   .texMediumRocks[v % ResourceManager::NUM_MEDIUM_ROCK_TYPES];
          scale = 1.0f;
          break;
        case DecorationType::Flower:
          decTex = &resourceManager
                        .texFlowers[v % ResourceManager::NUM_FLOWER_TYPES];
          scale = 1.0f;
          break;
        case DecorationType::Mushroom:
          decTex = &resourceManager
                        .texMushrooms[v % ResourceManager::NUM_MUSHROOM_TYPES];
          scale = 0.8f;
          break;
        case DecorationType::BigRock:
          // Use BigRock (Rock1)
          decTex = &resourceManager
                        .texBigRocks[v % ResourceManager::NUM_BIG_ROCK_TYPES];
          scale = 1.0f;
          break;

        default:
          break;
        }

        if (decTex && decTex->id > 0) {
          Rectangle src = {0, 0, (float)decTex->width, (float)decTex->height};
          float w = tileSize * scale;
          float h = w * ((float)decTex->height / (float)decTex->width);
          // Centered bottom
          Rectangle dest = {(float)(x * tileSize + tileSize / 2),
                            (float)(y * tileSize + tileSize / 2), w, h};
          Vector2 origin = {w / 2, h * 0.9f};
          DrawTexturePro(*decTex, src, dest, origin, 0.0f, WHITE);
        }
      }
    }
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

  // PASS 8: Entities
  DrawEntities();
}

void WorldRenderer::DrawEntities() {
  ResourceManager &resourceManager = world.GetResourceManager();
  if (!resourceManager.IsLoaded())
    return;

  int tileSize = 10;
  const std::vector<Entity> &entities = world.GetEntities();

  for (const Entity &e : entities) {
    if (e.type == EntityType::Human) {
      // Human Drawing with 16 Sprites
      // Mapping:
      // 00-03: Down (Dir 0)
      // 04-07: Right (Dir 1)
      // 08-11: Left (Dir -1)
      // 12-15: Up (Dir 2)

      int baseIndex = 0; // Default Down
      if (e.facingDirection == 1)
        baseIndex = 4; // Right
      else if (e.facingDirection == -1)
        baseIndex = 8; // Left
      else if (e.facingDirection == 2)
        baseIndex = 12; // Up

      int frame = e.currentFrame % 4;
      int finalIndex = baseIndex + frame;

      // Access public array directly (assuming it's public)
      Texture2D tex = resourceManager.texHuman[finalIndex];

      if (tex.id > 0) {
        // Draw full texture (no sprite sheet slicing needed as they are
        // individual files) Adjust sizing: Files are likely small. Let's assume
        // we maintain the same visual size on screen. Previous sheet logic:
        // frame 64x96, scale 0.5 -> 32x48. If individual files are same
        // resolution (64x96 per file), use same sizing.

        float destW = tex.width * 0.5f;
        float destH = tex.height * 0.5f;

        // Center feet at position
        float screenX = e.position.x * tileSize;
        float screenY = e.position.y * tileSize;

        // Source is full image
        Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
        Rectangle dest = {screenX, screenY, destW, destH};
        Vector2 origin = {destW / 2, destH * 0.9f}; // Pivot at feet

        DrawTexturePro(tex, src, dest, origin, 0.0f, WHITE);
      } else {
        // Fallback
        DrawCircle((int)(e.position.x * tileSize),
                   (int)(e.position.y * tileSize), tileSize / 2, RED);
      }
    } else {
      // Other entities
      DrawCircle((int)(e.position.x * tileSize), (int)(e.position.y * tileSize),
                 tileSize / 2, BLUE);
    }
  }
}
