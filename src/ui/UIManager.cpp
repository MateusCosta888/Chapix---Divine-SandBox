#include "UIManager.h"
#include "raymath.h"
#include <string>

UIManager::UIManager() {}
UIManager::~UIManager() { Unload(); }

void UIManager::Load() {
  texButton = LoadTexture("assets/decorations/20240709dragonButtonA-Sheet.png");
  if (texButton.id == 0) {
    // Fallback or error logging could go here, but this file is known to exist
    TraceLog(LOG_WARNING, "Failed to load default button texture");
  }

  texPanel =
      LoadTexture("assets/decorations/20240713dragonFilledFrame-Sheet.png");
  texTab = LoadTexture("assets/decorations/20240707dragonTabA-Sheet.png");
  texCursor = LoadTexture("assets/cursor.png");

  SetTextureFilter(texButton, TEXTURE_FILTER_POINT);
  SetTextureFilter(texPanel, TEXTURE_FILTER_POINT);
  SetTextureFilter(texTab, TEXTURE_FILTER_POINT);
}

void UIManager::Unload() {
  UnloadTexture(texButton);
  UnloadTexture(texPanel);
  UnloadTexture(texTab);
  UnloadTexture(texCursor);
}

bool UIManager::IsPointerOnUI() const {
  Vector2 mousePos = GetMousePosition();
  bool onUI = mousePos.y > (SCREEN_HEIGHT - TOOLBAR_HEIGHT - TAB_HEIGHT);
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    TraceLog(LOG_INFO,
             "DEBUG: IsPointerOnUI Check: MouseY=%f, Threshold=%d, Result=%d",
             mousePos.y, (SCREEN_HEIGHT - TOOLBAR_HEIGHT - TAB_HEIGHT), onUI);
  }
  return onUI;
}

void UIManager::Update(World &world, Camera2D &camera) {
  popupJustOpened = false;
  HandleInput(world, camera);
}

void UIManager::HandleInput(World &world, Camera2D &camera) {
  Vector2 mousePos = GetMousePosition();
  bool isPointerOnUI = IsPointerOnUI();

  // World Interaction (Painting)
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !isPointerOnUI &&
      !showBrushPopup && currentTab != UIState::Settings) {
    Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
    int tx = (int)(worldPos.x / 10);
    int ty = (int)(worldPos.y / 10);

    // Brush Logic
    int size =
        (currentBrushSize == BrushSize::Single) ? 1 : (int)currentBrushSize;
    int start = -size / 2;
    int end = size / 2 + size % 2;

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

        if (nx < padding || nx >= world.GetWidth() - padding || ny < padding ||
            ny >= world.GetHeight() - padding)
          continue;

        // Tab Logic
        if (currentTab == UIState::Terrain) {
          TileType types[] = {TileType::DeepOcean,    TileType::Ocean,
                              TileType::ShallowOcean, TileType::Sand,
                              TileType::Grass,        TileType::Forest,
                              TileType::Mountain,     TileType::Snow};
          if (selectedToolIndex < 8) {
            Tile &t = world.GetTile(nx, ny);
            if (t.type != types[selectedToolIndex]) {
              world.SetTileType(nx, ny, types[selectedToolIndex]);
            }
          } else {
            // Eraser
            Tile &t = world.GetTile(nx, ny);

            // Priority: Remove decoration first
            if (t.decoration != DecorationType::None) {
              TraceLog(LOG_INFO, "ERASER: Removing Decoration at %d,%d", nx,
                       ny);
              world.SetTileDecoration(nx, ny, DecorationType::None);
            }
            // Then remove terrain (Dig to Bedrock)
            else if (t.type != TileType::Bedrock) {
              TraceLog(LOG_INFO, "ERASER: Digging to Bedrock at %d,%d", nx, ny);
              world.SetTileType(nx, ny, TileType::Bedrock);
            } else {
              TraceLog(LOG_INFO,
                       "ERASER: Hit Bedrock at %d,%d (Indestructible)", nx, ny);
            }
          }
        } else if (currentTab == UIState::Nature) {
          DecorationType decs[] = {
              DecorationType::Tree,     DecorationType::PineTree,
              DecorationType::PalmTree, DecorationType::Bush,
              DecorationType::Flower,   DecorationType::Mushroom};
          if (selectedToolIndex < 6) {
            Tile &t = world.GetTile(nx, ny);
            if (t.decoration != decs[selectedToolIndex]) {
              world.SetTileDecoration(nx, ny, decs[selectedToolIndex]);
            }
          } else {
            // Eraser
            Tile &t = world.GetTile(nx, ny);
            if (t.decoration != DecorationType::None)
              world.SetTileDecoration(nx, ny, DecorationType::None);
          }
        } else if (currentTab == UIState::Rocks) {
          DecorationType decs[] = {DecorationType::Rock,
                                   DecorationType::BigRock};
          if (selectedToolIndex < 2) {
            Tile &t = world.GetTile(nx, ny);
            if (t.decoration != decs[selectedToolIndex])
              world.SetTileDecoration(nx, ny, decs[selectedToolIndex]);
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

  // Right Click - Brush Popup
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    showBrushPopup = true;
    popupJustOpened = true;
  }
}

void UIManager::Draw(const World &world) {
  Vector2 mousePos = GetMousePosition();

  DrawToolbar(world);

  // Draw Cursor
  DrawTextureEx(texCursor, mousePos, 0.0f, 2.0f, WHITE);
}

void UIManager::DrawToolbar(const World &world) {
  Vector2 mousePos = GetMousePosition();

  // UI Render
  Rectangle tabArea = {0, (float)SCREEN_HEIGHT - TOOLBAR_HEIGHT - TAB_HEIGHT,
                       (float)SCREEN_WIDTH, (float)TAB_HEIGHT};
  DrawRectangleRec(tabArea, GetColor(0x16213eFF));

  const char *tabNames[] = {"Terrains", "Nature", "Rocks", "Settings"};
  for (int i = 0; i < 4; i++) {
    float tabW = 150;
    Rectangle tabRect = {i * tabW, tabArea.y, tabW, tabArea.height};
    bool isHover = CheckCollisionPointRec(mousePos, tabRect);
    bool isActive = ((int)currentTab == i);

    Color tabColor =
        isActive ? GetColor(0x0f3460FF)
                 : (isHover ? GetColor(0x1a1a2eFF) : GetColor(0x16213eFF));
    DrawRectangleRec(tabRect, tabColor);

    // Draw Tab border
    Rectangle srcArg = {0, 0, (float)texTab.width, (float)texTab.height};
    Rectangle destArg = {tabRect.x, tabRect.y, tabRect.width, tabRect.height};
    // DrawTexturePro(texTab, srcArg, destArg, {0, 0}, 0.0f, WHITE); // Basic
    // texture

    DrawText(tabNames[i], (int)tabRect.x + 10, (int)tabRect.y + 5, 20,
             isActive ? WHITE : GRAY);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHover) {
      currentTab = (UIState)i;
      selectedToolIndex = 0; // Reset tool on tab switch
    }
  }

  // Toolbar Background
  DrawRectangle(0, SCREEN_HEIGHT - TOOLBAR_HEIGHT, SCREEN_WIDTH, TOOLBAR_HEIGHT,
                GetColor(0x0f3460FF));

  // Content based on Tab
  float startX = 20;
  float startY = SCREEN_HEIGHT - TOOLBAR_HEIGHT + 10;
  float btnSize = 60;
  float padding = 10;

  int numTools = 0;

  // Arrays for rendering buttons. Max tools per tab approx 8-9
  if (currentTab == UIState::Terrain) {
    numTools = 9; // 8 types + eraser
    for (int i = 0; i < numTools; i++) {
      Rectangle btnRect = {startX + i * (btnSize + padding), startY, btnSize,
                           btnSize};
      if (i == 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        TraceLog(LOG_INFO,
                 "DEBUG: Button 0 Rect Y: %f, Mouse Y: %f, ScreenH: %d",
                 btnRect.y, GetMousePosition().y, SCREEN_HEIGHT);
      }
      bool isHover = CheckCollisionPointRec(mousePos, btnRect);

      // Button Background
      Rectangle srcPanel = {0, 0, (float)texButton.width,
                            (float)texButton.height};
      if (selectedToolIndex == i)
        srcPanel = {
            0, 0, (float)texButton.width,
            (float)texButton
                .height}; // keeping it simple, maybe add hover state later

      DrawTexturePro(texButton, srcPanel, btnRect, {0, 0}, 0.0f,
                     isHover ? LIGHTGRAY : WHITE);

      if (selectedToolIndex == i) {
        DrawRectangleLinesEx(btnRect, 3, YELLOW);
      }

      // Icon
      if (i < 8) {
        // Get texture sample from World/ResourceManager
        // Since UIManager doesn't have direct access to ResourceManager's
        // internal textures easily unless we expose more, we can use the World
        // helper GetTextureForUI

        TileType types[] = {TileType::DeepOcean,    TileType::Ocean,
                            TileType::ShallowOcean, TileType::Sand,
                            TileType::Grass,        TileType::Forest,
                            TileType::Mountain,     TileType::Snow};

        // We need const_cast because GetTextureForUI was non-const in World.h,
        // but Draw is const. Best practice: Update World.h to make
        // GetTextureForUI const. For now, assuming world is passed as non-const
        // or we fix constness. Actually, Draw(const World&) implies we
        // shouldn't modify world. Let's rely on World::GetTextureForUI being
        // effectively const-safe or fix it. It calls
        // ResourceManager::GetTextureForUI which IS const-safe logic wise
        // (returns copy/ref to existing tex).

        Texture2D tex = const_cast<World &>(world).GetTextureForUI(types[i]);

        if (tex.id > 0) {
          // Center and fit
          float scale =
              std::min((btnSize - 10) / tex.width, (btnSize - 10) / tex.height);
          Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
          Rectangle dest = {btnRect.x + 5, btnRect.y + 5, tex.width * scale,
                            tex.height * scale};
          DrawTexturePro(tex, src, dest, {0, 0}, 0.0f, WHITE);
        }
      } else {
        DrawText("Eraser", (int)btnRect.x + 5, (int)btnRect.y + 20, 10, RED);
      }

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHover &&
          !showBrushPopup) {
        TraceLog(LOG_INFO, "UI: Clicked Tool Index %d", i);
        selectedToolIndex = i;
      }
    }

  } else if (currentTab == UIState::Nature) {
    numTools = 7; // 6 types + eraser
    for (int i = 0; i < numTools; i++) {
      Rectangle btnRect = {startX + i * (btnSize + padding), startY, btnSize,
                           btnSize};
      bool isHover = CheckCollisionPointRec(mousePos, btnRect);
      DrawTexturePro(texButton,
                     {0, 0, (float)texButton.width, (float)texButton.height},
                     btnRect, {0, 0}, 0.0f, isHover ? LIGHTGRAY : WHITE);
      if (selectedToolIndex == i) {
        DrawRectangleLinesEx(btnRect, 3, YELLOW);
      }

      if (i < 6) {
        DecorationType decs[] = {
            DecorationType::Tree,     DecorationType::PineTree,
            DecorationType::PalmTree, DecorationType::Bush,
            DecorationType::Flower,   DecorationType::Mushroom};
        Texture2D tex = const_cast<World &>(world).GetTextureForUI(decs[i]);
        if (tex.id > 0) {
          float scale =
              std::min((btnSize - 10) / tex.width, (btnSize - 10) / tex.height);
          Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
          Rectangle dest = {btnRect.x + 5, btnRect.y + 5, tex.width * scale,
                            tex.height * scale};
          DrawTexturePro(tex, src, dest, {0, 0}, 0.0f, WHITE);
        }
      } else {
        DrawText("Eraser", (int)btnRect.x + 5, (int)btnRect.y + 20, 10, RED);
      }

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHover &&
          !showBrushPopup) {
        TraceLog(LOG_INFO, "UI: Clicked Tool Index %d", i);
        selectedToolIndex = i;
      }
    }
  } else if (currentTab == UIState::Rocks) {
    numTools = 3; // 2 types + eraser
    for (int i = 0; i < numTools; i++) {
      Rectangle btnRect = {startX + i * (btnSize + padding), startY, btnSize,
                           btnSize};
      bool isHover = CheckCollisionPointRec(mousePos, btnRect);
      DrawTexturePro(texButton,
                     {0, 0, (float)texButton.width, (float)texButton.height},
                     btnRect, {0, 0}, 0.0f, isHover ? LIGHTGRAY : WHITE);
      if (selectedToolIndex == i) {
        DrawRectangleLinesEx(btnRect, 3, YELLOW);
      }

      if (i < 2) {
        DecorationType decs[] = {DecorationType::Rock, DecorationType::BigRock};
        Texture2D tex = const_cast<World &>(world).GetTextureForUI(decs[i]);
        if (tex.id > 0) {
          float scale =
              std::min((btnSize - 10) / tex.width, (btnSize - 10) / tex.height);
          Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
          Rectangle dest = {btnRect.x + 5, btnRect.y + 5, tex.width * scale,
                            tex.height * scale};
          DrawTexturePro(tex, src, dest, {0, 0}, 0.0f, WHITE);
        }
      } else {
        DrawText("Eraser", (int)btnRect.x + 5, (int)btnRect.y + 20, 10, RED);
      }

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHover &&
          !showBrushPopup) {
        TraceLog(LOG_INFO, "UI: Clicked Tool Index %d", i);
        selectedToolIndex = i;
      }
    }
  } else if (currentTab == UIState::Settings) {
    DrawText("Press 'R' in game to Regenerate", (int)startX, (int)startY + 20,
             20, WHITE);
  }

  // Brush Popup Logic
  if (showBrushPopup) {
    // Close on click outside (simulated)
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !popupJustOpened) {
      showBrushPopup = false;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      // Check collision with popup items, if none, close
      // For now just close helper
      // showBrushPopup = false;
    }

    Vector2 center = {(float)SCREEN_WIDTH / 2, (float)SCREEN_HEIGHT / 2};
    Rectangle popupRect = {center.x - 150, center.y - 100, 300, 200};

    DrawRectangleRec(popupRect, GetColor(0x1a1a2eFF));
    DrawRectangleLinesEx(popupRect, 2, WHITE);
    DrawText("Brush Size", (int)popupRect.x + 10, (int)popupRect.y + 10, 20,
             WHITE);

    const char *sizes[] = {"Single", "Small", "Medium", "Large", "X-Large"};
    BrushSize bSizes[] = {BrushSize::Single, BrushSize::S, BrushSize::M,
                          BrushSize::L, BrushSize::XL};

    for (int i = 0; i < 5; i++) {
      Rectangle item = {popupRect.x + 20, popupRect.y + 40 + i * 30, 260, 25};
      bool isHover = CheckCollisionPointRec(mousePos, item);
      if (isHover)
        DrawRectangleRec(item, GetColor(0x0f3460FF));

      DrawText(sizes[i], (int)item.x + 5, (int)item.y + 2, 20, WHITE);
      if (currentBrushSize == bSizes[i])
        DrawText("*", (int)item.x + 240, (int)item.y, 20, YELLOW);

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHover) {
        currentBrushSize = bSizes[i];
        showBrushPopup = false;
      }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(mousePos, popupRect)) {
      showBrushPopup = false;
    }
  }
}
