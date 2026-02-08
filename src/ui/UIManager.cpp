#include "UIManager.h"
#include "raymath.h"
#include <string>

UIManager::UIManager() {}
UIManager::~UIManager() { Unload(); }

void UIManager::Load() {
  texCursor = LoadTexture("assets/cursor.png");

  // Load 9-Slice Panel Textures
  texPanelTL = LoadTexture("assets/UI/TaskBar/bordas/superior esquerda.png");
  texPanelTC = LoadTexture("assets/UI/TaskBar/bordas/Superior Central.png");
  texPanelTR = LoadTexture("assets/UI/TaskBar/bordas/Superior Direita.png");
  texPanelML = LoadTexture("assets/UI/TaskBar/bordas/Meio esquerda.png");
  texPanelMC = LoadTexture("assets/UI/TaskBar/bordas/Meio central.png");
  texPanelMR = LoadTexture("assets/UI/TaskBar/bordas/Meio Direita.png");
  texPanelBL = LoadTexture("assets/UI/TaskBar/bordas/Inferior Esquerda.png");
  texPanelBC = LoadTexture("assets/UI/TaskBar/bordas/Inferior Central.png");
  texPanelBR = LoadTexture("assets/UI/TaskBar/bordas/Inferior Direita.png");

  // Load Button Texture
  texButton = LoadTexture("assets/UI/TaskBar/Botoes/boto004.png");
  texTabButton = LoadTexture("assets/UI/TaskBar/Botoes/ButtomAbas.png");

  // Load Font
  // Load Font
  uiFont = LoadFontEx("assets/UI/Font/BoldPixels.ttf", 40, 0, 250);

  // Set default cursor size
  cursorScale = 0.20f;

  // Load Circle Mask Shader
  const char *fsCode = R"(
      #version 330
      in vec2 fragTexCoord;
      in vec4 fragColor;
      out vec4 finalColor;
      uniform sampler2D texture0;
      uniform vec4 colDiffuse;

      void main()
      {
          vec4 texelColor = texture(texture0, fragTexCoord);
          vec2 center = vec2(0.5, 0.5);
          float dist = distance(fragTexCoord, center);
          if (dist > 0.49) discard; 
          finalColor = texelColor * colDiffuse * fragColor;
      }
  )";
  circleMaskShader = LoadShaderFromMemory(0, fsCode);

  // Load Custom Icons
  texEraser = LoadTexture("assets/UI/Icons/Itens/Eraser.png");
  texIconWaterDeep = LoadTexture("assets/UI/Icons/water/funda agua.png");
  texIconWaterOcean = LoadTexture("assets/UI/Icons/water/media agua.png");
  texIconWaterShallow = LoadTexture("assets/UI/Icons/water/rasa agua.png");
}

void UIManager::Unload() {
  UnloadTexture(texCursor);
  UnloadTexture(texPanelTL);
  UnloadTexture(texPanelTC);
  UnloadTexture(texPanelTR);
  UnloadTexture(texPanelML);
  UnloadTexture(texPanelMC);
  UnloadTexture(texPanelMR);
  UnloadTexture(texPanelBL);
  UnloadTexture(texPanelBC);
  UnloadTexture(texPanelBR);
  UnloadTexture(texButton);
  UnloadTexture(texTabButton);
  UnloadTexture(texEraser);
  UnloadTexture(texIconWaterDeep);
  UnloadTexture(texIconWaterOcean);
  UnloadTexture(texIconWaterShallow);
  UnloadFont(uiFont);
  UnloadShader(circleMaskShader);
}

bool UIManager::IsPointerOnUI() const {
  Vector2 mousePos = GetMousePosition();
  bool onUI = mousePos.y > (SCREEN_HEIGHT - TOOLBAR_HEIGHT - TAB_HEIGHT);
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    if (mousePos.y > (SCREEN_HEIGHT - TOOLBAR_HEIGHT - TAB_HEIGHT)) {
      return true;
    }
    return onUI;
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
              EntityType::HumanUnarmed, EntityType::HumanArmed,
              EntityType::Boar,         EntityType::Cow,
              EntityType::Chicken,      EntityType::Sheep,
              EntityType::Bull,         EntityType::Chicken2,
              EntityType::Lamb,         EntityType::Pig,
              EntityType::Turkey};

          // Updated size to 11
          if (selectedToolIndex >= 0 && selectedToolIndex < 11) {
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
  // Helper helper
  auto DrawNineSlicePanel = [&](Rectangle rect) {
    int cornerW = texPanelTL.width;
    int cornerH = texPanelTL.height;

    // Corners
    DrawTexture(texPanelTL, rect.x, rect.y, WHITE);
    DrawTexture(texPanelTR, rect.x + rect.width - cornerW, rect.y, WHITE);
    DrawTexture(texPanelBL, rect.x, rect.y + rect.height - cornerH, WHITE);
    DrawTexture(texPanelBR, rect.x + rect.width - cornerW,
                rect.y + rect.height - cornerH, WHITE);

    // Edges (Stretched)
    DrawTexturePro(
        texPanelTC, {0, 0, (float)texPanelTC.width, (float)texPanelTC.height},
        {rect.x + cornerW, rect.y, rect.width - 2 * cornerW, (float)cornerH},
        {0, 0}, 0.0f, WHITE);
    DrawTexturePro(texPanelBC,
                   {0, 0, (float)texPanelBC.width, (float)texPanelBC.height},
                   {rect.x + cornerW, rect.y + rect.height - cornerH,
                    rect.width - 2 * cornerW, (float)cornerH},
                   {0, 0}, 0.0f, WHITE);
    DrawTexturePro(
        texPanelML, {0, 0, (float)texPanelML.width, (float)texPanelML.height},
        {rect.x, rect.y + cornerH, (float)cornerW, rect.height - 2 * cornerH},
        {0, 0}, 0.0f, WHITE);
    DrawTexturePro(texPanelMR,
                   {0, 0, (float)texPanelMR.width, (float)texPanelMR.height},
                   {rect.x + rect.width - cornerW, rect.y + cornerH,
                    (float)cornerW, rect.height - 2 * cornerH},
                   {0, 0}, 0.0f, WHITE);

    // Center
    DrawTexturePro(texPanelMC,
                   {0, 0, (float)texPanelMC.width, (float)texPanelMC.height},
                   {rect.x + cornerW, rect.y + cornerH,
                    rect.width - 2 * cornerW, rect.height - 2 * cornerH},
                   {0, 0}, 0.0f, WHITE);
  };

  // Helper for textured buttons
  auto DrawTexturedButton = [&](Texture2D tex, Rectangle rect, bool isSelected,
                                bool isHover) {
    // Draw Base Texture (boto003)
    DrawTexturePro(tex, {0, 0, (float)tex.width, (float)tex.height}, rect,
                   {0, 0}, 0.0f, WHITE);

    if (isSelected) {
      DrawRectangleLinesEx(rect, 3, YELLOW);
    } else if (isHover) {
      DrawRectangleLinesEx(rect, 2, WHITE);
    }
  };

  Vector2 mousePos = GetMousePosition();

  // Full Taskbar Area (Tabs + Toolbar) - Encapsulated
  Rectangle fullTaskbarRect = {
      0, (float)SCREEN_HEIGHT - TOOLBAR_HEIGHT - TAB_HEIGHT,
      (float)SCREEN_WIDTH, (float)TOOLBAR_HEIGHT + TAB_HEIGHT};
  DrawNineSlicePanel(fullTaskbarRect);

  Rectangle tabArea = {0, fullTaskbarRect.y, (float)SCREEN_WIDTH,
                       (float)TAB_HEIGHT};

  const char *tabNames[] = {"Terrains", "Nature", "Rocks", "Creatures", "..."};
  for (int i = 0; i < 5; i++) {
    float tabW = 170;
    Rectangle tabRect = {i * tabW + 5, tabArea.y + 5, tabW - 5,
                         tabArea.height - 5};
    bool isHover = CheckCollisionPointRec(GetMousePosition(), tabRect);
    bool isActive = ((int)currentTab == i);

    // Use texTabButton for Tabs
    DrawTexturedButton(texTabButton, tabRect, isActive, isHover);

    // Centered Text
    Vector2 textSize = MeasureTextEx(uiFont, tabNames[i], 20, 2); // Size 20
    Vector2 textPos = {tabRect.x + (tabRect.width - textSize.x) / 2,
                       tabRect.y + (tabRect.height - textSize.y) / 2 -
                           5}; // Raised by 5px

    DrawTextEx(uiFont, tabNames[i], textPos, 20, 2,
               isActive ? WHITE : LIGHTGRAY);

    if (isHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !showBrushPopup) {
      if (currentTab != (UIState)i) {
        currentTab = (UIState)i;
        selectedToolIndex = 0;
        scrollOffset = 0.0f; // Reset scroll on tab change
      }
    }
  }

  // Content based on Tab
  float startX = 25;
  float visibleWidth = SCREEN_WIDTH - 50; // Padding on sides
  float startY =
      SCREEN_HEIGHT - TOOLBAR_HEIGHT + (TOOLBAR_HEIGHT - 80) / 2; // Centered
  float btnSize = 80;
  float padding = 12;

  // SCROLLING LOGIC
  // Only apply scrolling if we are in a tool tab (Terrain, Nature, Creatures)
  // Settings tab has its own layout.
  if (currentTab == UIState::Terrain || currentTab == UIState::Creatures ||
      currentTab == UIState::Nature) {

    int itemCount = 0;
    if (currentTab == UIState::Terrain)
      itemCount = 9;
    else if (currentTab == UIState::Creatures)
      itemCount = 11; // Updated Boar

    if (itemCount > 0) {
      float totalContentWidth =
          itemCount * (btnSize + padding) - padding; // Remove last padding
      float maxScrollVal = std::max(0.0f, totalContentWidth - visibleWidth);

      // Input
      // Check if mouse is over the toolbar area to enable scrolling
      if (mousePos.y > SCREEN_HEIGHT - TOOLBAR_HEIGHT) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
          scrollOffset -= wheel * 40.0f; // Scroll speed
          if (scrollOffset < 0)
            scrollOffset = 0;
          if (scrollOffset > maxScrollVal)
            scrollOffset = maxScrollVal;
        }
      }
    }
  }

  // Define Scissor Area
  // We want to clip content that flows outside startX -> startX + visibleWidth
  BeginScissorMode((int)startX, (int)(SCREEN_HEIGHT - TOOLBAR_HEIGHT),
                   (int)visibleWidth, TOOLBAR_HEIGHT);

  // Arrays for rendering buttons...

  int numTools = 0;

  // Arrays for rendering buttons. Max tools per tab approx 8-9
  if (currentTab == UIState::Terrain) {
    numTools = 9; // 8 types + eraser
    for (int i = 0; i < numTools; i++) {
      // Apply Scroll Offset to X position
      float btnX = startX + i * (btnSize + padding) - scrollOffset;

      // Optimization: Don't draw if completely out of view
      if (btnX + btnSize < startX || btnX > startX + visibleWidth)
        continue;

      Rectangle btnRect = {btnX, startY, btnSize, btnSize};
      if (i == 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        TraceLog(LOG_INFO,
                 "DEBUG: Button 0 Rect Y: %f, Mouse Y: %f, ScreenH: %d",
                 btnRect.y, GetMousePosition().y, SCREEN_HEIGHT);
      }

      // Check collision with the SCROLLED rect, but strictly within the visible
      // area logic Since we are optimizing above, this is mostly fine, but
      // let's ensure we don't click "invisible" buttons if the scissor didn't
      // work (it only clips drawing) CheckCollisionPointRec works on logic
      // coordinates. So detailed check: Mouse must be within the scissor area
      // AND the button rect.
      bool isHover =
          CheckCollisionPointRec(mousePos, btnRect) &&
          (mousePos.x >= startX && mousePos.x <= startX + visibleWidth);

      // Button Background
      // Button Background
      DrawTexturedButton(texButton, btnRect, selectedToolIndex == i, isHover);

      if (selectedToolIndex == i) {
        DrawRectangleLinesEx(btnRect, 3, YELLOW);
      }

      // Icon
      if (i < 8) {
        TileType types[] = {TileType::DeepOcean,    TileType::Ocean,
                            TileType::ShallowOcean, TileType::Sand,
                            TileType::Grass,        TileType::Forest,
                            TileType::Mountain,     TileType::Snow};

        Texture2D tex = {0};
        if (i == 0)
          tex = texIconWaterDeep;
        else if (i == 1)
          tex = texIconWaterOcean;
        else if (i == 2)
          tex = texIconWaterShallow;
        else
          tex = const_cast<World &>(world).GetTextureForUI(types[i]);

        if (tex.id > 0) {
          // Center and fit
          float availableSize = btnSize - 10;
          float scale = availableSize / (float)std::max(tex.width, tex.height);

          // Only floor if we are upscaling significantly (pixel art look)
          // If scaling down or close to 1, keep float precision
          if (scale > 1.0f)
            scale = std::floor(scale);

          float scaledW = tex.width * scale;
          float scaledH = tex.height * scale;

          // Center in button
          float offsetX = (btnSize - scaledW) / 2.0f;
          float offsetY = (btnSize - scaledH) / 2.0f;

          Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
          Rectangle dest = {btnRect.x + offsetX, btnRect.y + offsetY, scaledW,
                            scaledH};

          BeginShaderMode(circleMaskShader);
          DrawTexturePro(tex, src, dest, {0, 0}, 0.0f, WHITE);
          EndShaderMode();
        }
      } else {
        // Eraser Icon (Custom)
        if (texEraser.id > 0) {
          float availableSize = btnSize - 10;
          float rawScale = availableSize /
                           (float)std::max(texEraser.width, texEraser.height);
          float scale = rawScale; // Allow downscaling
          if (scale > 1.0f)
            scale = std::floor(scale);

          float scaledW = texEraser.width * scale;
          float scaledH = texEraser.height * scale;
          float offsetX = (btnSize - scaledW) / 2.0f;
          float offsetY = (btnSize - scaledH) / 2.0f;

          Rectangle src = {0, 0, (float)texEraser.width,
                           (float)texEraser.height};
          Rectangle dest = {btnRect.x + offsetX, btnRect.y + offsetY, scaledW,
                            scaledH};

          DrawTexturePro(texEraser, src, dest, {0, 0}, 0.0f, WHITE);
        }
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

      // Button Background
      DrawTexturedButton(texButton, btnRect, selectedToolIndex == i, isHover);

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
          // Standard scaling for nature shouldn't need floor logic if we trust
          // it, but let's keep it safe or use the new standard. Nature items
          // are usually small pixel art, so floor is good, but let's just use
          // the safer logic:
          if (scale > 1.0f)
            scale = std::floor(scale);

          Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
          Rectangle dest = {btnRect.x + 5, btnRect.y + 5, tex.width * scale,
                            tex.height * scale};
          DrawTexturePro(tex, src, dest, {0, 0}, 0.0f, WHITE);
        }
      } else {
        // Eraser Icon (Custom)
        if (texEraser.id > 0) {
          float availableSize = btnSize - 10;
          float rawScale = availableSize /
                           (float)std::max(texEraser.width, texEraser.height);
          float scale = rawScale;
          if (scale > 1.0f)
            scale = std::floor(scale);

          float scaledW = texEraser.width * scale;
          float scaledH = texEraser.height * scale;
          float offsetX = (btnSize - scaledW) / 2.0f;
          float offsetY = (btnSize - scaledH) / 2.0f;

          Rectangle src = {0, 0, (float)texEraser.width,
                           (float)texEraser.height};
          Rectangle dest = {btnRect.x + offsetX, btnRect.y + offsetY, scaledW,
                            scaledH};

          DrawTexturePro(texEraser, src, dest, {0, 0}, 0.0f, WHITE);
        }
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

      DrawTexturedButton(texButton, btnRect, selectedToolIndex == i, isHover);

      if (selectedToolIndex == i) {
        DrawRectangleLinesEx(btnRect, 3, YELLOW);
      }

      if (i < 2) {
        DecorationType decs[] = {DecorationType::Rock, DecorationType::BigRock};
        Texture2D tex = const_cast<World &>(world).GetTextureForUI(decs[i]);
        if (tex.id > 0) {
          float scale =
              std::min((btnSize - 10) / tex.width, (btnSize - 10) / tex.height);
          // Standard scaling for nature shouldn't need floor logic if we trust
          // it, but let's keep it safe or use the new standard. Nature items
          // are usually small pixel art, so floor is good, but let's just use
          // the safer logic:
          if (scale > 1.0f)
            scale = std::floor(scale);

          Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
          Rectangle dest = {btnRect.x + 5, btnRect.y + 5, tex.width * scale,
                            tex.height * scale};
          DrawTexturePro(tex, src, dest, {0, 0}, 0.0f, WHITE);
        }
      } else {
        // Eraser Icon (Custom)
        if (texEraser.id > 0) {
          float availableSize = btnSize - 10;
          float rawScale = availableSize /
                           (float)std::max(texEraser.width, texEraser.height);
          float scale = rawScale;
          if (scale > 1.0f)
            scale = std::floor(scale);

          float scaledW = texEraser.width * scale;
          float scaledH = texEraser.height * scale;
          float offsetX = (btnSize - scaledW) / 2.0f;
          float offsetY = (btnSize - scaledH) / 2.0f;

          Rectangle src = {0, 0, (float)texEraser.width,
                           (float)texEraser.height};
          Rectangle dest = {btnRect.x + offsetX, btnRect.y + offsetY, scaledW,
                            scaledH};

          DrawTexturePro(texEraser, src, dest, {0, 0}, 0.0f, WHITE);
        }
      }

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHover &&
          !showBrushPopup) {
        TraceLog(LOG_INFO, "UI: Clicked Tool Index %d", i);
        selectedToolIndex = i;
      }
    }
  } else if (currentTab == UIState::Creatures) {
    numTools = 11; // Updated count (Added Boar)
    EntityType creatureTypes[] = {
        EntityType::HumanUnarmed, EntityType::HumanArmed,
        EntityType::Boar, // Added Boar
        EntityType::Cow,          EntityType::Chicken,    EntityType::Sheep,
        EntityType::Bull,         EntityType::Chicken2,   EntityType::Lamb,
        EntityType::Pig,          EntityType::Turkey};

    for (int i = 0; i < numTools; i++) {
      // Apply Scroll Offset to X position
      float btnX = startX + i * (btnSize + padding) - scrollOffset;

      // Optimization: Don't draw if completely out of view
      if (btnX + btnSize < startX || btnX > startX + visibleWidth)
        continue;

      Rectangle btnRect = {btnX, startY, btnSize, btnSize};

      // Check collision with the SCROLLED rect, but strictly within the visible
      // area logic
      bool isHover =
          CheckCollisionPointRec(mousePos, btnRect) &&
          (mousePos.x >= startX && mousePos.x <= startX + visibleWidth);

      DrawTexturedButton(texButton, btnRect, selectedToolIndex == i, isHover);

      if (selectedToolIndex == i)
        DrawRectangleLinesEx(btnRect, 3, YELLOW);

      // Draw Creature Icon
      Texture2D tex =
          const_cast<World &>(world).GetTextureForUI(creatureTypes[i]);
      if (tex.id > 0) {
        // Center and fit
        float availableSize =
            btnSize - 4; // Reduced padding to allow 1.5x scale (76px available
                         // > 72px needed)
        float scale = availableSize / (float)std::max(tex.width, tex.height);
        if (scale > 1.0f)
          scale = std::floor(scale * 2.0f) /
                  2.0f; // Allow 0.5x increments (e.g. 1.5x)

        float scaledW = tex.width * scale;
        float scaledH = tex.height * scale;

        Rectangle src = {0, 0, (float)tex.width, (float)tex.height};

        if (creatureTypes[i] == EntityType::HumanArmed) {
          // Define a crop area (e.g., 32x36 centered horizontally,
          // bottom-aligned but cutting off footer variance)
          // Original: 64x64. Center X = 32. Bottom Y = 64.
          // Crop W=32, H=36. X = 16, Y = 14.
          // This range [14, 50] captures the body/head while cutting empty
          // space, allowing 'scale' to reach 2.0x within 74px height.
          src = {16, 14, 32, 36};

          // Recalculate scale for the CROPPED size
          float availableSize = btnSize - 6; // Padding
          scale = availableSize / (float)std::max(src.width, src.height);
          if (scale > 1.0f)
            scale = std::floor(scale * 2.0f) / 2.0f;

          scaledW = src.width * scale;
          scaledH = src.height * scale;
        }

        // Center in button
        float offsetX = (btnSize - scaledW) / 2.0f;
        float offsetY = (btnSize - scaledH) / 2.0f;

        Rectangle dest = {btnRect.x + offsetX, btnRect.y + offsetY, scaledW,
                          scaledH};
        DrawTexturePro(tex, src, dest, {0, 0}, 0.0f, WHITE);
      }

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHover &&
          !showBrushPopup) {
        selectedToolIndex = i;
      }
    }
  }

  EndScissorMode(); // End Clipping

  if (currentTab == UIState::Settings) {
    // === HOME / SETTINGS TAB ===

    // 1. Brush Size Control (Moved here)
    DrawTextEx(uiFont, "Brush Size:", {(float)startX, (float)startY}, 20, 1,
               WHITE);

    float toggleW = 120;
    Rectangle toggleRect = {startX, startY + 30, toggleW, 40}; // Below label
    bool isToggleHover = CheckCollisionPointRec(mousePos, toggleRect);

    DrawTexturedButton(texButton, toggleRect, false, isToggleHover);

    const char *sizeNames[] = {"Single", "Small", "Medium", "Large", "X-Large"};
    int sizeIndex = 0;
    if (currentBrushSize == BrushSize::S)
      sizeIndex = 1;
    else if (currentBrushSize == BrushSize::M)
      sizeIndex = 2;
    else if (currentBrushSize == BrushSize::L)
      sizeIndex = 3;
    else if (currentBrushSize == BrushSize::XL)
      sizeIndex = 4;

    // Draw Size Text inside button
    Vector2 szTextSize = MeasureTextEx(uiFont, sizeNames[sizeIndex], 20, 1);
    DrawTextEx(uiFont, sizeNames[sizeIndex],
               {toggleRect.x + (toggleRect.width - szTextSize.x) / 2,
                toggleRect.y + (toggleRect.height - szTextSize.y) / 2},
               20, 1, WHITE);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isToggleHover) {
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

    // 2. Settings Button (Placeholder)
    Rectangle setBtnRect = {startX + 150, startY + 30, 120, 40};
    bool isSetHover = CheckCollisionPointRec(mousePos, setBtnRect);
    DrawTexturedButton(texButton, setBtnRect, false, isSetHover);

    Vector2 setTextSize = MeasureTextEx(uiFont, "Settings", 20, 1);
    DrawTextEx(uiFont, "Settings",
               {setBtnRect.x + (setBtnRect.width - setTextSize.x) / 2,
                setBtnRect.y + (setBtnRect.height - setTextSize.y) / 2},
               20, 1, WHITE);
  }
}
