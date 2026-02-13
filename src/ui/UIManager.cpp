#include "UIManager.h"
#include "../simulation/SimulationManager.h"
#include "raymath.h"
#include <algorithm> // for max/min
#include <cstring>
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

  // Load City Popup Textures (9-Slice)
  texPopupTL = LoadTexture(
      "assets/UI/Pup-Up Backgroud/base Pu-Up/Superior esquerda.png");
  texPopupTC =
      LoadTexture("assets/UI/Pup-Up Backgroud/base Pu-Up/Superior Centro.png");
  texPopupTR =
      LoadTexture("assets/UI/Pup-Up Backgroud/base Pu-Up/Superior Direita.png");
  texPopupML =
      LoadTexture("assets/UI/Pup-Up Backgroud/base Pu-Up/Meio esquerda.png");
  texPopupMC =
      LoadTexture("assets/UI/Pup-Up Backgroud/base Pu-Up/Meio Centro.png");
  texPopupMR =
      LoadTexture("assets/UI/Pup-Up Backgroud/base Pu-Up/Meio Direita.png");
  texPopupBL = LoadTexture(
      "assets/UI/Pup-Up Backgroud/base Pu-Up/Inferior esquerda.png");
  texPopupBC =
      LoadTexture("assets/UI/Pup-Up Backgroud/base Pu-Up/Inferior Centro.png");
  texPopupBR =
      LoadTexture("assets/UI/Pup-Up Backgroud/base Pu-Up/Inferior direita.png");

  // Load Human Popup Textures (9-Slice Base Pup-Up2)
  texPopup2TL = LoadTexture(
      "assets/UI/Pup-Up Backgroud/Base Pup-Up2/Superior Esquerdo.png");
  texPopup2TC = LoadTexture(
      "assets/UI/Pup-Up Backgroud/Base Pup-Up2/Superior Centro.png");
  texPopup2TR = LoadTexture(
      "assets/UI/Pup-Up Backgroud/Base Pup-Up2/Superior Direita.png");
  texPopup2ML =
      LoadTexture("assets/UI/Pup-Up Backgroud/Base Pup-Up2/Meio Esquerdo.png");
  texPopup2MC =
      LoadTexture("assets/UI/Pup-Up Backgroud/Base Pup-Up2/Meio Centro.png");
  texPopup2MR =
      LoadTexture("assets/UI/Pup-Up Backgroud/Base Pup-Up2/Meio Direita.png");
  texPopup2BL = LoadTexture(
      "assets/UI/Pup-Up Backgroud/Base Pup-Up2/Inferior Esquerdo.png");
  texPopup2BC = LoadTexture(
      "assets/UI/Pup-Up Backgroud/Base Pup-Up2/Inferior Centro.png");
  texPopup2BR = LoadTexture(
      "assets/UI/Pup-Up Backgroud/Base Pup-Up2/Inferior Direita.png");

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

  UnloadTexture(texPopupTL);
  UnloadTexture(texPopupTC);
  UnloadTexture(texPopupTR);
  UnloadTexture(texPopupML);
  UnloadTexture(texPopupMC);
  UnloadTexture(texPopupMR);
  UnloadTexture(texPopupBL);
  UnloadTexture(texPopupBC);
  UnloadTexture(texPopupBR);

  UnloadTexture(texPopup2TL);
  UnloadTexture(texPopup2TC);
  UnloadTexture(texPopup2TR);
  UnloadTexture(texPopup2ML);
  UnloadTexture(texPopup2MC);
  UnloadTexture(texPopup2MR);
  UnloadTexture(texPopup2BL);
  UnloadTexture(texPopup2BC);
  UnloadTexture(texPopup2BR);
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

  // 1. Toolbar area
  if (mousePos.y > (SCREEN_HEIGHT - TOOLBAR_HEIGHT - TAB_HEIGHT))
    return true;

  // 2. City Popup
  if (showCityPopup) {
    float w = 400;
    float h = 500;
    float x = cityPopupPos.x;
    float y = cityPopupPos.y;

    // Fallback if not initialized yet (first frame)
    if (x == 0 && y == 0) {
      x = (SCREEN_WIDTH - w) / 2;
      y = (SCREEN_HEIGHT - h) / 2;
    }

    if (CheckCollisionPointRec(mousePos, {x, y, w, h}))
      return true;
  }

  // 3. Human Popup
  if (showHumanPopup) {
    float w = 400;
    float h = 600;
    float x = humanPopupPos.x;
    float y = humanPopupPos.y;

    // Fallback
    if (x == 0 && y == 0) {
      x = (SCREEN_WIDTH - w) / 2;
      y = (SCREEN_HEIGHT - h) / 2;
    }

    if (CheckCollisionPointRec(mousePos, {x, y, w, h}))
      return true;
  }

  // 4. Brush Popup logic (if needed, but usually handled separately)
  if (showBrushPopup) {
    // Assuming brush popup is roughly where toolbar is or specific location
    // For now, toolbar check might cover it or added here if it floats
  }

  return false;
}

void UIManager::Update(World &world, Camera2D &camera) {
  popupJustOpened = false;
  HandleInput(world, camera);
}

void UIManager::HandleInput(World &world, Camera2D &camera) {
  Vector2 mousePos = GetMousePosition();
  bool isPointerOnUI =
      IsPointerOnUI() || showCityPopup; // Block clicks if popup is open

  // City Popup Interaction
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !isPointerOnUI) {
    // Raycast for Cities
    Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
    auto &sim = const_cast<World &>(world).GetSimulation();

    // 1. Check for CITIZENS (Priority over City)
    int clickedCitizenID = -1;
    const std::vector<Entity> &entities = world.GetEntities();
    for (const auto &e : entities) {
      if (e.citizenID != -1) {
        // Entity pos is in Grid Coords (e.g. 50.5), TileSize = 10.0f
        Vector2 entPosPx = {e.position.x * 10.0f, e.position.y * 10.0f};
        // Hitbox radius ~8px (almost full tile)
        if (CheckCollisionPointCircle(worldPos, entPosPx, 8.0f)) {
          clickedCitizenID = e.citizenID;
          break;
        }
      }
    }

    if (clickedCitizenID != -1) {
      showHumanPopup = true;
      popupCitizenID = clickedCitizenID;
      showCityPopup = false;
      isRenamingHuman = false;
      isDraggingHuman = false; // Fix: Reset drag
      return;                  // Consumed input
    }

    // 2. Check for CITIES
    int clickedCityID = -1;
    float minDist =
        64.0f; // Click radius around city center (buildings usually close)

    for (const auto &pair : sim.GetCities()) {
      const City &city = pair.second;
      // Approximate center by first building or avg position?
      // Better: Check if any building of this city is clicked
      for (const auto &b : city.buildings) {
        float bx = b.tileX * 10.0f + 5.0f;
        float by = b.tileY * 10.0f + 5.0f;
        if (CheckCollisionPointCircle(worldPos, {bx, by},
                                      15.0f)) { // 1.5 tile radius
          clickedCityID = city.id;
          break;
        }
      }
      if (clickedCityID != -1)
        break;
    }

    if (clickedCityID != -1) {
      showCityPopup = true;
      popupCityID = clickedCityID;
      isRenamingCity = false; // Reset renaming
      isDraggingCity = false; // Fix: Reset drag
    }
  }

  // Close Popup if clicked outside (City)
  if (showCityPopup && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    float w = 400;
    float h = 500;
    float x = cityPopupPos.x != 0 ? cityPopupPos.x : (SCREEN_WIDTH - w) / 2;
    float y = cityPopupPos.y != 0 ? cityPopupPos.y : (SCREEN_HEIGHT - h) / 2;

    if (!CheckCollisionPointRec(GetMousePosition(), {x, y, w, h})) {
      showCityPopup = false;
      isDraggingCity = false; // Ensure drag is reset
    }
  }

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

  // Draw City Popup
  if (showCityPopup) {
    DrawCityPopup(world);
  }

  // Draw Human Popup
  if (showHumanPopup) {
    DrawHumanPopup(world);
  }

  // Draw Cursor (Always Top)
  DrawTextureEx(texCursor, mousePos, 0.0f, cursorScale, WHITE);
}

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
    cityPopupPos.x = (SCREEN_WIDTH - w) / 2;
    cityPopupPos.y = (SCREEN_HEIGHT - h) / 2;
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

  contentY += 50;

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
    humanPopupPos.x = (SCREEN_WIDTH - w) / 2;
    humanPopupPos.y = (SCREEN_HEIGHT - h) / 2;
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

  // 1. NAME (Centered & Editable)
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
  contentY += 60;

  // 2. XP BAR (+/- Buttons)
  // Center "Level X" text
  const char *levelText = TextFormat("Level %d", c->level);
  Vector2 levelSize = MeasureTextEx(uiFont, levelText, 20, 1);
  DrawTextEx(uiFont, levelText, {x + (w - levelSize.x) / 2, contentY}, 20, 1,
             WHITE);

  // Buttons [-] [+] (Keep at right for now, or move next to text?)
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

  // Center Grid: 3 cols * 60px - 10 (last gap) = 170px width
  // But logic is i%3 * 60. So 0, 60, 120. Rect is 50. End is 120+50 = 170.
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
  contentY += 40;

  // 5. ABILITIES (8 Slots)
  const char *abText = "Abilities:";
  Vector2 abSize = MeasureTextEx(uiFont, abText, 20, 1);
  DrawTextEx(uiFont, abText, {x + (w - abSize.x) / 2, contentY}, 20, 1, WHITE);
  contentY += 25;

  // Center Grid: 4 cols. Width = 3*60 + 50 = 230.
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

  // Close Button
  Rectangle closeBtn = {x + w - 30, y + 10, 20, 20};
  DrawText("X", (int)closeBtn.x + 5, (int)closeBtn.y + 2, 20, RED);
  if (CheckCollisionPointRec(GetMousePosition(), closeBtn) &&
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    showHumanPopup = false;
    isDraggingHuman = false; // Fix: Reset drag
  }
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
  // We want to clip content that flows outside startX -> startX +
  // visibleWidth
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
          // Standard scaling for nature shouldn't need floor logic if we
          // trust it, but let's keep it safe or use the new standard. Nature
          // items are usually small pixel art, so floor is good, but let's
          // just use the safer logic:
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
          // Standard scaling for nature shouldn't need floor logic if we
          // trust it, but let's keep it safe or use the new standard. Nature
          // items are usually small pixel art, so floor is good, but let's
          // just use the safer logic:
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
  }

  // === TIME POPUP (drawn above everything) ===
  if (showTimePopup) {
    TimeManager &tm = TimeManager::Get();

    // Popup dimensions
    float popupW = 280;
    float popupH = 120;
    float popupX = (SCREEN_WIDTH - popupW) / 2;
    float popupY = SCREEN_HEIGHT - TOOLBAR_HEIGHT - TAB_HEIGHT - popupH - 20;

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
}
