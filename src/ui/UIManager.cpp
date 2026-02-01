#include "UIManager.h"
#include "raymath.h"
#include <string>

UIManager::UIManager() {}
UIManager::~UIManager() { Unload(); }

void UIManager::Load() { texCursor = LoadTexture("assets/cursor.png"); }

void UIManager::Unload() { UnloadTexture(texCursor); }

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
            if (t.decoration != decs[selectedToolIndex]) {
              world.SetTileDecoration(nx, ny, decs[selectedToolIndex]);
            }
          } else {
            // Eraser
            Tile &t = world.GetTile(nx, ny);
            if (t.decoration != DecorationType::None)
              world.SetTileDecoration(nx, ny, DecorationType::None);
          }
        } else if (currentTab == UIState::Creatures) {
          // Creature Placement
          EntityType creatureTypesForPlacement[] = {
              EntityType::Human, EntityType::Cow,  EntityType::Chicken,
              EntityType::Sheep, EntityType::Bull, EntityType::Chicken2,
              EntityType::Lamb,  EntityType::Pig,  EntityType::Turkey};
          if (selectedToolIndex >= 0 && selectedToolIndex < 9) {
            // Only place on click (not hold) to avoid spamming
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
              world.AddEntity(creatureTypesForPlacement[selectedToolIndex],
                              {(float)nx + 0.5f, (float)ny + 0.5f});
            }
          }
        }
      }
    }
  }
}

void UIManager::Draw(const World &world) {
  Vector2 mousePos = GetMousePosition();

  DrawToolbar(world);

  // Draw Cursor
  DrawTextureEx(texCursor, mousePos, 0.0f, cursorScale, WHITE);
}

void UIManager::DrawToolbar(const World &world) {
  Vector2 mousePos = GetMousePosition();

  // UI Render
  Rectangle tabArea = {0, (float)SCREEN_HEIGHT - TOOLBAR_HEIGHT - TAB_HEIGHT,
                       (float)SCREEN_WIDTH, (float)TAB_HEIGHT};
  DrawRectangleRec(tabArea, GetColor(0x16213eFF));

  const char *tabNames[] = {"Terrains", "Nature", "Rocks", "Creatures",
                            "Settings"};
  for (int i = 0; i < 5; i++) {
    float tabW = 150;
    Rectangle tabRect = {i * tabW, tabArea.y, tabW, tabArea.height};
    bool isHover = CheckCollisionPointRec(GetMousePosition(), tabRect);
    bool isActive = ((int)currentTab == i);

    Color tabColor =
        isActive ? GetColor(0x0f3460FF)
                 : (isHover ? GetColor(0x1a1a2eFF) : GetColor(0x16213eFF));
    DrawRectangleRec(tabRect, tabColor);
    DrawRectangleLinesEx(tabRect, 1, GetColor(0x0f3460FF));
    DrawText(tabNames[i], (int)tabRect.x + 40, (int)tabRect.y + 8, 20,
             isActive ? WHITE : LIGHTGRAY);

    if (isHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !showBrushPopup) {
      currentTab = (UIState)i;
      selectedToolIndex = 0;
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
      Color btnColor =
          (selectedToolIndex == i)
              ? GetColor(0x1a1a2eFF)
              : (isHover ? GetColor(0x16213eFF) : GetColor(0x0f3460FF));
      DrawRectangleRec(btnRect, btnColor);
      DrawRectangleLinesEx(btnRect, 2, WHITE);

      if (selectedToolIndex == i) {
        DrawRectangleLinesEx(btnRect, 3, YELLOW);
      }

      // Icon
      if (i < 8) {
        TileType types[] = {TileType::DeepOcean,    TileType::Ocean,
                            TileType::ShallowOcean, TileType::Sand,
                            TileType::Grass,        TileType::Forest,
                            TileType::Mountain,     TileType::Snow};

        Texture2D tex = const_cast<World &>(world).GetTextureForUI(types[i]);

        if (tex.id > 0) {
          // Center and fit - use integer scale for pixel-perfect scaling
          float availableSize = btnSize - 10;
          float rawScale =
              availableSize / (float)std::max(tex.width, tex.height);
          float scale =
              std::max(1.0f, std::floor(rawScale)); // Pixel scale, at least 1x

          float scaledW = tex.width * scale;
          float scaledH = tex.height * scale;

          // Center in button
          float offsetX = (btnSize - scaledW) / 2.0f;
          float offsetY = (btnSize - scaledH) / 2.0f;

          Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
          Rectangle dest = {btnRect.x + offsetX, btnRect.y + offsetY, scaledW,
                            scaledH};
          DrawTexturePro(tex, src, dest, {0, 0}, 0.0f, WHITE);
        }
      } else {
        // Procedural Eraser Icon
        float pad = 15;
        Rectangle eraserRect = {btnRect.x + pad, btnRect.y + pad,
                                btnSize - pad * 2, btnSize - pad * 2};
        DrawRectanglePro({eraserRect.x + eraserRect.width / 2,
                          eraserRect.y + eraserRect.height / 2,
                          eraserRect.width, eraserRect.height},
                         {eraserRect.width / 2, eraserRect.height / 2}, 30.0f,
                         WHITE);
        DrawRectanglePro({eraserRect.x + eraserRect.width / 2,
                          eraserRect.y + eraserRect.height / 2,
                          eraserRect.width, eraserRect.height / 2},
                         {eraserRect.width / 2, eraserRect.height / 2}, 30.0f,
                         PINK);
        // DrawText("Eraser", (int)btnRect.x + 5, (int)btnRect.y + 40, 10, RED);
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

      Color btnColor =
          (selectedToolIndex == i)
              ? GetColor(0x1a1a2eFF)
              : (isHover ? GetColor(0x16213eFF) : GetColor(0x0f3460FF));
      DrawRectangleRec(btnRect, btnColor);
      DrawRectangleLinesEx(btnRect, 2, WHITE);

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
        // Procedural Eraser Icon
        float pad = 15;
        Rectangle eraserRect = {btnRect.x + pad, btnRect.y + pad,
                                btnSize - pad * 2, btnSize - pad * 2};
        DrawRectanglePro({eraserRect.x + eraserRect.width / 2,
                          eraserRect.y + eraserRect.height / 2,
                          eraserRect.width, eraserRect.height},
                         {eraserRect.width / 2, eraserRect.height / 2}, 30.0f,
                         WHITE);
        DrawRectanglePro({eraserRect.x + eraserRect.width / 2,
                          eraserRect.y + eraserRect.height / 2,
                          eraserRect.width, eraserRect.height / 2},
                         {eraserRect.width / 2, eraserRect.height / 2}, 30.0f,
                         PINK);
        // DrawText("Eraser", (int)btnRect.x + 5, (int)btnRect.y + 40, 10, RED);
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

      Color btnColor =
          (selectedToolIndex == i)
              ? GetColor(0x1a1a2eFF)
              : (isHover ? GetColor(0x16213eFF) : GetColor(0x0f3460FF));
      DrawRectangleRec(btnRect, btnColor);
      DrawRectangleLinesEx(btnRect, 2, WHITE);

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
        // Procedural Eraser Icon
        float pad = 15;
        Rectangle eraserRect = {btnRect.x + pad, btnRect.y + pad,
                                btnSize - pad * 2, btnSize - pad * 2};
        DrawRectanglePro({eraserRect.x + eraserRect.width / 2,
                          eraserRect.y + eraserRect.height / 2,
                          eraserRect.width, eraserRect.height},
                         {eraserRect.width / 2, eraserRect.height / 2}, 30.0f,
                         WHITE);
        DrawRectanglePro({eraserRect.x + eraserRect.width / 2,
                          eraserRect.y + eraserRect.height / 2,
                          eraserRect.width, eraserRect.height / 2},
                         {eraserRect.width / 2, eraserRect.height / 2}, 30.0f,
                         PINK);
        // DrawText("Eraser", (int)btnRect.x + 5, (int)btnRect.y + 40, 10, RED);
      }

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHover &&
          !showBrushPopup) {
        TraceLog(LOG_INFO, "UI: Clicked Tool Index %d", i);
        selectedToolIndex = i;
      }
    }
  } else if (currentTab == UIState::Creatures) {
    numTools =
        9; // Human, Cow, Chicken, Sheep, Bull, Chicken2, Lamb, Pig, Turkey
    EntityType creatureTypes[] = {
        EntityType::Human, EntityType::Cow,  EntityType::Chicken,
        EntityType::Sheep, EntityType::Bull, EntityType::Chicken2,
        EntityType::Lamb,  EntityType::Pig,  EntityType::Turkey};

    for (int i = 0; i < numTools; i++) {
      Rectangle btnRect = {startX + i * (btnSize + padding), startY, btnSize,
                           btnSize};
      bool isHover = CheckCollisionPointRec(mousePos, btnRect);

      Color btnColor =
          (selectedToolIndex == i)
              ? GetColor(0x1a1a2eFF)
              : (isHover ? GetColor(0x16213eFF) : GetColor(0x0f3460FF));
      DrawRectangleRec(btnRect, btnColor);
      DrawRectangleLinesEx(btnRect, 2, WHITE);

      if (selectedToolIndex == i)
        DrawRectangleLinesEx(btnRect, 3, YELLOW);

      // Draw Creature Icon
      Texture2D tex =
          const_cast<World &>(world).GetTextureForUI(creatureTypes[i]);
      if (tex.id > 0) {
        // Center and fit - use integer scale for pixel-perfect scaling
        float availableSize = btnSize - 10;
        float rawScale = availableSize / (float)std::max(tex.width, tex.height);
        float scale =
            std::max(1.0f, std::floor(rawScale)); // Pixel scale, at least 1x

        float scaledW = tex.width * scale;
        float scaledH = tex.height * scale;

        // Center in button
        float offsetX = (btnSize - scaledW) / 2.0f;
        float offsetY = (btnSize - scaledH) / 2.0f;

        Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
        Rectangle dest = {btnRect.x + offsetX, btnRect.y + offsetY, scaledW,
                          scaledH};
        DrawTexturePro(tex, src, dest, {0, 0}, 0.0f, WHITE);
      }

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHover &&
          !showBrushPopup) {
        selectedToolIndex = i;
      }
    }
  } else if (currentTab == UIState::Settings) {
    DrawText("Press 'R' in game to Regenerate", (int)startX, (int)startY + 20,
             20, WHITE);

    // Cursor Size Button
    Rectangle cursorBtn = {startX, startY + 50, 200, 40};
    bool isHover = CheckCollisionPointRec(mousePos, cursorBtn);
    DrawRectangleRec(cursorBtn,
                     isHover ? GetColor(0x1a1a2eFF) : GetColor(0x0f3460FF));
    DrawRectangleLinesEx(cursorBtn, 2, WHITE);

    // Display Current Size
    const char *sizeText = "Cursor: 0.5x";
    if (cursorScale == 0.25f)
      sizeText = "Cursor: 0.25x";
    else if (cursorScale == 0.75f)
      sizeText = "Cursor: 0.75x";
    else if (cursorScale == 1.0f)
      sizeText = "Cursor: 1.0x";

    DrawText(sizeText, (int)cursorBtn.x + 10, (int)cursorBtn.y + 10, 20, WHITE);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHover) {
      // Cycle logic: 1.0 -> 0.75 -> 0.5 -> 0.25 -> 1.0 (Decreasing as requested
      // or simply smaller range) Let's cycle up for intuition: 0.25 -> 0.5 ->
      // 0.75 -> 1.0
      if (cursorScale == 0.25f)
        cursorScale = 0.5f;
      else if (cursorScale == 0.5f)
        cursorScale = 0.75f;
      else if (cursorScale == 0.75f)
        cursorScale = 1.0f;
      else
        cursorScale = 0.25f;
    }
  }

  // Brush Size Toggle Button (Right Side)
  float toggleW = 120;
  Rectangle toggleRect = {(float)SCREEN_WIDTH - toggleW - 20, startY, toggleW,
                          btnSize};
  bool isToggleHover = CheckCollisionPointRec(mousePos, toggleRect);

  DrawRectangleRec(toggleRect,
                   isToggleHover ? GetColor(0x1a1a2eFF) : GetColor(0x0f3460FF));
  DrawRectangleLinesEx(toggleRect, 2, WHITE);

  const char *sizeNames[] = {"Single", "Small", "Medium", "Large", "X-Large"};
  // Convert enum to index for display
  int sizeIndex = 0;
  if (currentBrushSize == BrushSize::S)
    sizeIndex = 1;
  else if (currentBrushSize == BrushSize::M)
    sizeIndex = 2;
  else if (currentBrushSize == BrushSize::L)
    sizeIndex = 3;
  else if (currentBrushSize == BrushSize::XL)
    sizeIndex = 4;

  DrawText("Size:", (int)toggleRect.x + 10, (int)toggleRect.y + 10, 10,
           LIGHTGRAY);
  DrawText(sizeNames[sizeIndex], (int)toggleRect.x + 10, (int)toggleRect.y + 25,
           20, WHITE);

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isToggleHover) {
    // Cycle size
    if (currentBrushSize == BrushSize::Single)
      currentBrushSize = BrushSize::S;
    else if (currentBrushSize == BrushSize::S)
      currentBrushSize = BrushSize::M;
    else if (currentBrushSize == BrushSize::M)
      currentBrushSize = BrushSize::L;
    else if (currentBrushSize == BrushSize::L)
      currentBrushSize = BrushSize::XL;
    else
      currentBrushSize = BrushSize::Single;

    TraceLog(LOG_INFO, "UI: Changed Brush Size to %d", (int)currentBrushSize);
  }
}
