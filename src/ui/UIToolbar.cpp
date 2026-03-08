#include "../simulation/SimulationManager.h"
#include "UIManager.h"
#include <algorithm>
#include <cmath>

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

  if (showHumanSpawnMenu) {
    // Move mouse offscreen for the underlying UI checks so it can't hover/click
    // them
    mousePos = {-1000.0f, -1000.0f};
  }

  // Full Taskbar Area (Tabs + Toolbar) - Encapsulated
  Rectangle fullTaskbarRect = {
      0, (float)getScreenH() - TOOLBAR_HEIGHT - TAB_HEIGHT, (float)getScreenW(),
      (float)TOOLBAR_HEIGHT + TAB_HEIGHT};
  DrawNineSlicePanel(fullTaskbarRect);

  Rectangle tabArea = {0, fullTaskbarRect.y, (float)getScreenW(),
                       (float)TAB_HEIGHT};

  const char *tabNames[] = {"Terrains",  "Nature", "Rocks",
                            "Creatures", "Social", "..."};
  for (int i = 0; i < 6; i++) {
    float tabW = 170;
    Rectangle tabRect = {i * tabW + 5, tabArea.y + 5, tabW - 5,
                         tabArea.height - 5};
    bool isHover = CheckCollisionPointRec(mousePos, tabRect);
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
  float visibleWidth = getScreenW() - 50; // Padding on sides
  float startY =
      getScreenH() - TOOLBAR_HEIGHT + (TOOLBAR_HEIGHT - 80) / 2; // Centered
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
      if (mousePos.y > getScreenH() - TOOLBAR_HEIGHT) {
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
  // We want to clip content that flows outside startX -> startX +
  // visibleWidth
  BeginScissorMode((int)startX, (int)(getScreenH() - TOOLBAR_HEIGHT),
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
                 btnRect.y, mousePos.y, getScreenH());
      }

      // Check collision with the SCROLLED rect, but strictly within the
      // visible area logic Since we are optimizing above, this is mostly
      // fine, but let's ensure we don't click "invisible" buttons if the
      // scissor didn't work (it only clips drawing) CheckCollisionPointRec
      // works on logic coordinates. So detailed check: Mouse must be within
      // the scissor area AND the button rect.
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
  } else if (currentTab == UIState::Social) {
    numTools = 2; // "Cidades" + "Force War"
    const char *socialBtnNames[] = {"Cidades", "Guerra"};
    for (int i = 0; i < numTools; i++) {
      Rectangle btnRect = {startX + i * (btnSize + padding), startY, btnSize,
                           btnSize};
      bool isHover = CheckCollisionPointRec(mousePos, btnRect);

      bool isActive = (i == 1 && forceWarMode);
      DrawTexturedButton(texButton, btnRect, isActive, isHover);

      if (isActive)
        DrawRectangleLinesEx(btnRect, 3, RED);

      Vector2 ts = MeasureTextEx(uiFont, socialBtnNames[i], 18, 1);
      Color textColor = (i == 1 && forceWarMode) ? RED : WHITE;
      DrawTextEx(
          uiFont, socialBtnNames[i],
          {btnRect.x + (btnSize - ts.x) / 2, btnRect.y + (btnSize - ts.y) / 2},
          18, 1, textColor);

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHover &&
          !showBrushPopup) {
        if (i == 0) {
          showSocialCityList = !showSocialCityList;
        } else if (i == 1) {
          // Toggle Force War mode
          forceWarMode = !forceWarMode;
          forceWarKingdomA = -1;
          forceWarKingdomB = -1;
          showForceWarConfirm = false;
          if (forceWarMode) {
            showSocialCityList = true; // Auto-open city list for selection
          }
        }
      }
    }
  } else if (currentTab == UIState::Creatures) {
    numTools = 10;
    EntityType creatureTypes[] = {EntityType::HumanUnarmed, EntityType::Boar,
                                  EntityType::Cow,          EntityType::Chicken,
                                  EntityType::Sheep,        EntityType::Bull,
                                  EntityType::Chicken2,     EntityType::Lamb,
                                  EntityType::Pig,          EntityType::Turkey};

    for (int i = 0; i < numTools; i++) {
      // Apply Scroll Offset to X position
      float btnX = startX + i * (btnSize + padding) - scrollOffset;

      // Optimization: Don't draw if completely out of view
      if (btnX + btnSize < startX || btnX > startX + visibleWidth)
        continue;

      Rectangle btnRect = {btnX, startY, btnSize, btnSize};

      // Check collision with the SCROLLED rect, but strictly within the
      // visible area logic
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
            btnSize - 4; // Reduced padding to allow 1.5x scale (76px
                         // available > 72px needed)
        float scale = availableSize / (float)std::max(tex.width, tex.height);
        if (scale > 1.0f)
          scale = std::floor(scale * 2.0f) /
                  2.0f; // Allow 0.5x increments (e.g. 1.5x)

        float scaledW = tex.width * scale;
        float scaledH = tex.height * scale;

        Rectangle src = {0, 0, (float)tex.width, (float)tex.height};

        // Center in button
        float offsetX = (btnSize - scaledW) / 2.0f;
        float offsetY = (btnSize - scaledH) / 2.0f;

        Rectangle dest = {btnRect.x + offsetX, btnRect.y + offsetY, scaledW,
                          scaledH};
        DrawTexturePro(tex, src, dest, {0, 0}, 0.0f, WHITE);
      }

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHover &&
          !showBrushPopup) {
        if (i == 0) {
          // Human button: open spawn menu popup
          showHumanSpawnMenu = !showHumanSpawnMenu;
          humanSpawnMenuPos = {btnRect.x, btnRect.y - 110};
          humanSpawnSelection = -1;
          popupJustOpened = true; // Prevent immediate close
        } else {
          selectedToolIndex = i;
          showHumanSpawnMenu = false;
        }
      }
    }
  }

  EndScissorMode(); // End Clipping

  // === HUMAN SPAWN MENU POPUP (outside scissor so it renders above toolbar)
  // ===
  if (showHumanSpawnMenu) {
    // 1. Draw a fullscreen invisible/darkened modal background to absorb clicks
    Rectangle fullscreenRec = {0, 0, (float)GetScreenWidth(),
                               (float)GetScreenHeight()};
    DrawRectangleRec(fullscreenRec, ColorAlpha(BLACK, 0.2f));

    float pmW = 160;
    float pmH = 105;
    float pmX = humanSpawnMenuPos.x;
    float pmY = humanSpawnMenuPos.y;

    // Background
    DrawRectangle(pmX, pmY, pmW, pmH, ColorAlpha(BLACK, 0.85f));
    DrawRectangleLinesEx({pmX, pmY, pmW, pmH}, 2, GOLD);

    // Title
    DrawTextEx(uiFont, "Spawn Human", {pmX + 10, pmY + 5}, 16, 1, GOLD);

    // Buttons
    const char *options[] = {"Random", "Man", "Woman"};
    bool anyOptionClicked = false;
    for (int opt = 0; opt < 3; opt++) {
      Rectangle optRect = {pmX + 5, pmY + 25 + opt * 27, pmW - 10, 24};
      Vector2 actualMousePos = GetMousePosition();
      bool optHover = CheckCollisionPointRec(actualMousePos, optRect);

      Color bgCol = optHover ? Color{80, 80, 120, 255} : Color{50, 50, 70, 255};
      DrawRectangleRec(optRect, bgCol);
      DrawRectangleLinesEx(optRect, 1, GRAY);

      Color textCol = optHover ? YELLOW : WHITE;
      Vector2 ts2 = MeasureTextEx(uiFont, options[opt], 14, 1);
      DrawTextEx(uiFont, options[opt],
                 {optRect.x + (optRect.width - ts2.x) / 2,
                  optRect.y + (optRect.height - ts2.y) / 2},
                 14, 1, textCol);

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && optHover) {
        humanSpawnSelection = opt;
        selectedToolIndex = 0;
        showHumanSpawnMenu = false;
        anyOptionClicked = true;
      }
    }

    // Close if clicked outside, but NOT on the frame it was just opened
    // and NOT if we just clicked an option
    if (!anyOptionClicked && !popupJustOpened &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(GetMousePosition(), {pmX, pmY, pmW, pmH})) {
      showHumanSpawnMenu = false;
    }
  }

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

    // 2. Time Control Button
    DrawTextEx(uiFont, "Time Control:", {startX + 150, (float)startY}, 20, 1,
               WHITE);

    Rectangle timeBtnRect = {startX + 150, startY + 30, 120, 40};
    bool isTimeBtnHover = CheckCollisionPointRec(mousePos, timeBtnRect);
    DrawTexturedButton(texButton, timeBtnRect, showTimePopup, isTimeBtnHover);

    // Show current speed or PAUSED
    TimeManager &tm = TimeManager::Get();
    const char *timeLabel =
        tm.IsPaused() ? "PAUSED" : TextFormat("%.0fx", tm.GetTimeScale());
    Vector2 timeLabelSize = MeasureTextEx(uiFont, timeLabel, 20, 1);
    DrawTextEx(uiFont, timeLabel,
               {timeBtnRect.x + (timeBtnRect.width - timeLabelSize.x) / 2,
                timeBtnRect.y + (timeBtnRect.height - timeLabelSize.y) / 2},
               20, 1, tm.IsPaused() ? RED : WHITE);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isTimeBtnHover) {
      showTimePopup = !showTimePopup;
      popupJustOpened = showTimePopup;
    }

    // 3. Save Menu Button (Standalone Icon)
    float iconSize = 64.0f;
    float iconX = startX + 300;
    float iconY = startY + 5.0f;
    Rectangle saveIconRect = {iconX, iconY, iconSize, iconSize};
    bool isSaveBtnHover = CheckCollisionPointRec(mousePos, saveIconRect);

    if (texSaveIcon.id > 0) {
      Rectangle src = {0, 0, (float)texSaveIcon.width,
                       (float)texSaveIcon.height};
      Color tint = isSaveBtnHover ? LIGHTGRAY : WHITE;
      DrawTexturePro(texSaveIcon, src, saveIconRect, {0, 0}, 0.0f, tint);
    } else {
      DrawRectangleRec(saveIconRect, isSaveBtnHover ? LIGHTGRAY : DARKGRAY);
      DrawTextEx(uiFont, "Save", {iconX + 10, iconY + 20}, 20, 1, WHITE);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isSaveBtnHover) {
      showSavePopup = !showSavePopup;
      if (showSavePopup && (savePopupPos.x == 0 && savePopupPos.y == 0)) {
        savePopupPos = {getScreenW() / 2.0f - 250, getScreenH() / 2.0f - 200};
      }
      popupJustOpened = showSavePopup;
    }

    // 4. Return to Main Menu Button
    Rectangle menuBtnRect = {iconX + iconSize + 20, startY + 5.0f, 160, 50};
    bool isMenuBtnHover = CheckCollisionPointRec(mousePos, menuBtnRect);
    DrawTexturedButton(texButton, menuBtnRect, false, isMenuBtnHover);
    const char *menuLabel = "Menu";
    Vector2 menuLabelSz = MeasureTextEx(uiFont, menuLabel, 18, 1);
    DrawTextEx(uiFont, menuLabel,
               {menuBtnRect.x + (menuBtnRect.width - menuLabelSz.x) / 2,
                menuBtnRect.y + (menuBtnRect.height - menuLabelSz.y) / 2},
               18, 1, WHITE);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isMenuBtnHover) {
      // CLEAR EVERYTHING TO AVOID DANGING POINTER CRASHES
      currentState = GameState::MAIN_MENU;
      isMainMenuNight = false;
      shouldStartGame = false;
      showSavePopup = false;
      showCityPopup = false;
      showHumanPopup = false;
      showSocialCityList = false;
      showBrushPopup = false;
      showTimePopup = false;
      showOptionsPopup = false;

      // Stop rendering any selected things
      selectedToolIndex = 0;
      popupCitizenID = -1;
      popupCityID = -1;
    }

    // 5. Options Button (next to Menu)
    Rectangle optBtnRect = {menuBtnRect.x + menuBtnRect.width + 10,
                            startY + 5.0f, 160, 50};
    bool isOptBtnHover = CheckCollisionPointRec(mousePos, optBtnRect);
    DrawTexturedButton(texButton, optBtnRect, false, isOptBtnHover);
    const char *optLabel = "Opções";
    Vector2 optLabelSz = MeasureTextEx(uiFont, optLabel, 18, 1);
    DrawTextEx(uiFont, optLabel,
               {optBtnRect.x + (optBtnRect.width - optLabelSz.x) / 2,
                optBtnRect.y + (optBtnRect.height - optLabelSz.y) / 2},
               18, 1, WHITE);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isOptBtnHover) {
      showOptionsPopup = !showOptionsPopup;
    }

    // 6. Seed Display (Subtle)
    const char *seedLabel = TextFormat("Seed: %u", world.GetSeed());
    DrawTextEx(uiFont, seedLabel,
               {optBtnRect.x + optBtnRect.width + 30, startY + 20}, 16, 1,
               DARKGRAY);
  }

  // === TIME POPUP (drawn above everything) ===
  if (showTimePopup) {
    TimeManager &tm = TimeManager::Get();

    // Popup dimensions
    float popupW = 360;
    float popupH = 120;
    float popupX = (getScreenW() - popupW) / 2;
    float popupY = getScreenH() - TOOLBAR_HEIGHT - TAB_HEIGHT - popupH - 20;

    // Background
    DrawRectangle(popupX, popupY, popupW, popupH, ColorAlpha(BLACK, 0.9f));
    DrawRectangleLinesEx({popupX, popupY, popupW, popupH}, 2, GOLD);

    // Title
    DrawTextEx(uiFont, "Time Control", {popupX + 10, popupY + 10}, 22, 1, GOLD);

    // Pause/Play Button
    Rectangle pauseBtn = {popupX + 10, popupY + 45, 80, 35};
    bool isPauseHover = CheckCollisionPointRec(mousePos, pauseBtn);
    DrawTexturedButton(texButton, pauseBtn, tm.IsPaused(), isPauseHover);

    const char *pauseText = tm.IsPaused() ? "Play" : "Pause";
    Vector2 pauseSize = MeasureTextEx(uiFont, pauseText, 18, 1);
    DrawTextEx(uiFont, pauseText,
               {pauseBtn.x + (pauseBtn.width - pauseSize.x) / 2,
                pauseBtn.y + (pauseBtn.height - pauseSize.y) / 2},
               18, 1, tm.IsPaused() ? GREEN : WHITE);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isPauseHover &&
        !popupJustOpened) {
      tm.TogglePause();
    }

    // Speed Buttons (1x - 5x)
    float speedBtnW = 45;
    float speedStartX = popupX + 100;
    for (int s = 1; s <= 5; s++) {
      Rectangle speedBtn = {speedStartX + (s - 1) * (speedBtnW + 5),
                            popupY + 45, speedBtnW, 35};
      bool isSpeedHover = CheckCollisionPointRec(mousePos, speedBtn);
      bool isSelected = (!tm.IsPaused() && (int)tm.GetTimeScale() == s);

      DrawTexturedButton(texButton, speedBtn, isSelected, isSpeedHover);

      const char *speedLabel = TextFormat("%dx", s);
      Vector2 speedSize = MeasureTextEx(uiFont, speedLabel, 16, 1);
      DrawTextEx(uiFont, speedLabel,
                 {speedBtn.x + (speedBtn.width - speedSize.x) / 2,
                  speedBtn.y + (speedBtn.height - speedSize.y) / 2},
                 16, 1, isSelected ? YELLOW : WHITE);

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isSpeedHover &&
          !popupJustOpened) {
        tm.SetTimeScale((float)s);
        if (tm.IsPaused())
          tm.SetPaused(false); // Unpause when selecting speed
      }
    }

    // Close button (X)
    Rectangle closeBtn = {popupX + popupW - 30, popupY + 5, 25, 25};
    bool isCloseHover = CheckCollisionPointRec(mousePos, closeBtn);
    DrawRectangleRec(closeBtn, isCloseHover ? RED : DARKGRAY);
    DrawTextEx(uiFont, "X", {closeBtn.x + 7, closeBtn.y + 2}, 18, 1, WHITE);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isCloseHover) {
      showTimePopup = false;
    }

    // Click outside popup closes it
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(mousePos, {popupX, popupY, popupW, popupH}) &&
        !popupJustOpened) {
      showTimePopup = false;
    }
  }

  // === SOCIAL CITY LIST POPUP ===
  if (showSocialCityList) {
    DrawSocialCityList(world);
  }

  popupJustOpened = false;
}

void UIManager::DrawSocialCityList(const World &world) {
  auto &sim = const_cast<World &>(world).GetSimulation();
  const auto &cities = sim.GetCities();

  // Popup Dimensions
  float w = 450;
  float h = 600;

  // Lazy Init Position
  if (socialPopupPos.x == 0 && socialPopupPos.y == 0) {
    socialPopupPos.x = (getScreenW() - w) / 2;
    socialPopupPos.y = (getScreenH() - h) / 2;
  }

  // Drag Logic
  Vector2 mousePos = GetMousePosition();
  Rectangle headerRect = {socialPopupPos.x, socialPopupPos.y, w, 40};

  if (CheckCollisionPointRec(mousePos, headerRect)) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      isDraggingSocial = true;
      dragOffset.x = mousePos.x - socialPopupPos.x;
      dragOffset.y = mousePos.y - socialPopupPos.y;
    }
  }

  if (isDraggingSocial) {
    socialPopupPos.x = mousePos.x - dragOffset.x;
    socialPopupPos.y = mousePos.y - dragOffset.y;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      isDraggingSocial = false;
    }
  }

  float x = socialPopupPos.x;
  float y = socialPopupPos.y;

  // 9-Slice Draw (Violet Popup)
  int cw = texPopup3TL.width;
  int ch = texPopup3TL.height;

  DrawTexture(texPopup3TL, x, y, WHITE);
  DrawTexture(texPopup3TR, x + w - cw, y, WHITE);
  DrawTexture(texPopup3BL, x, y + h - ch, WHITE);
  DrawTexture(texPopup3BR, x + w - cw, y + h - ch, WHITE);

  DrawTexturePro(texPopup3TC,
                 {0, 0, (float)texPopup3TC.width, (float)texPopup3TC.height},
                 {x + cw, y, w - 2 * cw, (float)ch}, {0, 0}, 0, WHITE);
  DrawTexturePro(texPopup3BC,
                 {0, 0, (float)texPopup3BC.width, (float)texPopup3BC.height},
                 {x + cw, y + h - ch, w - 2 * cw, (float)ch}, {0, 0}, 0, WHITE);
  DrawTexturePro(texPopup3ML,
                 {0, 0, (float)texPopup3ML.width, (float)texPopup3ML.height},
                 {x, y + ch, (float)cw, h - 2 * ch}, {0, 0}, 0, WHITE);
  DrawTexturePro(texPopup3MR,
                 {0, 0, (float)texPopup3MR.width, (float)texPopup3MR.height},
                 {x + w - cw, y + ch, (float)cw, h - 2 * ch}, {0, 0}, 0, WHITE);
  DrawTexturePro(texPopup3MC,
                 {0, 0, (float)texPopup3MC.width, (float)texPopup3MC.height},
                 {x + cw, y + ch, w - 2 * cw, h - 2 * ch}, {0, 0}, 0, WHITE);

  // Title
  const char *titleText =
      forceWarMode ? "Selecione Reinos para Guerra" : "Todas as Cidades";
  Color titleColor = forceWarMode ? RED : GOLD;
  Vector2 titleSize = MeasureTextEx(uiFont, titleText, 24, 1);
  DrawTextEx(uiFont, titleText, {x + (w - titleSize.x) / 2, y + 10}, 24, 1,
             titleColor);

  // Force War status indicator
  if (forceWarMode) {
    const char *statusText =
        (forceWarKingdomA == -1) ? "Clique no 1 reino" : "Clique no 2 reino";
    Vector2 stSize = MeasureTextEx(uiFont, statusText, 16, 1);
    DrawTextEx(uiFont, statusText, {x + (w - stSize.x) / 2, y + 35}, 16, 1,
               YELLOW);
  }

  // Close Button
  Rectangle closeBtn = {x + w - 30, y + 10, 20, 20};
  DrawText("X", (int)closeBtn.x + 5, (int)closeBtn.y + 2, 20, RED);

  if (CheckCollisionPointRec(mousePos, closeBtn) &&
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    showSocialCityList = false;
    isDraggingSocial = false;
  }

  // Scroll List Area
  float listY = y + 50;
  float listH = h - 70;
  float contentH = cities.size() * 50.0f; // 50px per city row

  // Scroll Input
  Rectangle listRect = {x + 10, listY, w - 20, listH};
  if (CheckCollisionPointRec(mousePos, listRect)) {
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
      socialCityListScroll -= wheel * 30.0f;
    }
  }

  // Clamp Scrolling
  float maxScroll = std::max(0.0f, contentH - listH);
  if (socialCityListScroll < 0)
    socialCityListScroll = 0;
  if (socialCityListScroll > maxScroll)
    socialCityListScroll = maxScroll;

  BeginScissorMode((int)listRect.x, (int)listRect.y, (int)listRect.width,
                   (int)listRect.height);

  float currentY = listY - socialCityListScroll;

  for (const auto &pair : cities) {
    const City &city = pair.second;
    if (!city.isAlive)
      continue;

    // Skip drawing if outside visual area
    if (currentY + 50 >= listY && currentY <= listY + listH) {
      Rectangle itemRect = {listRect.x + 5, currentY + 5, listRect.width - 10,
                            40};

      // Hover effect and click
      bool isItemHover = CheckCollisionPointRec(mousePos, itemRect);
      if (isItemHover) {
        DrawRectangleRec(itemRect, Fade(WHITE, 0.2f));
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          if (forceWarMode && city.kingdomID >= 0) {
            // Force War: select kingdoms
            if (forceWarKingdomA == -1) {
              forceWarKingdomA = city.kingdomID;
            } else if (city.kingdomID != forceWarKingdomA) {
              forceWarKingdomB = city.kingdomID;
              // Execute Force War!
              Kingdom *kA = sim.GetKingdom(forceWarKingdomA);
              Kingdom *kB = sim.GetKingdom(forceWarKingdomB);
              if (kA && kB) {
                kA->DeclareWar(forceWarKingdomB);
                kB->DeclareWar(forceWarKingdomA);
                TraceLog(LOG_INFO, "FORCE WAR: %s vs %s!", kA->name.c_str(),
                         kB->name.c_str());
              }
              forceWarMode = false;
              forceWarKingdomA = -1;
              forceWarKingdomB = -1;
            }
          } else {
            showCityPopup = true;
            popupCityID = city.id;
            isDraggingCity = false;
          }
        }
      } else {
        DrawRectangleRec(itemRect, Fade(BLACK, 0.3f));
      }
      DrawRectangleLinesEx(itemRect, 1, DARKGRAY);

      // Draw Flag if possessed
      if (city.flagID >= 0 &&
          city.flagID < (int)world.GetResourceManager().cityFlags.size()) {
        Texture2D flagTex = const_cast<World &>(world)
                                .GetResourceManager()
                                .cityFlags[city.flagID];
        DrawTexturePro(
            flagTex, {0, 0, (float)flagTex.width, (float)flagTex.height},
            {itemRect.x + 5, itemRect.y + 5, 30, 30}, {0, 0}, 0.0f, WHITE);
      }

      // Draw Info
      const char *infoLine =
          TextFormat("%s | Pop: %d | Ouro: %d", city.name.c_str(),
                     city.GetPopulation(), city.resources.gold);
      DrawTextEx(uiFont, infoLine, {itemRect.x + 45, itemRect.y + 12}, 18, 1,
                 WHITE);
    }
    currentY += 50;
  }

  EndScissorMode();
}
