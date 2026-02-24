#include "../core/SaveManager.h"
#include "../simulation/SimulationManager.h"
#include "UIManager.h"

void UIManager::UpdateMainMenu(World &world) {
  // Nothing to update for now, logic is in DrawMainMenu
}

void UIManager::DrawMainMenu(const World &world) {
  float sw = (float)GetScreenWidth();
  float sh = (float)GetScreenHeight();
  Vector2 mousePos = GetMousePosition();

  if (!isMainMenuNight) {
    // ==================== DAY VIEW ====================

    // Background (stretch to fill screen)
    if (texMenuDayBg.id > 0) {
      DrawTexturePro(
          texMenuDayBg,
          {0, 0, (float)texMenuDayBg.width, (float)texMenuDayBg.height},
          {0, 0, sw, sh}, {0, 0}, 0, WHITE);
    } else {
      ClearBackground(GetColor(0x87CEEBFF));
    }

    // Border (stretch to fill screen, decorative overlay)
    if (texMenuDayBorder.id > 0) {
      DrawTexturePro(
          texMenuDayBorder,
          {0, 0, (float)texMenuDayBorder.width, (float)texMenuDayBorder.height},
          {0, 0, sw, sh}, {0, 0}, 0, WHITE);
    }

    // Logo + Illustration layout
    // Logo on the left, illustration on the right, above the buttons
    float logoScale = 0.55f;
    float logoW = texMenuLogo.width * logoScale;
    float logoH = texMenuLogo.height * logoScale;
    float ilustScale = 0.45f;
    float ilustW = texMenuIlustration.width * ilustScale;
    float ilustH = texMenuIlustration.height * ilustScale;

    // Center the logo+illustration block horizontally
    float blockW = logoW + ilustW - 20;
    float blockX = (sw - blockW) / 2;
    float logoY = sh * 0.08f;

    if (texMenuLogo.id > 0) {
      DrawTexturePro(
          texMenuLogo,
          {0, 0, (float)texMenuLogo.width, (float)texMenuLogo.height},
          {blockX, logoY, logoW, logoH}, {0, 0}, 0, WHITE);
    }
    if (texMenuIlustration.id > 0) {
      DrawTexturePro(texMenuIlustration,
                     {0, 0, (float)texMenuIlustration.width,
                      (float)texMenuIlustration.height},
                     {blockX + logoW - 20, logoY - 10, ilustW, ilustH}, {0, 0},
                     0, WHITE);
    }

    // --- Buttons (centered, stacked vertically) ---
    float btnScale = 0.5f;
    float btnW = texMenuBtnStart.width * btnScale;
    float btnH = texMenuBtnStart.height * btnScale;
    float btnX = (sw - btnW) / 2;
    float btnStartY =
        logoY + logoH + 180; // Push buttons much lower to clear illustration
    float btnGap = 10;

    // START GAME button
    Rectangle startRect = {btnX, btnStartY, btnW, btnH};
    bool startHover = CheckCollisionPointRec(mousePos, startRect);
    if (texMenuBtnStart.id > 0) {
      DrawTexturePro(
          texMenuBtnStart,
          {0, 0, (float)texMenuBtnStart.width, (float)texMenuBtnStart.height},
          startRect, {0, 0}, 0, startHover ? LIGHTGRAY : WHITE);
    }
    if (startHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !showOptionsPopup) {
      isMainMenuNight = true;
    }

    // OPTIONS button
    Rectangle optRect = {btnX, btnStartY + btnH + btnGap, btnW, btnH};
    bool optHover = CheckCollisionPointRec(mousePos, optRect);
    if (texMenuBtnOptions.id > 0) {
      DrawTexturePro(texMenuBtnOptions,
                     {0, 0, (float)texMenuBtnOptions.width,
                      (float)texMenuBtnOptions.height},
                     optRect, {0, 0}, 0, optHover ? LIGHTGRAY : WHITE);
    }
    // Options not implemented yet
    if (optHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !showOptionsPopup) {
      showOptionsPopup = !showOptionsPopup;
    }

    // EXIT button
    Rectangle exitRect = {btnX, btnStartY + 2 * (btnH + btnGap), btnW, btnH};
    bool exitHover = CheckCollisionPointRec(mousePos, exitRect);
    if (texMenuBtnExit.id > 0) {
      DrawTexturePro(
          texMenuBtnExit,
          {0, 0, (float)texMenuBtnExit.width, (float)texMenuBtnExit.height},
          exitRect, {0, 0}, 0, exitHover ? LIGHTGRAY : WHITE);
    }
    if (exitHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !showOptionsPopup) {
      shouldExitGame = true;
    }

  } else {
    // ==================== NIGHT VIEW (Save Slots) ====================

    // Night Background
    if (texMenuNightBg.id > 0) {
      DrawTexturePro(
          texMenuNightBg,
          {0, 0, (float)texMenuNightBg.width, (float)texMenuNightBg.height},
          {0, 0, sw, sh}, {0, 0}, 0, WHITE);
    } else {
      ClearBackground(GetColor(0x0a0a2eFF));
    }

    // Night Border
    if (texMenuNightBorder.id > 0) {
      DrawTexturePro(texMenuNightBorder,
                     {0, 0, (float)texMenuNightBorder.width,
                      (float)texMenuNightBorder.height},
                     {0, 0, sw, sh}, {0, 0}, 0, WHITE);
    }

    // Calculate grid dimensions first to center everything as a group
    const int COLS = 4;
    const int ROWS = 3;
    const int TOTAL = COLS * ROWS;
    float cellSize = 100.0f;
    float cellPad = 16.0f;
    float gridW = COLS * cellSize + (COLS - 1) * cellPad;
    float gridH = ROWS * cellSize + (ROWS - 1) * cellPad;
    float gridX = (sw - gridW) / 2;
    float gridY = (sh - gridH) / 2; // Centralizado verticalmente

    // Title (positioned relative to grid)
    const char *title = "SELECIONAR SAVE";
    Vector2 titleSz = MeasureTextEx(uiFont, title, 22, 1);
    DrawTextEx(uiFont, title, {(sw - titleSz.x) / 2, gridY - 60}, 22, 1, WHITE);

    bool openedActionThisFrame = false;
    bool openedConfirmThisFrame = false;

    for (int i = 0; i < TOTAL; i++) {
      int col = i % COLS;
      int row = i / COLS;
      int slotID = i + 1;

      float cx = gridX + col * (cellSize + cellPad);
      float cy = gridY + row * (cellSize + cellPad);
      Rectangle cellRect = {cx, cy, cellSize, cellSize};

      bool exists = SaveManager::SaveExists(slotID);
      bool isHover = CheckCollisionPointRec(mousePos, cellRect);

      // Cell background
      Color bgColor =
          exists ? ColorAlpha(DARKGREEN, 0.5f) : ColorAlpha(DARKGRAY, 0.4f);
      if (isHover)
        bgColor =
            exists ? ColorAlpha(GREEN, 0.5f) : ColorAlpha(LIGHTGRAY, 0.35f);
      DrawRectangleRec(cellRect, bgColor);
      DrawRectangleLinesEx(cellRect, 2, exists ? GOLD : GRAY);

      // Draw save icon if occupied
      if (exists && texSaveIcon.id > 0) {
        float maxIconSz = 42.0f;
        float aspectW = (float)texSaveIcon.width;
        float aspectH = (float)texSaveIcon.height;
        float scale = maxIconSz / std::max(aspectW, aspectH);
        float iconW = aspectW * scale;
        float iconH = aspectH * scale;
        Rectangle iSrc = {0, 0, aspectW, aspectH};
        Rectangle iDst = {cx + (cellSize - iconW) / 2, cy + 10, iconW, iconH};
        DrawTexturePro(texSaveIcon, iSrc, iDst, {0, 0}, 0.0f, WHITE);
      }

      // Slot number
      const char *numTxt = TextFormat("%d", slotID);
      Vector2 numSz = MeasureTextEx(uiFont, numTxt, 16, 1);
      float textY = exists ? cy + 58 : cy + (cellSize - numSz.y) / 2;
      DrawTextEx(uiFont, numTxt, {cx + (cellSize - numSz.x) / 2, textY}, 16, 1,
                 WHITE);

      // Status label
      if (exists) {
        const char *lbl = "Saved";
        Vector2 lblSz = MeasureTextEx(uiFont, lbl, 12, 1);
        DrawTextEx(uiFont, lbl, {cx + (cellSize - lblSz.x) / 2, cy + 78}, 12, 1,
                   GREEN);
      } else {
        const char *lbl = "New Game";
        Vector2 lblSz = MeasureTextEx(uiFont, lbl, 11, 1);
        DrawTextEx(uiFont, lbl, {cx + (cellSize - lblSz.x) / 2, cy + 75}, 11, 1,
                   LIGHTGRAY);
      }

      // Click logic
      if (isHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
          !showSaveActionPopup && !showDeleteConfirmPopup) {
        if (exists) {
          // Open Action Popup (Load / Delete)
          showSaveActionPopup = true;
          actionSlotID = slotID;
          openedActionThisFrame = true;
        } else {
          // Open World Creator Popup
          TraceLog(LOG_INFO, "MainMenu: Opening Creator for slot %d", slotID);
          showWorldCreatorPopup = true;
          creatingSlotID = slotID;
          openedActionThisFrame = true;
        }
      }
    }

    // "Back" button at bottom
    float voltarW = 160, voltarH = 36;
    Rectangle voltarBtn = {(sw - voltarW) / 2, gridY + gridH + 25, voltarW,
                           voltarH};
    bool voltarHover = CheckCollisionPointRec(mousePos, voltarBtn);

    // Use texButton for voltar
    Rectangle btnSrc = {0, 0, (float)texButton.width, (float)texButton.height};
    DrawTexturePro(texButton, btnSrc, voltarBtn, {0, 0}, 0.0f,
                   voltarHover ? LIGHTGRAY : WHITE);
    Vector2 voltarSz = MeasureTextEx(uiFont, "Back", 14, 1);
    DrawTextEx(uiFont, "Back",
               {voltarBtn.x + (voltarW - voltarSz.x) / 2,
                voltarBtn.y + (voltarH - voltarSz.y) / 2},
               14, 1, WHITE);

    if (voltarHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !showSaveActionPopup && !showDeleteConfirmPopup &&
        !showWorldCreatorPopup) {
      isMainMenuNight = false;
    }

    // --- SAVE ACTION POPUP (Carregar ou Excluir) ---
    if (showSaveActionPopup && actionSlotID > 0) {
      // Dim background slightly
      DrawRectangle(0, 0, (int)sw, (int)sh, ColorAlpha(BLACK, 0.4f));

      float dw = 260, dh = 140;
      float dx = (sw - dw) / 2;
      float dy = (sh - dh) / 2;
      DrawRectangle((int)dx, (int)dy, (int)dw, (int)dh,
                    ColorAlpha(GetColor(0x2E1A1AFF), 0.95f)); // Dark red-brown
      DrawRectangleLinesEx({dx, dy, dw, dh}, 2, GOLD);

      const char *msg = TextFormat("Action for Save %d", actionSlotID);
      Vector2 msgSz = MeasureTextEx(uiFont, msg, 16, 1);
      DrawTextEx(uiFont, msg, {dx + (dw - msgSz.x) / 2, dy + 15}, 16, 1, GOLD);

      float btnW = 200, btnH = 28, btnX = dx + (dw - btnW) / 2;

      // Carregar button
      Rectangle loadBtn = {btnX, dy + 45, btnW, btnH};
      bool loadHover = CheckCollisionPointRec(mousePos, loadBtn);
      DrawRectangleRec(loadBtn, loadHover ? ColorAlpha(BLUE, 0.8f)
                                          : ColorAlpha(BLUE, 0.5f));
      DrawRectangleLinesEx(loadBtn, 1, WHITE);
      Vector2 loadSz = MeasureTextEx(uiFont, "Load Game", 14, 1);
      DrawTextEx(uiFont, "Load Game",
                 {loadBtn.x + (btnW - loadSz.x) / 2, loadBtn.y + 7}, 14, 1,
                 WHITE);

      // Excluir button
      Rectangle delBtn = {btnX, dy + 80, btnW, btnH};
      bool delHover = CheckCollisionPointRec(mousePos, delBtn);
      DrawRectangleRec(delBtn, delHover ? ColorAlpha(RED, 0.8f)
                                        : ColorAlpha(RED, 0.5f));
      DrawRectangleLinesEx(delBtn, 1, WHITE);
      Vector2 delSz = MeasureTextEx(uiFont, "Delete Save", 14, 1);
      DrawTextEx(uiFont, "Delete Save",
                 {delBtn.x + (btnW - delSz.x) / 2, delBtn.y + 7}, 14, 1, WHITE);

      // Cancel click outside
      if (!openedActionThisFrame && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (loadHover) {
          TraceLog(LOG_INFO, "MainMenu: Loading slot %d", actionSlotID);
          SaveManager::LoadGame(actionSlotID, const_cast<World &>(world));
          currentState = GameState::PLAYING;
          shouldStartGame = true;
          showSaveActionPopup = false;
        } else if (delHover) {
          showSaveActionPopup = false;
          showDeleteConfirmPopup = true; // Open confirmation
          openedConfirmThisFrame = true;
        } else if (!CheckCollisionPointRec(mousePos, {dx, dy, dw, dh})) {
          showSaveActionPopup = false;
        }
      }
    }

    // --- DELETE CONFIRMATION POPUP ---
    if (showDeleteConfirmPopup && actionSlotID > 0) {
      // Dim background heavily
      DrawRectangle(0, 0, (int)sw, (int)sh, ColorAlpha(BLACK, 0.6f));

      float dw = 300, dh = 150;
      float dx = (sw - dw) / 2;
      float dy = (sh - dh) / 2;
      DrawRectangle((int)dx, (int)dy, (int)dw, (int)dh,
                    ColorAlpha(DARKBROWN, 0.95f));
      DrawRectangleLinesEx({dx, dy, dw, dh}, 2, RED);

      const char *msg1 = "Are you sure you want to";
      const char *msg2 = TextFormat("DELETE the Save %d?", actionSlotID);
      Vector2 msg1Sz = MeasureTextEx(uiFont, msg1, 16, 1);
      Vector2 msg2Sz = MeasureTextEx(uiFont, msg2, 16, 1);

      DrawTextEx(uiFont, msg1, {dx + (dw - msg1Sz.x) / 2, dy + 20}, 16, 1,
                 WHITE);
      DrawTextEx(uiFont, msg2, {dx + (dw - msg2Sz.x) / 2, dy + 40}, 16, 1, RED);

      float btnW = 120, btnH = 30;

      // Sim (Delete)
      Rectangle yesBtn = {dx + 20, dy + 90, btnW, btnH};
      bool yesHover = CheckCollisionPointRec(mousePos, yesBtn);
      DrawRectangleRec(yesBtn, yesHover ? ColorAlpha(RED, 0.9f)
                                        : ColorAlpha(RED, 0.6f));
      DrawRectangleLinesEx(yesBtn, 1, WHITE);
      Vector2 yesSz = MeasureTextEx(uiFont, "Yes, Delete", 14, 1);
      DrawTextEx(uiFont, "Yes, Delete",
                 {yesBtn.x + (btnW - yesSz.x) / 2, yesBtn.y + 8}, 14, 1, WHITE);

      // Nao (Cancel)
      Rectangle noBtn = {dx + dw - btnW - 20, dy + 90, btnW, btnH};
      bool noHover = CheckCollisionPointRec(mousePos, noBtn);
      DrawRectangleRec(noBtn, noHover ? ColorAlpha(GRAY, 0.9f)
                                      : ColorAlpha(GRAY, 0.6f));
      DrawRectangleLinesEx(noBtn, 1, WHITE);
      Vector2 noSz = MeasureTextEx(uiFont, "Cancel", 14, 1);
      DrawTextEx(uiFont, "Cancel", {noBtn.x + (btnW - noSz.x) / 2, noBtn.y + 8},
                 14, 1, WHITE);

      if (!openedConfirmThisFrame && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (yesHover) {
          TraceLog(LOG_INFO, "MainMenu: Deleting slot %d", actionSlotID);
          SaveManager::DeleteSave(
              actionSlotID); // Assume this method exists or we need to add it!
          showDeleteConfirmPopup = false;
        } else if (noHover ||
                   !CheckCollisionPointRec(mousePos, {dx, dy, dw, dh})) {
          showDeleteConfirmPopup = false; // Just close
        }
      }
    }

    // Draw World Creator Popup
    if (showWorldCreatorPopup && creatingSlotID > 0) {
      if (texCreatorBg.id > 0) {
        Rectangle bgRect = {0, 0, sw, sh};
        DrawTexturePro(
            texCreatorBg,
            {0, 0, (float)texCreatorBg.width, (float)texCreatorBg.height},
            bgRect, {0, 0}, 0.0f, WHITE);
      } else {
        DrawRectangle(0, 0, (int)sw, (int)sh, ColorAlpha(BLACK, 0.9f));
      }

      // World creation title
      const char *title = "WORLD CREATOR";
      Vector2 tsz = MeasureTextEx(uiFont, title, 30, 1);
      DrawTextEx(uiFont, title, {(sw - tsz.x) / 2, 50}, 30, 1, WHITE);

      // Planet Animation
      planetAnimTimer += GetFrameTime();
      if (planetAnimTimer >= 0.05f) { // 20 FPS (approx)
        planetAnimTimer = 0.0f;
        planetAnimFrame++;
        if (texPlanetAnim.size() > 0 &&
            planetAnimFrame >= texPlanetAnim.size()) {
          planetAnimFrame = 0;
        }
      }

      if (texPlanetAnim.size() > 0 && planetAnimFrame >= 0 &&
          planetAnimFrame < texPlanetAnim.size()) {
        Texture2D currentPlanetFrame = texPlanetAnim[planetAnimFrame];
        // Draw big planet on the left side
        float pSize = sw * 0.4f;
        Rectangle pSrc = {0, 0, (float)currentPlanetFrame.width,
                          (float)currentPlanetFrame.height};
        Rectangle pDest = {sw * 0.1f, (sh - pSize) / 2, pSize, pSize};
        DrawTexturePro(currentPlanetFrame, pSrc, pDest, {0, 0}, 0.0f, WHITE);
      }

      // Right Side: Configurations
      float panelX = sw * 0.55f;
      float panelY = sh * 0.3f;
      float panelW = sw * 0.35f;

      DrawTextEx(uiFont, "World Size", {panelX, panelY}, 20, 1, GOLD);

      // Size Buttons
      const char *sizes[3] = {"Small (64)", "Medium (128)", "Large (256)"};
      for (int i = 0; i < 3; i++) {
        Rectangle sBtn = {panelX, panelY + 30 + (i * 40), panelW * 0.8f, 32};
        bool sHover = CheckCollisionPointRec(mousePos, sBtn);

        Color baseCol = (newWorldBaseSize == i) ? ORANGE : DARKGRAY;
        DrawRectangleRec(sBtn, sHover ? ColorAlpha(baseCol, 0.8f) : baseCol);
        DrawRectangleLinesEx(sBtn, 1, WHITE);

        Vector2 sSz = MeasureTextEx(uiFont, sizes[i], 16, 1);
        DrawTextEx(uiFont, sizes[i],
                   {sBtn.x + (sBtn.width - sSz.x) / 2,
                    sBtn.y + (sBtn.height - sSz.y) / 2},
                   16, 1, WHITE);

        if (sHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          newWorldBaseSize = i;
        }
      }

      // Seed Input
      DrawTextEx(uiFont, "Seed (Optional)", {panelX, panelY + 160}, 18, 1,
                 GOLD);
      Rectangle seedBox = {panelX, panelY + 185, panelW * 0.8f, 32};
      bool seedHover = CheckCollisionPointRec(mousePos, seedBox);

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        isSeedInputActive = seedHover;
      }

      DrawRectangleRec(seedBox, isSeedInputActive ? ColorAlpha(DARKBLUE, 0.8f)
                                                  : ColorAlpha(DARKGRAY, 0.8f));
      DrawRectangleLinesEx(seedBox, 1, isSeedInputActive ? ORANGE : WHITE);

      if (isSeedInputActive) {
        int key = GetCharPressed();
        while (key > 0) {
          // Only allow numbers for the seed, up to 9 characters (safe for
          // 32-bit uint)
          if ((key >= 48 && key <= 57) && (strlen(newWorldSeedInput) < 9)) {
            int len = strlen(newWorldSeedInput);
            newWorldSeedInput[len] = (char)key;
            newWorldSeedInput[len + 1] = '\0';
          }
          key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
          int len = strlen(newWorldSeedInput);
          if (len > 0)
            newWorldSeedInput[len - 1] = '\0';
        }
      }

      const char *seedDisplayText =
          (strlen(newWorldSeedInput) == 0 && !isSeedInputActive)
              ? "Aleatoria..."
              : newWorldSeedInput;
      Vector2 sdSz = MeasureTextEx(uiFont, seedDisplayText, 16, 1);
      DrawTextEx(
          uiFont, seedDisplayText,
          {seedBox.x + 10, seedBox.y + (seedBox.height - sdSz.y) / 2}, 16, 1,
          (strlen(newWorldSeedInput) == 0 && !isSeedInputActive) ? GRAY
                                                                 : WHITE);

      // Add feedback text
      bool hasValidSeedLen =
          strlen(newWorldSeedInput) == 0 || strlen(newWorldSeedInput) == 9;
      if (!hasValidSeedLen) {
        DrawTextEx(uiFont, "A seed precisa ter exatamente 9 digitos!",
                   {seedBox.x, seedBox.y + seedBox.height + 2}, 13, 1, RED);
      }

      // Create Button
      float genW = 200, genH = 40;
      Rectangle genBtn = {panelX + (panelW * 0.8f - genW) / 2, panelY + 240,
                          genW, genH};
      bool genHover = CheckCollisionPointRec(mousePos, genBtn);

      DrawRectangleRec(genBtn,
                       genHover && hasValidSeedLen
                           ? ColorAlpha(GREEN, 0.9f)
                           : ColorAlpha(GREEN, hasValidSeedLen ? 0.6f : 0.2f));
      DrawRectangleLinesEx(genBtn, 2, hasValidSeedLen ? WHITE : GRAY);
      Vector2 genSz = MeasureTextEx(uiFont, "GERAR E JOGAR", 18, 1);
      DrawTextEx(uiFont, "GERAR E JOGAR",
                 {genBtn.x + (genW - genSz.x) / 2, genBtn.y + 11}, 18, 1,
                 hasValidSeedLen ? WHITE : GRAY);

      if (genHover && hasValidSeedLen &&
          IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        TraceLog(LOG_INFO, "MainMenu: Generating World for slot %d",
                 creatingSlotID);
        int targetWidth = (newWorldBaseSize == 0)   ? 64
                          : (newWorldBaseSize == 1) ? 128
                                                    : 256;
        int targetHeight = targetWidth; // square for now

        uint32_t finalSeed = 0;
        if (strlen(newWorldSeedInput) > 0) {
          finalSeed = (uint32_t)std::stoul(newWorldSeedInput);
        } else {
          finalSeed =
              (uint32_t)(GetTime() * 1000.0) + GetRandomValue(0, 1000000);
        }

        const_cast<World &>(world).Reset(finalSeed);
        const_cast<World &>(world).ResizeAndGenerate(targetWidth, targetHeight);
        SaveManager::SaveGame(
            creatingSlotID,
            const_cast<World &>(
                world)); // Immediately initialize saving structure

        currentState = GameState::PLAYING;
        shouldStartGame = true;
        showWorldCreatorPopup = false;
        isMainMenuNight = false; // Reset main menu state for further back out
      }

      // Cancel / Return Button
      Rectangle backBtn = {10, 10, 100, 30};
      bool backHover = CheckCollisionPointRec(mousePos, backBtn);
      DrawRectangleRec(backBtn, backHover ? RED : MAROON);
      DrawTextEx(uiFont, "Back", {backBtn.x + 25, backBtn.y + 6}, 16, 1, WHITE);

      if (backHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        showWorldCreatorPopup = false;
      }
    }
  }

  // Draw Options Popup (over main menu)
  if (showOptionsPopup) {
    DrawOptionsPopup();
  }

  // Draw Cursor on top
  float cursorSc = 0.20f;
  DrawTextureEx(texCursor, mousePos, 0.0f, cursorSc, WHITE);
}
