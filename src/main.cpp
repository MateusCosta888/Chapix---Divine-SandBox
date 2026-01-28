#include "graphics/WorldRenderer.h"
#include "raylib.h"
#include "raymath.h"
#include "world/World.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// UI Constants
const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;
const int TOOLBAR_HEIGHT = 100;
const int TAB_HEIGHT = 30;

// Enums
enum class UIState { Terrain, Nature, Rocks, Settings };

enum class BrushSize {
  Single = 0, // Special case for exact 1x1 painting
  S = 1,
  M = 3,
  L = 5,
  XL = 9
};

// UI Structure
struct GameUI {
  Texture2D texButton;
  Texture2D texPanel;
  Texture2D texTab;
  Texture2D texCursor;
  bool showBrushPopup = false;
  bool popupJustOpened = false; // logic fix

  UIState currentTab = UIState::Terrain;
  BrushSize currentBrushSize = BrushSize::S;

  // Selection state
  int selectedToolIndex = 0;

  void Load() {
    texButton = LoadTexture("assets/Pixel Buttom.png");
    if (texButton.id == 0)
      texButton = LoadTexture("assets/Pixel Button.png");

    texPanel =
        LoadTexture("assets/decorations/20240713dragonFilledFrame-Sheet.png");
    texTab = LoadTexture("assets/decorations/20240707dragonTabA-Sheet.png");
    texCursor = LoadTexture("assets/cursor.png");

    SetTextureFilter(texButton, TEXTURE_FILTER_POINT);
    SetTextureFilter(texPanel, TEXTURE_FILTER_POINT);
    SetTextureFilter(texTab, TEXTURE_FILTER_POINT);
  }

  void Unload() {
    UnloadTexture(texButton);
    UnloadTexture(texPanel);
    UnloadTexture(texTab);
    UnloadTexture(texCursor);
  }
};

int main(int argc, char *argv[]) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Genesis 2D - God Simulator");
  SetTargetFPS(60);

  uint32_t seed = 0;
  if (argc > 1 && std::string(argv[1]) == "--seed") {
    seed = std::stoul(argv[2]);
  } else {
    seed = (uint32_t)GetTime();
  }

  World world(128, 128, seed);
  world.Generate();
  world.LoadTextures();

  WorldRenderer worldRenderer(world);

  GameUI ui;
  ui.Load();
  HideCursor();

  Camera2D camera = {0};
  camera.zoom = 2.0f;
  camera.offset = {SCREEN_WIDTH / 2.0f,
                   (SCREEN_HEIGHT - TOOLBAR_HEIGHT) / 2.0f};
  camera.target = {world.GetWidth() * 5.0f, world.GetHeight() * 5.0f};

  while (!WindowShouldClose()) {
    ui.popupJustOpened = false;

    Vector2 mousePos = GetMousePosition();
    bool isPointerOnUI =
        mousePos.y > (SCREEN_HEIGHT - TOOLBAR_HEIGHT - TAB_HEIGHT);

    // Camera Controls
    if (!isPointerOnUI && !ui.showBrushPopup) {
      float wheel = GetMouseWheelMove();
      if (wheel != 0) {
        Vector2 mouseWorldBefore = GetScreenToWorld2D(mousePos, camera);
        camera.zoom = Clamp(camera.zoom + wheel * 0.5f, 0.5f, 10.0f);
        Vector2 mouseWorldAfter = GetScreenToWorld2D(mousePos, camera);
        camera.target = Vector2Add(
            camera.target, Vector2Subtract(mouseWorldBefore, mouseWorldAfter));
      }
      if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = Vector2Scale(GetMouseDelta(), -1.0f / camera.zoom);
        camera.target = Vector2Add(camera.target, delta);
      }
    }

    // World Interaction (Painting)
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !isPointerOnUI &&
        !ui.showBrushPopup && ui.currentTab != UIState::Settings) {
      Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
      int tx = (int)(worldPos.x / 10);
      int ty = (int)(worldPos.y / 10);

      // Brush Logic
      int size = (ui.currentBrushSize == BrushSize::Single)
                     ? 1
                     : (int)ui.currentBrushSize;
      int start = -size / 2;
      int end = size / 2 + size % 2;
      // Special case for single/even sizes logic if needed, but centering on
      // tile usually implies odd sizes
      if (size == 1) {
        start = 0;
        end = 0;
      } // Exact 1 tile
      else {
        start = -size / 2;
        end = size / 2;
      } // Standard centering

      int padding = 10;

      for (int dy = start; dy <= end; dy++) {
        for (int dx = start; dx <= end; dx++) {
          int nx = tx + dx;
          int ny = ty + dy;

          if (nx < padding || nx >= world.GetWidth() - padding ||
              ny < padding || ny >= world.GetHeight() - padding)
            continue;

          // Tab Logic
          if (ui.currentTab == UIState::Terrain) {
            TileType types[] = {TileType::DeepOcean,    TileType::Ocean,
                                TileType::ShallowOcean, TileType::Sand,
                                TileType::Grass,        TileType::Forest,
                                TileType::Mountain,     TileType::Snow};
            if (ui.selectedToolIndex < 8) {
              // Check repetition
              Tile &t = world.GetTile(nx, ny);
              if (t.type != types[ui.selectedToolIndex]) {
                world.SetTileType(nx, ny, types[ui.selectedToolIndex]);
              }
            } else {
              // Eraser
              Tile &t = world.GetTile(nx, ny);
              if (t.type != TileType::Grass ||
                  t.decoration != DecorationType::None) {
                world.SetTileType(nx, ny, TileType::Grass);
                world.SetTileDecoration(nx, ny, DecorationType::None);
              }
            }
          } else if (ui.currentTab == UIState::Nature) {
            DecorationType decs[] = {
                DecorationType::Tree,     DecorationType::PineTree,
                DecorationType::PalmTree, DecorationType::Bush,
                DecorationType::Flower,   DecorationType::Mushroom};
            if (ui.selectedToolIndex < 6) {
              Tile &t = world.GetTile(nx, ny);
              if (t.decoration != decs[ui.selectedToolIndex]) {
                world.SetTileDecoration(nx, ny, decs[ui.selectedToolIndex]);
              }
            } else {
              // Eraser
              Tile &t = world.GetTile(nx, ny);
              if (t.decoration != DecorationType::None)
                world.SetTileDecoration(nx, ny, DecorationType::None);
            }
          } else if (ui.currentTab == UIState::Rocks) {
            DecorationType decs[] = {DecorationType::Rock,
                                     DecorationType::BigRock};
            if (ui.selectedToolIndex < 2) {
              Tile &t = world.GetTile(nx, ny);
              if (t.decoration != decs[ui.selectedToolIndex])
                world.SetTileDecoration(nx, ny, decs[ui.selectedToolIndex]);
            } else {
              // Eraser
              Tile &t = world.GetTile(nx, ny);
              if (t.decoration != DecorationType::None)
                world.SetTileDecoration(nx, ny, DecorationType::None);
            }
          }
        }
      }
    }

    world.UpdateAnimation(GetFrameTime());
    if (IsKeyPressed(KEY_R)) {
      world.Reset((uint32_t)GetTime());
      world.Generate();
    }

    // --- DRAW ---
    BeginDrawing();
    ClearBackground(GetColor(0x1a1a2eFF));

    BeginMode2D(camera);
    worldRenderer.Draw();

    // Brush Preview
    if (!isPointerOnUI && !ui.showBrushPopup) {
      Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
      int tx = (int)(worldPos.x / 10);
      int ty = (int)(worldPos.y / 10);
      int size = (ui.currentBrushSize == BrushSize::Single)
                     ? 1
                     : (int)ui.currentBrushSize;
      float rectSize = size * 10.0f;
      float offX = (size == 1) ? 0 : -(size / 2) * 10.0f;
      float offY = (size == 1) ? 0 : -(size / 2) * 10.0f;

      DrawRectangleLinesEx(
          {tx * 10.0f + offX, ty * 10.0f + offY, rectSize, rectSize},
          1 / camera.zoom, WHITE);
    }

    EndMode2D();

    // UI Render
    Rectangle tabArea = {0, (float)SCREEN_HEIGHT - TOOLBAR_HEIGHT - TAB_HEIGHT,
                         (float)SCREEN_WIDTH, (float)TAB_HEIGHT};
    DrawRectangleRec(tabArea, GetColor(0x16213eFF));

    const char *tabNames[] = {"Terrains", "Nature", "Rocks", "Settings"};
    for (int i = 0; i < 4; i++) {
      float tabW = 150;
      Rectangle tabRect = {i * tabW, tabArea.y, tabW, tabArea.height};
      bool isHover = CheckCollisionPointRec(mousePos, tabRect);
      bool isActive = ((int)ui.currentTab == i);

      Color tabColor =
          isActive ? GetColor(0x0f3460FF)
                   : (isHover ? GetColor(0x1a1a2eFF) : GetColor(0x16213eFF));
      DrawRectangleRec(tabRect, tabColor);
      DrawRectangleLinesEx(tabRect, 1, GetColor(0x0f3460FF));
      DrawText(tabNames[i], (int)tabRect.x + 40, (int)tabRect.y + 8, 20,
               isActive ? WHITE : LIGHTGRAY);

      if (isHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
          !ui.showBrushPopup) {
        ui.currentTab = (UIState)i;
        ui.selectedToolIndex = 0;
      }
    }

    Rectangle toolbarArea = {0, (float)SCREEN_HEIGHT - TOOLBAR_HEIGHT,
                             (float)SCREEN_WIDTH, (float)TOOLBAR_HEIGHT};
    DrawRectangleRec(toolbarArea, GetColor(0x0f3460FF));

    float startX = 20;
    float startY = toolbarArea.y + 20;
    float btnSize = 64;
    float gap = 12;

    if (ui.currentTab == UIState::Settings) {
      DrawText("Settings Panel", (int)startX, (int)startY, 30, WHITE);
      DrawText("Press 'R' to Regenerate World", (int)startX, (int)startY + 40,
               20, LIGHTGRAY);
    } else {
      // Brush Button
      Rectangle brushBtnMsg = {SCREEN_WIDTH - 120.0f, startY, btnSize, btnSize};
      DrawRectangleRec(brushBtnMsg, DARKGRAY);
      DrawRectangleLinesEx(brushBtnMsg, 2, WHITE);

      // Draw icon based on current size
      if (ui.currentBrushSize == BrushSize::Single)
        DrawCircle((int)(brushBtnMsg.x + btnSize / 2),
                   (int)(brushBtnMsg.y + btnSize / 2), 2, WHITE);
      else
        DrawCircle((int)(brushBtnMsg.x + btnSize / 2),
                   (int)(brushBtnMsg.y + btnSize / 2),
                   (int)(ui.currentBrushSize) * 2, WHITE);

      DrawText("Brush", (int)brushBtnMsg.x + 10,
               (int)brushBtnMsg.y + btnSize + 5, 10, LIGHTGRAY);

      if (CheckCollisionPointRec(mousePos, brushBtnMsg) &&
          IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !ui.showBrushPopup) {
        ui.showBrushPopup = true;
        ui.popupJustOpened = true; // Flag to prevent immediate close
      }

      // Tools
      int numTools = 0;
      TileType terrainTypes[] = {TileType::DeepOcean,    TileType::Ocean,
                                 TileType::ShallowOcean, TileType::Sand,
                                 TileType::Grass,        TileType::Forest,
                                 TileType::Mountain,     TileType::Snow};
      DecorationType natureTypes[] = {
          DecorationType::Tree,     DecorationType::PineTree,
          DecorationType::PalmTree, DecorationType::Bush,
          DecorationType::Flower,   DecorationType::Mushroom}; // Updated
      DecorationType rockTypes[] = {
          DecorationType::SmallRock, DecorationType::MediumRock,
          DecorationType::BigRock}; // Updated with new types

      if (ui.currentTab == UIState::Terrain)
        numTools = 9;
      else if (ui.currentTab == UIState::Nature)
        numTools = 7;
      else if (ui.currentTab == UIState::Rocks)
        numTools = 4; // 3 types + Eraser

      for (int i = 0; i < numTools; i++) {
        Rectangle btnRect = {startX + i * (btnSize + gap), startY, btnSize,
                             btnSize};
        bool isSelected = (ui.selectedToolIndex == i);

        DrawRectangleRec(btnRect, isSelected ? LIGHTGRAY : DARKGRAY);
        if (isSelected)
          DrawRectangleLinesEx(btnRect, 3, YELLOW);
        else
          DrawRectangleLinesEx(btnRect, 2, BLACK);

        Texture2D icon = {0};
        bool isEraser = false;

        if (ui.currentTab == UIState::Terrain) {
          if (i < 8)
            icon = world.GetTextureForUI(terrainTypes[i]);
          else
            isEraser = true;
        } else if (ui.currentTab == UIState::Nature) {
          if (i < 6)
            icon = world.GetTextureForUI(natureTypes[i]);
          else
            isEraser = true;
        } else if (ui.currentTab == UIState::Rocks) {
          if (i < 3)
            icon = world.GetTextureForUI(rockTypes[i]);
          else
            isEraser = true;
        }

        if (isEraser) {
          DrawText("X", (int)btnRect.x + 20, (int)btnRect.y + 10, 40, RED);
          DrawText("Erase", (int)btnRect.x + 10, (int)btnRect.y + 50, 10,
                   WHITE);
        } else if (icon.id > 0) {
          float scale =
              std::min(btnSize / icon.width, btnSize / icon.height) * 0.8f;
          Rectangle src = {0, 0, (float)icon.width, (float)icon.height};
          Rectangle dest = {btnRect.x + btnSize / 2 - (icon.width * scale) / 2,
                            btnRect.y + btnSize / 2 - (icon.height * scale) / 2,
                            icon.width * scale, icon.height * scale};
          DrawTexturePro(icon, src, dest, {0, 0}, 0, WHITE);
        }

        if (CheckCollisionPointRec(mousePos, btnRect) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !ui.showBrushPopup) {
          ui.selectedToolIndex = i;
        }
      }
    }

    // Brush Popup Logic
    if (ui.showBrushPopup) {
      DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));

      Rectangle popupRect = {SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 100,
                             400, 200};
      DrawRectangleRec(popupRect, GetColor(0x16213eFF));
      DrawRectangleLinesEx(popupRect, 3, LIGHTGRAY);
      DrawText("Select Brush Size", (int)popupRect.x + 110,
               (int)popupRect.y + 20, 20, WHITE);

      const char *sizes[] = {"1x1", "S", "M", "L", "XL"};
      int sizeVals[] = {(int)BrushSize::Single, (int)BrushSize::S,
                        (int)BrushSize::M, (int)BrushSize::L,
                        (int)BrushSize::XL};

      for (int i = 0; i < 5; i++) {
        float bx = popupRect.x + 25 + i * (60 + 10);
        float by = popupRect.y + 80;
        Rectangle bRect = {bx, by, 60, 60};

        bool active = ((int)ui.currentBrushSize == sizeVals[i]);
        Color bColor = active ? YELLOW : DARKGRAY;

        DrawRectangleRec(bRect, bColor);
        DrawRectangleLinesEx(bRect, 2, WHITE);
        DrawText(sizes[i], (int)bx + 15, (int)by + 15, 20,
                 active ? BLACK : WHITE);

        int rsize = (sizeVals[i] == 0) ? 2 : sizeVals[i];
        DrawCircleLines((int)bx + 30, (int)by + 45, rsize * 1.5f, LIGHTGRAY);

        if (CheckCollisionPointRec(mousePos, bRect) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          ui.currentBrushSize = (BrushSize)sizeVals[i];
          ui.showBrushPopup = false;
        }
      }

      // Close logic
      if (!ui.popupJustOpened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
          !CheckCollisionPointRec(mousePos, popupRect)) {
        ui.showBrushPopup = false;
      }
    }

    DrawTextureEx(ui.texCursor, mousePos, 0, 0.5f, WHITE);
    EndDrawing();
  }

  ui.Unload();
  CloseWindow();
  return 0;
}
