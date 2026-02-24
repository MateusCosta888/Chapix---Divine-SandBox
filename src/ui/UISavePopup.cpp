#include "../core/AudioManager.h"
#include "../core/SaveManager.h"
#include "../simulation/SimulationManager.h"
#include "UIManager.h"
#include <algorithm>


void UIManager::DrawSavePopup(const World &world) {
  // Grid layout: 3 cols x 4 rows = 12 slots
  const int COLS = 3;
  const int ROWS = 4;
  const int TOTAL_SLOTS = COLS * ROWS;
  float cellSize = 70.0f;
  float cellPad = 12.0f;
  float marginX = 30.0f;
  float marginTop = 55.0f;
  float marginBottom = 25.0f;

  float w = marginX * 2 + COLS * cellSize + (COLS - 1) * cellPad;
  float h = marginTop + ROWS * cellSize + (ROWS - 1) * cellPad + marginBottom;
  float x = savePopupPos.x;
  float y = savePopupPos.y;

  Vector2 mousePos = GetMousePosition();
  Rectangle headerRect = {x, y, w, 40};

  // Drag Logic
  if (CheckCollisionPointRec(mousePos, headerRect)) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      isDraggingSave = true;
      dragOffset.x = mousePos.x - x;
      dragOffset.y = mousePos.y - y;
    }
  }
  if (isDraggingSave) {
    savePopupPos.x = mousePos.x - dragOffset.x;
    savePopupPos.y = mousePos.y - dragOffset.y;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      isDraggingSave = false;
    }
    x = savePopupPos.x;
    y = savePopupPos.y;
  }

  // --- DRAW 9-SLICE PERGAMINHO BACKGROUND ---
  int cw = texPergaminhoTL.width;
  int ch = texPergaminhoTL.height;

  if (texPergaminhoTL.id > 0) {
    // Corners
    DrawTexture(texPergaminhoTL, x, y, WHITE);
    DrawTexture(texPergaminhoTR, x + w - cw, y, WHITE);
    DrawTexture(texPergaminhoBL, x, y + h - ch, WHITE);
    DrawTexture(texPergaminhoBR, x + w - cw, y + h - ch, WHITE);
    // Edges
    DrawTexturePro(
        texPergaminhoTC,
        {0, 0, (float)texPergaminhoTC.width, (float)texPergaminhoTC.height},
        {x + cw, y, w - 2 * cw, (float)ch}, {0, 0}, 0, WHITE);
    DrawTexturePro(
        texPergaminhoBC,
        {0, 0, (float)texPergaminhoBC.width, (float)texPergaminhoBC.height},
        {x + cw, y + h - ch, w - 2 * cw, (float)ch}, {0, 0}, 0, WHITE);
    DrawTexturePro(
        texPergaminhoML,
        {0, 0, (float)texPergaminhoML.width, (float)texPergaminhoML.height},
        {x, y + ch, (float)cw, h - 2 * ch}, {0, 0}, 0, WHITE);
    DrawTexturePro(
        texPergaminhoMR,
        {0, 0, (float)texPergaminhoMR.width, (float)texPergaminhoMR.height},
        {x + w - cw, y + ch, (float)cw, h - 2 * ch}, {0, 0}, 0, WHITE);
    // Center fill
    DrawTexturePro(
        texPergaminhoMC,
        {0, 0, (float)texPergaminhoMC.width, (float)texPergaminhoMC.height},
        {x + cw, y + ch, w - 2 * cw, h - 2 * ch}, {0, 0}, 0, WHITE);
  } else {
    DrawRectangle(x, y, w, h, ColorAlpha(BLACK, 0.9f));
    DrawRectangleLines(x, y, w, h, GOLD);
  }

  // Title
  const char *titleText = "SAVE GAME";
  Vector2 titleSize = MeasureTextEx(uiFont, titleText, 16, 1);
  DrawTextEx(uiFont, titleText, {x + (w - titleSize.x) / 2, y + 18}, 16, 1,
             DARKBROWN);

  // Close Button
  Rectangle closeBtn = {x + w - 30, y + 12, 16, 16};
  DrawText("X", (int)closeBtn.x + 3, (int)closeBtn.y + 1, 16, MAROON);
  if (CheckCollisionPointRec(mousePos, closeBtn) &&
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    showSavePopup = false;
    isDraggingSave = false;
    showConfirmOverwrite = false;
    confirmSlot = -1;
  }

  // Draw Grid of Slots
  for (int i = 0; i < TOTAL_SLOTS; i++) {
    int col = i % COLS;
    int row = i / COLS;
    int slotID = i + 1;

    float cx = x + marginX + col * (cellSize + cellPad);
    float cy = y + marginTop + row * (cellSize + cellPad);
    Rectangle cellRect = {cx, cy, cellSize, cellSize};

    bool exists = SaveManager::SaveExists(slotID);
    bool isHover = CheckCollisionPointRec(mousePos, cellRect);

    // Cell background
    Color bgColor =
        exists ? ColorAlpha(DARKGREEN, 0.5f) : ColorAlpha(DARKGRAY, 0.5f);
    if (isHover)
      bgColor = exists ? ColorAlpha(GREEN, 0.4f) : ColorAlpha(LIGHTGRAY, 0.3f);
    DrawRectangleRec(cellRect, bgColor);
    DrawRectangleLinesEx(cellRect, 2, exists ? GOLD : GRAY);

    // Draw save icon if occupied (preserve aspect ratio)
    if (exists && texSaveIcon.id > 0) {
      float maxIconSz = 32.0f;
      float aspectW = (float)texSaveIcon.width;
      float aspectH = (float)texSaveIcon.height;
      float scale = maxIconSz / std::max(aspectW, aspectH);
      float iconW = aspectW * scale;
      float iconH = aspectH * scale;
      Rectangle iSrc = {0, 0, aspectW, aspectH};
      Rectangle iDst = {cx + (cellSize - iconW) / 2, cy + 6, iconW, iconH};
      DrawTexturePro(texSaveIcon, iSrc, iDst, {0, 0}, 0.0f, WHITE);
    }

    // Slot number
    const char *numTxt = TextFormat("%d", slotID);
    Vector2 numSz = MeasureTextEx(uiFont, numTxt, 12, 1);
    float textY = exists ? cy + 42 : cy + (cellSize - numSz.y) / 2;
    DrawTextEx(uiFont, numTxt, {cx + (cellSize - numSz.x) / 2, textY}, 12, 1,
               WHITE);

    // Status label
    if (exists) {
      const char *lbl = "Salvo";
      Vector2 lblSz = MeasureTextEx(uiFont, lbl, 10, 1);
      DrawTextEx(uiFont, lbl, {cx + (cellSize - lblSz.x) / 2, cy + 55}, 10, 1,
                 GREEN);
    }

    // Click logic
    if (isHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !showConfirmOverwrite) {
      if (exists) {
        // Show Load/Overwrite dialog
        showConfirmOverwrite = true;
        confirmSlot = slotID;
      } else {
        // Empty slot: save directly
        TraceLog(LOG_INFO, "Saving in slot %d", slotID);
        SaveManager::SaveGame(slotID, world);
      }
    }
  }

  // --- LOAD / OVERWRITE DIALOG ---
  if (showConfirmOverwrite && confirmSlot > 0) {
    // Dim background
    DrawRectangle(x, y, w, h, ColorAlpha(BLACK, 0.6f));

    // Dialog box
    float dw = 240, dh = 130;
    float dx = x + (w - dw) / 2;
    float dy = y + (h - dh) / 2;
    DrawRectangle(dx, dy, dw, dh, ColorAlpha(DARKBROWN, 0.95f));
    DrawRectangleLinesEx({dx, dy, dw, dh}, 2, GOLD);

    // Message
    const char *msg = TextFormat("Slot %d", confirmSlot);
    Vector2 msgSz = MeasureTextEx(uiFont, msg, 14, 1);
    DrawTextEx(uiFont, msg, {dx + (dw - msgSz.x) / 2, dy + 12}, 14, 1, WHITE);

    float btnW = 190, btnH = 26, btnX = dx + (dw - btnW) / 2;

    // Carregar (Load) button
    Rectangle loadBtn = {btnX, dy + 38, btnW, btnH};
    bool loadHover = CheckCollisionPointRec(mousePos, loadBtn);
    DrawRectangleRec(loadBtn, loadHover ? ColorAlpha(BLUE, 0.8f)
                                        : ColorAlpha(BLUE, 0.5f));
    DrawRectangleLinesEx(loadBtn, 1, WHITE);
    Vector2 loadSz = MeasureTextEx(uiFont, "Carregar", 12, 1);
    DrawTextEx(uiFont, "Carregar",
               {loadBtn.x + (btnW - loadSz.x) / 2, loadBtn.y + 6}, 12, 1,
               WHITE);

    // Substituir (Overwrite) button
    Rectangle overBtn = {btnX, dy + 68, btnW, btnH};
    bool overHover = CheckCollisionPointRec(mousePos, overBtn);
    DrawRectangleRec(overBtn, overHover ? ColorAlpha(ORANGE, 0.8f)
                                        : ColorAlpha(ORANGE, 0.5f));
    DrawRectangleLinesEx(overBtn, 1, WHITE);
    Vector2 overSz = MeasureTextEx(uiFont, "Substituir", 12, 1);
    DrawTextEx(uiFont, "Substituir",
               {overBtn.x + (btnW - overSz.x) / 2, overBtn.y + 6}, 12, 1,
               WHITE);

    // Cancelar button
    Rectangle cancelBtn = {btnX, dy + 98, btnW, btnH};
    bool cancelHover = CheckCollisionPointRec(mousePos, cancelBtn);
    DrawRectangleRec(cancelBtn, cancelHover ? ColorAlpha(RED, 0.8f)
                                            : ColorAlpha(RED, 0.5f));
    DrawRectangleLinesEx(cancelBtn, 1, WHITE);
    Vector2 cancelSz = MeasureTextEx(uiFont, "Cancelar", 12, 1);
    DrawTextEx(uiFont, "Cancelar",
               {cancelBtn.x + (btnW - cancelSz.x) / 2, cancelBtn.y + 6}, 12, 1,
               WHITE);

    // Click handlers
    if (loadHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      TraceLog(LOG_INFO, "Loading from slot %d", confirmSlot);
      SaveManager::LoadGame(confirmSlot, const_cast<World &>(world));
      showConfirmOverwrite = false;
      confirmSlot = -1;
      showSavePopup = false;
    }
    if (overHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      TraceLog(LOG_INFO, "Overwriting slot %d", confirmSlot);
      SaveManager::SaveGame(confirmSlot, world);
      showConfirmOverwrite = false;
      confirmSlot = -1;
    }
    if (cancelHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      showConfirmOverwrite = false;
      confirmSlot = -1;
    }
  }
}

void UIManager::DrawOptionsPopup() {
  float sw = (float)getScreenW();
  float sh = (float)getScreenH();
  Vector2 mousePos = GetMousePosition();

  // Dim background
  DrawRectangle(0, 0, (int)sw, (int)sh, ColorAlpha(BLACK, 0.5f));

  // Popup dimensions
  float popW = 420, popH = 280;
  float popX = (sw - popW) / 2;
  float popY = (sh - popH) / 2;

  // Draw popup background
  DrawRectangle((int)popX, (int)popY, (int)popW, (int)popH,
                ColorAlpha(GetColor(0x1a1a2eFF), 0.95f));
  DrawRectangleLinesEx({popX, popY, popW, popH}, 2, GOLD);

  // Title
  const char *title = "Opcoes";
  Vector2 titleSz = MeasureTextEx(uiFont, title, 22, 1);
  DrawTextEx(uiFont, title, {popX + (popW - titleSz.x) / 2, popY + 16}, 22, 1,
             GOLD);

  // === MUSIC VOLUME SLIDER ===
  float sliderX = popX + 40;
  float sliderY = popY + 70;
  float sliderW = popW - 80;
  float sliderH = 24;
  float labelY = sliderY - 24;

  // Label
  float currentVol = AudioManager::Get().GetMusicVolume();
  const char *volLabel =
      TextFormat("Volume da Musica: %d%%", (int)(currentVol * 100));
  DrawTextEx(uiFont, volLabel, {sliderX, labelY}, 16, 1, WHITE);

  // Slider background
  DrawRectangle((int)sliderX, (int)sliderY, (int)sliderW, (int)sliderH,
                ColorAlpha(DARKGRAY, 0.7f));
  DrawRectangleLinesEx({sliderX, sliderY, sliderW, sliderH}, 1, GRAY);

  // Filled portion
  float fillW = sliderW * currentVol;
  DrawRectangle((int)sliderX, (int)sliderY, (int)fillW, (int)sliderH,
                ColorAlpha(GOLD, 0.8f));

  // Knob
  float knobX = sliderX + fillW - 6;
  float knobY = sliderY - 4;
  float knobW = 12, knobH = sliderH + 8;
  DrawRectangle((int)knobX, (int)knobY, (int)knobW, (int)knobH, WHITE);
  DrawRectangleLinesEx({knobX, knobY, knobW, knobH}, 1, DARKGRAY);

  // Slider interaction (click or drag)
  Rectangle sliderRect = {sliderX, sliderY - 8, sliderW, sliderH + 16};
  if (CheckCollisionPointRec(mousePos, sliderRect) &&
      IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    float newVol = (mousePos.x - sliderX) / sliderW;
    if (newVol < 0.0f)
      newVol = 0.0f;
    if (newVol > 1.0f)
      newVol = 1.0f;
    AudioManager::Get().SetMusicVolume(newVol);
  }

  // === CLOSE BUTTON ===
  float btnW = 160, btnH = 36;
  float btnX = popX + (popW - btnW) / 2;
  float btnY = popY + popH - 60;
  Rectangle closeBtn = {btnX, btnY, btnW, btnH};
  bool closeHover = CheckCollisionPointRec(mousePos, closeBtn);

  if (texButton.id > 0) {
    Rectangle btnSrc = {0, 0, (float)texButton.width, (float)texButton.height};
    Color tint = closeHover ? LIGHTGRAY : WHITE;
    DrawTexturePro(texButton, btnSrc, closeBtn, {0, 0}, 0.0f, tint);
  } else {
    DrawRectangleRec(closeBtn, closeHover ? ColorAlpha(GRAY, 0.8f)
                                          : ColorAlpha(DARKGRAY, 0.8f));
  }
  const char *closeLabel = "Fechar";
  Vector2 closeSz = MeasureTextEx(uiFont, closeLabel, 16, 1);
  DrawTextEx(uiFont, closeLabel,
             {closeBtn.x + (btnW - closeSz.x) / 2,
              closeBtn.y + (btnH - closeSz.y) / 2},
             16, 1, WHITE);

  if (closeHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    showOptionsPopup = false;
  }
}
