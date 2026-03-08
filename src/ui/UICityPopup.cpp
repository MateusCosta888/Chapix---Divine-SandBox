#include "../simulation/SimulationManager.h"
#include "UIManager.h"
#include <cstring>
#include <string>


void UIManager::DrawCityPopup(const World &world) {
  auto &sim = const_cast<World &>(world).GetSimulation(); // Access simulation
  City *city = sim.GetCity(popupCityID);

  if (!city) {
    showCityPopup = false;
    return;
  }

  // Popup Dimensions
  float w = 400;
  float h = 500;

  // Lazy Init Position
  if (cityPopupPos.x == 0 && cityPopupPos.y == 0) {
    cityPopupPos.x = (getScreenW() - w) / 2;
    cityPopupPos.y = (getScreenH() - h) / 2;
  }

  // Drag Logic
  Vector2 mousePos = GetMousePosition();
  Rectangle headerRect = {cityPopupPos.x, cityPopupPos.y, w, 40}; // Top 40px

  // Only start drag if NOT renaming
  if (!isRenamingCity && CheckCollisionPointRec(mousePos, headerRect)) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      isDraggingCity = true;
      dragOffset.x = mousePos.x - cityPopupPos.x;
      dragOffset.y = mousePos.y - cityPopupPos.y;
    }
  }

  if (isDraggingCity) {
    cityPopupPos.x = mousePos.x - dragOffset.x;
    cityPopupPos.y = mousePos.y - dragOffset.y;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      isDraggingCity = false;
    }
  }

  // Use stored position
  float x = cityPopupPos.x;
  float y = cityPopupPos.y;
  Rectangle rect = {x, y, w, h};

  // 9-Slice Draw (Using Popup Textures)
  int cw = texPopupTL.width;
  int ch = texPopupTL.height;

  // Corners
  DrawTexture(texPopupTL, x, y, WHITE);
  DrawTexture(texPopupTR, x + w - cw, y, WHITE);
  DrawTexture(texPopupBL, x, y + h - ch, WHITE);
  DrawTexture(texPopupBR, x + w - cw, y + h - ch, WHITE);

  // Edges
  DrawTexturePro(texPopupTC,
                 {0, 0, (float)texPopupTC.width, (float)texPopupTC.height},
                 {x + cw, y, w - 2 * cw, (float)ch}, {0, 0}, 0, WHITE);
  DrawTexturePro(texPopupBC,
                 {0, 0, (float)texPopupBC.width, (float)texPopupBC.height},
                 {x + cw, y + h - ch, w - 2 * cw, (float)ch}, {0, 0}, 0, WHITE);
  DrawTexturePro(texPopupML,
                 {0, 0, (float)texPopupML.width, (float)texPopupML.height},
                 {x, y + ch, (float)cw, h - 2 * ch}, {0, 0}, 0, WHITE);
  DrawTexturePro(texPopupMR,
                 {0, 0, (float)texPopupMR.width, (float)texPopupMR.height},
                 {x + w - cw, y + ch, (float)cw, h - 2 * ch}, {0, 0}, 0, WHITE);

  // Center
  DrawTexturePro(texPopupMC,
                 {0, 0, (float)texPopupMC.width, (float)texPopupMC.height},
                 {x + cw, y + ch, w - 2 * cw, h - 2 * ch}, {0, 0}, 0, WHITE);

  // === CONTENT ===
  float contentX = x + 25;
  float contentY = y + 10; // Moved up from 25

  // 1. Title (Centered & Editable)
  const char *displayName =
      isRenamingCity ? cityRenameBuffer : city->name.c_str();
  Vector2 textSize = MeasureTextEx(uiFont, displayName, 30, 1);
  float titleX = x + (w - textSize.x) / 2; // Center horizontally

  if (isRenamingCity) {
    // Input Handling
    int key = GetCharPressed();
    while (key > 0) {
      if ((key >= 32) && (key <= 125) && (strlen(cityRenameBuffer) < 63)) {
        int len = strlen(cityRenameBuffer);
        cityRenameBuffer[len] = (char)key;
        cityRenameBuffer[len + 1] = '\0';
      }
      key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
      int len = strlen(cityRenameBuffer);
      if (len > 0)
        cityRenameBuffer[len - 1] = '\0';
    }

    if (IsKeyPressed(KEY_ENTER)) {
      city->name = std::string(cityRenameBuffer);
      isRenamingCity = false;
    }

    // Render Text Box
    DrawRectangleRec({titleX - 5, contentY, textSize.x + 10, 35}, DARKGRAY);
    DrawTextEx(uiFont, cityRenameBuffer, {titleX, contentY}, 30, 1, WHITE);

    // Blinking Cursor
    if ((int)(GetTime() * 2) % 2 == 0) {
      DrawTextEx(uiFont, "_", {titleX + textSize.x + 2, contentY}, 30, 1,
                 WHITE);
    }
  } else {
    // Render Static Name
    DrawTextEx(uiFont, city->name.c_str(), {titleX, contentY}, 30, 1, GOLD);

    // Click to Rename
    Rectangle titleRect = {titleX, contentY, textSize.x, 30};
    if (CheckCollisionPointRec(GetMousePosition(), titleRect) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      isRenamingCity = true;
      // Init buffer
      strncpy(cityRenameBuffer, city->name.c_str(), 63);
      cityRenameBuffer[63] = '\0'; // Safety
    }
  }

  contentY += 40;

  // 1.5 City Flag (Centered & Larger)
  if (city->flagID >= 0 &&
      city->flagID < (int)world.GetResourceManager().cityFlags.size()) {
    Texture2D flag =
        const_cast<World &>(world).GetResourceManager().cityFlags[city->flagID];

    float flagSize = 100.0f;              // Larger size
    float flagX = x + (w - flagSize) / 2; // Center horizontally

    Rectangle flagRect = {flagX, contentY, flagSize, flagSize};

    DrawTexturePro(flag, {0, 0, (float)flag.width, (float)flag.height},
                   flagRect, {0, 0}, 0, WHITE);

    if (CheckCollisionPointRec(GetMousePosition(), flagRect)) {
      DrawRectangleLinesEx(flagRect, 3, YELLOW); // Thicker highlight
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        showFlagSelector = true;
        flagSelectorScroll = 0.0f;
      }
    }
    contentY += (flagSize + 20); // Add spacing
  }

  // 2. Resources Grid
  DrawTextEx(uiFont, "Resources:", {contentX, contentY}, 20, 1, WHITE);
  contentY += 25;

  // Wood
  DrawTextEx(uiFont, TextFormat("Wood: %d", city->resources.wood),
             {contentX, contentY}, 18, 1, BROWN);
  contentY += 20;
  // Stone
  DrawTextEx(uiFont, TextFormat("Stone: %d", city->resources.stone),
             {contentX, contentY}, 18, 1, GRAY);
  contentY += 20;
  // Food
  DrawTextEx(
      uiFont,
      TextFormat("Food: %d / %d", city->resources.food, city->maxStorage),
      {contentX, contentY}, 18, 1, GREEN);

  contentY += 40;

  // 3. Population List
  DrawTextEx(uiFont, TextFormat("Population: %d", city->GetPopulation()),
             {contentX, contentY}, 20, 1, WHITE);
  contentY += 25;

  // Scroll Logic
  float listHeight = h - (contentY - y) - 30;
  float totalContentHeight = city->GetPopulation() * 20.0f;
  float maxScroll = totalContentHeight - listHeight;
  if (maxScroll < 0)
    maxScroll = 0;

  // Mouse Wheel
  if (CheckCollisionPointRec(GetMousePosition(), {x, y, w, h})) {
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
      cityPopupScroll -= wheel * 30.0f;
    }
  }
  // Clamp
  if (cityPopupScroll < 0)
    cityPopupScroll = 0;
  if (cityPopupScroll > maxScroll)
    cityPopupScroll = maxScroll;

  BeginScissorMode((int)contentX, (int)contentY, (int)(w - 40),
                   (int)listHeight);

  float currentY = contentY - cityPopupScroll;

  for (int id : city->citizenIDs) {
    Citizen *c = sim.GetCitizen(id);
    if (!c || !c->isAlive)
      continue;

    const char *professionName = "Unemployed";
    Color profColor = WHITE;
    switch (c->profession) {
    case Profession::Lumberjack:
      professionName = "Lumberjack";
      profColor = BROWN;
      break;
    case Profession::Miner:
      professionName = "Miner";
      profColor = GRAY;
      break;
    case Profession::Farmer:
      professionName = "Farmer";
      profColor = GREEN;
      break;
    case Profession::Builder:
      professionName = "Builder";
      profColor = YELLOW;
      break;
    case Profession::Soldier:
      professionName = "Soldier";
      profColor = RED;
      break;
    }

    // Interaction Line
    const char *text = TextFormat("- %s (%s)", c->name.c_str(), professionName);

    // Interaction Check (Right Click to Open Status)
    // Only check if possibly visible to save perf
    if (currentY + 20 >= contentY && currentY <= contentY + listHeight) {
      Vector2 textSize = MeasureTextEx(uiFont, text, 16, 1);
      Rectangle itemRect = {contentX, currentY, textSize.x + 200,
                            20}; // Full width clickable

      if (CheckCollisionPointRec(GetMousePosition(), itemRect)) {
        DrawRectangleRec(itemRect, Fade(LIGHTGRAY, 0.2f)); // Hover effect
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
          showHumanPopup = true;
          popupCitizenID = id;
          isDraggingHuman = false; // Fix: Reset drag
        }
      }
      DrawTextEx(uiFont, text, {contentX, currentY}, 16, 1, profColor);
    }

    currentY += 20;
  }
  EndScissorMode();

  // Close Button (Small X top right)
  Rectangle closeBtn = {x + w - 30, y + 10, 20, 20};
  DrawText("X", (int)closeBtn.x + 5, (int)closeBtn.y + 2, 20, RED);

  // Prevent Drag if hovering Close Button
  // (Handled by checking close button first or in Drag Logic, but resetting
  // here is key)

  if (CheckCollisionPointRec(GetMousePosition(), closeBtn) &&
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    showCityPopup = false;
    isDraggingCity = false; // Fix: Reset drag
  }
}
