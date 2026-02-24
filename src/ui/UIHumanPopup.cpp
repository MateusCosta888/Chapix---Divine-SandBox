#include "../simulation/SimulationManager.h"
#include "UIManager.h"
#include "raymath.h"
#include <cmath>
#include <cstring>
#include <string>

void UIManager::DrawHumanPopup(const World &world) {
  auto &sim = const_cast<World &>(world).GetSimulation();
  Citizen *c = sim.GetCitizen(popupCitizenID);

  if (!c || !c->isAlive) {
    showHumanPopup = false;
    return;
  }

  // Popup Dimensions (Base Pup-Up2)
  float w = 400;
  float h = 600; // Taller for abilities/inventory

  // Lazy Init Position
  if (humanPopupPos.x == 0 && humanPopupPos.y == 0) {
    humanPopupPos.x = (getScreenW() - w) / 2;
    humanPopupPos.y = (getScreenH() - h) / 2;
  }

  // Drag Logic
  Vector2 mousePos = GetMousePosition();
  Rectangle headerRect = {humanPopupPos.x, humanPopupPos.y, w, 40}; // Top 40px

  if (!isRenamingHuman && CheckCollisionPointRec(mousePos, headerRect)) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      isDraggingHuman = true;
      dragOffset.x = mousePos.x - humanPopupPos.x;
      dragOffset.y = mousePos.y - humanPopupPos.y;
    }
  }

  if (isDraggingHuman) {
    humanPopupPos.x = mousePos.x - dragOffset.x;
    humanPopupPos.y = mousePos.y - dragOffset.y;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      isDraggingHuman = false;
    }
  }

  // Use stored position
  float x = humanPopupPos.x;
  float y = humanPopupPos.y;

  // 9-Slice Draw (Base Pup-Up2)
  int cw = texPopup2TL.width;
  int ch = texPopup2TL.height;

  // --- TABS LOGIC ---
  float tabW = 96; // User requested 96x32 stretched
  float tabH = 32;
  float tabX = x - 30; // Initial X (partially hidden)
  float tabY = y + 80;

  auto DrawTab = [&](int index, const char *icon) {
    float currentTabX = tabX;
    float targetX =
        x - tabW - 14; // Hover: Pop out 14px (more distinct) -- x-110

    Rectangle tabRect = {currentTabX, tabY + index * (tabH + 10), tabW, tabH};

    // Hover Animation
    if (CheckCollisionPointRec(GetMousePosition(),
                               {x - tabW - 20, tabRect.y, tabW + 50, tabH})) {
      tabRect.x = Lerp(tabRect.x, targetX, 15.0f * GetFrameTime());
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        humanWindowTab = index;
      }
    } else {
      // If active, fully visible (touch edge), else stick out more (x-80) but
      // still tucked
      float restX = (humanWindowTab == index) ? x - tabW : x - 80;
      tabRect.x = Lerp(tabRect.x, restX, 10.0f * GetFrameTime());

      // Snap to target if close to avoid jitter
      if (fabs(tabRect.x - restX) < 0.5f)
        tabRect.x = restX;
    }

    // Draw Tab Background (RedFlagButton) - Using DrawTexturePro
    // Round to nearest pixel to avoid shimmering
    Rectangle drawRect = {(float)(int)tabRect.x, (float)(int)tabRect.y,
                          tabRect.width, tabRect.height};

    DrawTexturePro(
        texTabRedFlag,
        {0, 0, (float)texTabRedFlag.width, (float)texTabRedFlag.height},
        drawRect, {0, 0}, 0.0f, WHITE);

    // Draw Icon/Text (Simple number for now or icon)
    // Adjusted X offset to 25 (more to the left)
    DrawText(TextFormat("%d", index + 1), (int)drawRect.x + 25,
             (int)drawRect.y + 6, 20, WHITE);
  };

  // Draw Tabs (behind window)
  DrawTab(0, "Main");
  DrawTab(1, "Abilities");

  // Corners
  DrawTexture(texPopup2TL, x, y, WHITE);
  DrawTexture(texPopup2TR, x + w - cw, y, WHITE);
  DrawTexture(texPopup2BL, x, y + h - ch, WHITE);
  DrawTexture(texPopup2BR, x + w - cw, y + h - ch, WHITE);

  // Edges
  DrawTexturePro(texPopup2TC,
                 {0, 0, (float)texPopup2TC.width, (float)texPopup2TC.height},
                 {x + cw, y, w - 2 * cw, (float)ch}, {0, 0}, 0, WHITE);
  DrawTexturePro(texPopup2BC,
                 {0, 0, (float)texPopup2BC.width, (float)texPopup2BC.height},
                 {x + cw, y + h - ch, w - 2 * cw, (float)ch}, {0, 0}, 0, WHITE);
  DrawTexturePro(texPopup2ML,
                 {0, 0, (float)texPopup2ML.width, (float)texPopup2ML.height},
                 {x, y + ch, (float)cw, h - 2 * ch}, {0, 0}, 0, WHITE);
  DrawTexturePro(texPopup2MR,
                 {0, 0, (float)texPopup2MR.width, (float)texPopup2MR.height},
                 {x + w - cw, y + ch, (float)cw, h - 2 * ch}, {0, 0}, 0, WHITE);

  // Center
  DrawTexturePro(texPopup2MC,
                 {0, 0, (float)texPopup2MC.width, (float)texPopup2MC.height},
                 {x + cw, y + ch, w - 2 * cw, h - 2 * ch}, {0, 0}, 0, WHITE);

  // === CONTENT ===
  float contentX = x + 35; // Padding
  float contentY = y + 25;

  // 0. Name (Top) - ALWAYS VISIBLE
  const char *displayName =
      isRenamingHuman ? humanRenameBuffer : c->name.c_str();
  Vector2 textSize = MeasureTextEx(uiFont, displayName, 30, 1);
  float titleX = x + (w - textSize.x) / 2;

  if (isRenamingHuman) {
    // Input Handling
    int key = GetCharPressed();
    while (key > 0) {
      if ((key >= 32) && (key <= 125) && (strlen(humanRenameBuffer) < 63)) {
        int len = strlen(humanRenameBuffer);
        humanRenameBuffer[len] = (char)key;
        humanRenameBuffer[len + 1] = '\0';
      }
      key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
      int len = strlen(humanRenameBuffer);
      if (len > 0)
        humanRenameBuffer[len - 1] = '\0';
    }
    if (IsKeyPressed(KEY_ENTER)) {
      c->name = std::string(humanRenameBuffer);
      isRenamingHuman = false;
    }

    DrawRectangleRec({titleX - 5, contentY, textSize.x + 10, 35}, DARKGRAY);
    DrawTextEx(uiFont, humanRenameBuffer, {titleX, contentY}, 30, 1, WHITE);
    if ((int)(GetTime() * 2) % 2 == 0)
      DrawTextEx(uiFont, "_", {titleX + textSize.x + 2, contentY}, 30, 1,
                 WHITE);
  } else {
    DrawTextEx(uiFont, c->name.c_str(), {titleX, contentY}, 30, 1, GOLD);

    // Rename click detection
    Rectangle titleRect = {titleX, contentY, textSize.x, 30};
    if (CheckCollisionPointRec(GetMousePosition(), titleRect) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      isRenamingHuman = true;
      strncpy(humanRenameBuffer, c->name.c_str(), 63);
      humanRenameBuffer[63] = '\0';
    }
  }
  contentY += 40;

  // 0.5 City Flag (Centered & Larger) - ALWAYS VISIBLE
  if (c->cityID != -1) {
    City *city = const_cast<World &>(world).GetSimulation().GetCity(c->cityID);
    if (city && city->flagID >= 0 &&
        city->flagID < (int)world.GetResourceManager().cityFlags.size()) {
      Texture2D flag = const_cast<World &>(world)
                           .GetResourceManager()
                           .cityFlags[city->flagID];

      float flagSize = 80.0f;
      float flagX = x + (w - flagSize) / 2;
      Rectangle flagRect = {flagX, contentY, flagSize, flagSize};

      DrawTexturePro(flag, {0, 0, (float)flag.width, (float)flag.height},
                     flagRect, {0, 0}, 0, WHITE);

      // Click logic (Go to City)
      if (CheckCollisionPointRec(GetMousePosition(), flagRect)) {
        DrawRectangleLinesEx(flagRect, 2, YELLOW);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          showCityPopup = true;
          popupCityID = c->cityID;
          showHumanPopup = false;
          return; // Close current popup immediately
        }
      }
      contentY += (flagSize + 10);
    }
  }

  // --- TAB CONTENT ---

  if (humanWindowTab == 0) {
    // TAB 0: MAIN INFO (XP, Stamina, Inventory, Profession)

    // 2. XP BAR (+/- Buttons)
    const char *levelText = TextFormat("Level %d", c->level);
    Vector2 levelSize = MeasureTextEx(uiFont, levelText, 20, 1);
    DrawTextEx(uiFont, levelText, {x + (w - levelSize.x) / 2, contentY}, 20, 1,
               WHITE);

    // Buttons [-] [+]
    Rectangle btnMinus = {x + w - 90, contentY - 5, 30, 30};
    Rectangle btnPlus = {x + w - 50, contentY - 5, 30, 30};

    DrawRectangleRec(btnMinus, RED);
    DrawText("-", (int)btnMinus.x + 10, (int)btnMinus.y + 2, 20, WHITE);
    if (CheckCollisionPointRec(GetMousePosition(), btnMinus) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (c->level > 1)
        c->level--;
    }

    DrawRectangleRec(btnPlus, GREEN);
    DrawText("+", (int)btnPlus.x + 8, (int)btnPlus.y + 2, 20, WHITE);
    if (CheckCollisionPointRec(GetMousePosition(), btnPlus) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      c->level++;
    }

    contentY += 35;
    // XP Bar Visual
    float barWidth = 330.0f;
    float barX = x + (w - barWidth) / 2;

    DrawRectangle(barX, contentY, barWidth, 15, BLACK);
    float xpPct = c->experience / c->maxExperience;
    if (xpPct > 1.0f)
      xpPct = 1.0f;
    DrawRectangle(barX, contentY, (int)(barWidth * xpPct), 15, GREEN);
    contentY += 30;

    // 2.5 STAMINA BAR
    const char *staminaText = "Stamina";
    DrawTextEx(
        uiFont, staminaText,
        {x + (w - MeasureTextEx(uiFont, staminaText, 18, 1).x) / 2, contentY},
        18, 1, WHITE);
    contentY += 20;

    DrawRectangle(barX, contentY, barWidth, 15, BLACK);
    float staminaPct = c->energy / 100.0f;
    if (staminaPct < 0)
      staminaPct = 0;

    Color staminaColor = ORANGE;
    if (staminaPct > 0.5f)
      staminaColor = GREEN;
    else if (staminaPct < 0.2f)
      staminaColor = RED;

    DrawRectangle(barX, contentY, (int)(barWidth * staminaPct), 15,
                  staminaColor);
    contentY += 20;

    // DEBUG INFO: Home & State
    const char *homeText = (c->homeID == -1)
                               ? "Homeless"
                               : TextFormat("Home: Building %d", c->homeID);
    DrawTextEx(uiFont, homeText, {x + 35, contentY}, 18, 1,
               (c->homeID == -1) ? RED : LIGHTGRAY);

    const char *stateText = "State: Idle";
    if (c->isResting)
      stateText = "State: Resting";
    else if (c->isGoingHome)
      stateText = "State: Going Home";
    else if (c->isWorking)
      stateText = "State: Working";
    DrawTextEx(uiFont, stateText, {x + 200, contentY}, 18, 1, WHITE);

    contentY += 40;

    // 3. INVENTORY (6 Slots)
    const char *invText = "Inventory:";
    Vector2 invSize = MeasureTextEx(uiFont, invText, 20, 1);
    DrawTextEx(uiFont, invText, {x + (w - invSize.x) / 2, contentY}, 20, 1,
               WHITE);
    contentY += 25;

    // Quick sync: carryingResource to Slot 0 (Visual only for now)
    c->inventory[0].type =
        c->carryingResource > 0 ? 1 : 0; // Simplified type assumption
    c->inventory[0].amount = c->carryingResource;

    float invGridWidth = 170.0f;
    float invStartX = x + (w - invGridWidth) / 2;

    for (int i = 0; i < 6; i++) {
      float slotX = invStartX + (i % 3) * 60; // 3 per row
      float slotY = contentY + (i / 3) * 60;
      DrawRectangleLines(slotX, slotY, 50, 50, LIGHTGRAY);

      if (c->inventory[i].amount > 0) {
        DrawText(TextFormat("%d", c->inventory[i].amount), (int)slotX + 5,
                 (int)slotY + 5, 20, WHITE);
        // Draw Icon based on type (TODO: Real icons)
        DrawCircle((int)slotX + 25, (int)slotY + 25, 10, BROWN);
      }
    }
    contentY += 130;

    // 4. PROFESSION
    const char *prof = "Unemployed";
    if (c->profession == Profession::Lumberjack)
      prof = "Lumberjack";
    else if (c->profession == Profession::Miner)
      prof = "Miner";
    else if (c->profession == Profession::Farmer)
      prof = "Farmer";
    else if (c->profession == Profession::Builder)
      prof = "Builder";
    else if (c->profession == Profession::Soldier)
      prof = "Soldier";

    const char *profLabel = TextFormat("Profession: %s", prof);
    Vector2 profSize = MeasureTextEx(uiFont, profLabel, 20, 1);
    DrawTextEx(uiFont, profLabel, {x + (w - profSize.x) / 2, contentY}, 20, 1,
               YELLOW);

  } else if (humanWindowTab == 1) {
    // TAB 1: ABILITIES

    // 5. ABILITIES (8 Slots)
    const char *abText = "Abilities:";
    Vector2 abSize = MeasureTextEx(uiFont, abText, 20, 1);
    DrawTextEx(uiFont, abText, {x + (w - abSize.x) / 2, contentY}, 20, 1,
               WHITE);
    contentY += 25;

    float abGridWidth = 230.0f;
    float abStartX = x + (w - abGridWidth) / 2;

    for (int i = 0; i < 8; i++) {
      float slotX = abStartX + (i % 4) * 60;
      float slotY = contentY + (i / 4) * 60;

      Rectangle abRect = {slotX, slotY, 50, 50};
      Color color = c->abilities[i] ? GOLD : DARKGRAY;

      DrawRectangleRec(abRect, color);
      DrawRectangleLinesEx(abRect, 2, WHITE);

      // Toggle on click
      if (CheckCollisionPointRec(GetMousePosition(), abRect) &&
          IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        c->abilities[i] = !c->abilities[i];
      }
    }
  }

  // Close Button
  Rectangle closeBtn = {x + w - 30, y + 10, 20, 20};
  DrawText("X", (int)closeBtn.x + 5, (int)closeBtn.y + 2, 20, RED);
  if (CheckCollisionPointRec(GetMousePosition(), closeBtn) &&
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    showHumanPopup = false;
    isDraggingHuman = false; // Fix: Reset drag
  }
}

void UIManager::DrawFlagSelector(const World &world) {
  auto &rm = const_cast<World &>(world).GetResourceManager();
  auto &sim = const_cast<World &>(world).GetSimulation();

  // Dimensions
  float w = 600;
  float h = 500;
  float x = (getScreenW() - w) / 2;
  float y = (getScreenH() - h) / 2;

  // Background
  DrawRectangle((int)x, (int)y, (int)w, (int)h, Fade(DARKBLUE, 0.95f));
  DrawRectangleLines((int)x, (int)y, (int)w, (int)h, WHITE);

  // Title
  DrawTextEx(uiFont, "Select City Flag", {x + 20, y + 20}, 30, 1, WHITE);

  // Close Button
  Rectangle closeBtn = {x + w - 30, y + 10, 20, 20};
  DrawText("X", (int)closeBtn.x + 5, (int)closeBtn.y + 2, 20, RED);
  if (CheckCollisionPointRec(GetMousePosition(), closeBtn) &&
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    showFlagSelector = false;
  }

  // Grid
  float contentY = y + 70;
  float contentX = x + 30;

  // Scroll Logic
  float totalHeight =
      (rm.cityFlags.size() / 8 + 1) * 70.0f; // 8 per row, 64px + padding
  float viewHeight = h - 90;
  float maxScroll = totalHeight - viewHeight;
  if (maxScroll < 0)
    maxScroll = 0;

  float wheel = GetMouseWheelMove();
  if (wheel != 0 && CheckCollisionPointRec(GetMousePosition(), {x, y, w, h})) {
    flagSelectorScroll -= wheel * 30.0f;
  }
  if (flagSelectorScroll < 0)
    flagSelectorScroll = 0;
  if (flagSelectorScroll > maxScroll)
    flagSelectorScroll = maxScroll;

  BeginScissorMode((int)contentX, (int)contentY, (int)(w - 60),
                   (int)viewHeight);

  float startY = contentY - flagSelectorScroll;
  float currentX = contentX;
  float currentY = startY;

  for (size_t i = 0; i < rm.cityFlags.size(); i++) {
    Rectangle dest = {currentX, currentY, 64, 64};

    // Only draw visible
    if (currentY + 64 >= contentY && currentY <= contentY + viewHeight) {
      DrawTexturePro(
          rm.cityFlags[i],
          {0, 0, (float)rm.cityFlags[i].width, (float)rm.cityFlags[i].height},
          dest, {0, 0}, 0, WHITE);

      // Selection Logic
      if (CheckCollisionPointRec(GetMousePosition(), dest)) {
        DrawRectangleLinesEx(dest, 2, YELLOW);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          City *city = sim.GetCity(popupCityID);
          if (city) {
            city->flagID = (int)i;
            showFlagSelector = false;
          }
        }
      }
    }

    currentX += 70; // 64 + 6 padding
    if (currentX + 64 > x + w - 30) {
      currentX = contentX;
      currentY += 70;
    }
  }

  EndScissorMode();
}
