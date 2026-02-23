#include "UIManager.h"
#include "../core/AudioManager.h"
#include "../core/SaveManager.h"
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

  // Load Social Popup Textures (Violet Background)
  texPopup3TL = LoadTexture(
      "assets/UI/Pup-Up Backgroud/BackgroudViolet/BackgroudViolet001.png");
  texPopup3TC = LoadTexture(
      "assets/UI/Pup-Up Backgroud/BackgroudViolet/BackgroudViolet002.png");
  texPopup3TR = LoadTexture(
      "assets/UI/Pup-Up Backgroud/BackgroudViolet/BackgroudViolet003.png");
  texPopup3ML = LoadTexture(
      "assets/UI/Pup-Up Backgroud/BackgroudViolet/BackgroudViolet004.png");
  texPopup3MC = LoadTexture(
      "assets/UI/Pup-Up Backgroud/BackgroudViolet/BackgroudViolet005.png");
  texPopup3MR = LoadTexture(
      "assets/UI/Pup-Up Backgroud/BackgroudViolet/BackgroudViolet006.png");
  texPopup3BL = LoadTexture(
      "assets/UI/Pup-Up Backgroud/BackgroudViolet/BackgroudViolet007.png");
  texPopup3BC = LoadTexture(
      "assets/UI/Pup-Up Backgroud/BackgroudViolet/BackgroudViolet008.png");
  texPopup3BR = LoadTexture(
      "assets/UI/Pup-Up Backgroud/BackgroudViolet/BackgroudViolet009.png");

  // Load Button Texture
  texButton = LoadTexture("assets/UI/TaskBar/Botoes/boto004.png");
  texTabButton = LoadTexture("assets/UI/TaskBar/Botoes/ButtomAbas.png");
  texTabRedFlag = LoadTexture("assets/UI/TaskBar/Botoes/RedFlagButton.png");
  if (texTabRedFlag.id == 0)
    TraceLog(LOG_WARNING, "UI: Failed to load texTabRedFlag");

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
  texSaveIcon = LoadTexture("assets/UI/Icons/SaveIcon.png"); // New Save Icon

  // Load Pergaminho (Save popup) 9-Slice Textures
  texPergaminhoTL = LoadTexture(
      "assets/UI/Pup-Up Backgroud/PergaminhoBackgod/Superior Esquerda.png");
  texPergaminhoTC = LoadTexture(
      "assets/UI/Pup-Up Backgroud/PergaminhoBackgod/Superior  Centro.png");
  texPergaminhoTR = LoadTexture(
      "assets/UI/Pup-Up Backgroud/PergaminhoBackgod/Superior Direita.png");
  texPergaminhoML = LoadTexture(
      "assets/UI/Pup-Up Backgroud/PergaminhoBackgod/Meio Esquerda.png");
  texPergaminhoMC = LoadTexture(
      "assets/UI/Pup-Up Backgroud/PergaminhoBackgod/Meio Centro.png");
  texPergaminhoMR = LoadTexture(
      "assets/UI/Pup-Up Backgroud/PergaminhoBackgod/Meio Direita.png");
  texPergaminhoBL = LoadTexture(
      "assets/UI/Pup-Up Backgroud/PergaminhoBackgod/Inferior Esquerda.png");
  texPergaminhoBC = LoadTexture(
      "assets/UI/Pup-Up Backgroud/PergaminhoBackgod/inferior Centro.png");
  texPergaminhoBR = LoadTexture(
      "assets/UI/Pup-Up Backgroud/PergaminhoBackgod/Inferior Direita.png");

  // Load Main Menu Textures
  texMenuDayBg = LoadTexture("assets/UI/main menu/DayBackgroud.png");
  texMenuDayBorder = LoadTexture("assets/UI/main menu/Moldura dia.png");
  texMenuNightBg = LoadTexture("assets/UI/main menu/BigBackgoudNight.png");
  texMenuNightBorder = LoadTexture("assets/UI/main menu/MolduraNight.png");
  texMenuLogo = LoadTexture("assets/UI/main menu/ChapiX - Logo.png");
  texMenuIlustration = LoadTexture("assets/UI/main menu/Ilustration.png");
  texMenuBtnStart = LoadTexture("assets/UI/main menu/Start.png");
  texMenuBtnOptions = LoadTexture("assets/UI/main menu/Options.png");
  texMenuBtnExit = LoadTexture("assets/UI/main menu/Exit.png");

  if (texPergaminhoTL.id == 0)
    TraceLog(LOG_WARNING, "UI: Failed to load PergaminhoBackgod textures!");
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

  UnloadTexture(texPopup3TL);
  UnloadTexture(texPopup3TC);
  UnloadTexture(texPopup3TR);
  UnloadTexture(texPopup3ML);
  UnloadTexture(texPopup3MC);
  UnloadTexture(texPopup3MR);
  UnloadTexture(texPopup3BL);
  UnloadTexture(texPopup3BC);
  UnloadTexture(texPopup3BR);

  UnloadTexture(texPergaminhoTL);
  UnloadTexture(texPergaminhoTC);
  UnloadTexture(texPergaminhoTR);
  UnloadTexture(texPergaminhoML);
  UnloadTexture(texPergaminhoMC);
  UnloadTexture(texPergaminhoMR);
  UnloadTexture(texPergaminhoBL);
  UnloadTexture(texPergaminhoBC);
  UnloadTexture(texPergaminhoBR);

  UnloadTexture(texMenuDayBg);
  UnloadTexture(texMenuDayBorder);
  UnloadTexture(texMenuNightBg);
  UnloadTexture(texMenuNightBorder);
  UnloadTexture(texMenuLogo);
  UnloadTexture(texMenuIlustration);
  UnloadTexture(texMenuBtnStart);
  UnloadTexture(texMenuBtnOptions);
  UnloadTexture(texMenuBtnExit);

  UnloadTexture(texButton);
  UnloadTexture(texTabButton);
  UnloadTexture(texTabRedFlag);
  UnloadTexture(texEraser);
  UnloadTexture(texIconWaterDeep);
  UnloadTexture(texIconWaterOcean);
  UnloadTexture(texIconWaterShallow);
  UnloadTexture(texSaveIcon);
  UnloadFont(uiFont);
  UnloadShader(circleMaskShader);
}

bool UIManager::IsPointerOnUI() const {
  Vector2 mousePos = GetMousePosition();

  // 1. Toolbar area
  if (mousePos.y > (getScreenH() - TOOLBAR_HEIGHT - TAB_HEIGHT))
    return true;

  // 2. City Popup
  if (showCityPopup) {
    float w = 400;
    float h = 500;
    float x = cityPopupPos.x;
    float y = cityPopupPos.y;

    // Fallback if not initialized yet (first frame)
    if (x == 0 && y == 0) {
      x = (getScreenW() - w) / 2;
      y = (getScreenH() - h) / 2;
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
      x = (getScreenW() - w) / 2;
      y = (getScreenH() - h) / 2;
    }

    if (CheckCollisionPointRec(mousePos, {x, y, w, h}))
      return true;
  }

  // 4. Brush Popup logic (if needed, but usually handled separately)
  if (showBrushPopup) {
    // Assuming brush popup is roughly where toolbar is or specific location
    // For now, toolbar check might cover it or added here if it floats
  }

  // 5. Social City List Popup
  if (showSocialCityList) {
    float w = 450;
    float h = 600;
    float x = socialPopupPos.x;
    float y = socialPopupPos.y;

    // Fallback if not initialized yet (first frame)
    if (x == 0 && y == 0) {
      x = (getScreenW() - w) / 2;
      y = (getScreenH() - h) / 2;
    }

    if (CheckCollisionPointRec(mousePos, {x, y, w, h}))
      return true;
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
    float x = cityPopupPos.x != 0 ? cityPopupPos.x : (getScreenW() - w) / 2;
    float y = cityPopupPos.y != 0 ? cityPopupPos.y : (getScreenH() - h) / 2;

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
              EntityType::HumanUnarmed, EntityType::Boar,  EntityType::Cow,
              EntityType::Chicken,      EntityType::Sheep, EntityType::Bull,
              EntityType::Chicken2,     EntityType::Lamb,  EntityType::Pig,
              EntityType::Turkey};

          // Updated size to 10
          if (selectedToolIndex >= 0 && selectedToolIndex < 10) {
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

  // Draw Standalone Calendar Window
  float calW = 160;
  float calH = 50;
  float calX = getScreenW() - calW - 20;
  float calY = 20;

  // Background
  DrawRectangle(calX, calY, calW, calH, ColorAlpha(BLACK, 0.7f));
  DrawRectangleLines(calX, calY, calW, calH, GOLD);

  int currentYear = const_cast<World &>(world).GetSimulation().GetCurrentYear();
  float progress = const_cast<World &>(world).GetSimulation().GetYearProgress();
  int currentMonth = (int)(progress * 12.0f) + 1;
  // Ensure month doesn't display 13 if it reaches 1.0 boundary exact
  if (currentMonth > 12)
    currentMonth = 12;

  const char *dateLabel =
      TextFormat("Ano: %d       Mês: %d", currentYear, currentMonth);
  Vector2 dateSize = MeasureTextEx(uiFont, dateLabel, 16, 1);
  DrawTextEx(uiFont, dateLabel, {calX + (calW - dateSize.x) / 2, calY + 8}, 16,
             1, WHITE);

  // Progress Bar
  Rectangle barBg = {calX + 10, calY + 30, calW - 20, 6};
  DrawRectangleRec(barBg, DARKGRAY);
  DrawRectangle(barBg.x, barBg.y, barBg.width * progress, barBg.height, GOLD);

  // Draw City Popup
  if (showCityPopup) {
    DrawCityPopup(world);
  }

  // Draw Human Popup
  if (showHumanPopup) {
    DrawHumanPopup(world);
  }

  // Draw Flag Selector (Topmost Popup)
  if (showFlagSelector) {
    DrawFlagSelector(world);
  }

  // Draw Save Popup
  if (showSavePopup) {
    DrawSavePopup(world);
  }

  // Draw Options Popup
  if (showOptionsPopup) {
    DrawOptionsPopup();
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
                 btnRect.y, GetMousePosition().y, getScreenH());
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
  } else if (currentTab == UIState::Social) {
    numTools = 1; // Just "Cidades" for now
    for (int i = 0; i < numTools; i++) {
      Rectangle btnRect = {startX + i * (btnSize + padding), startY, btnSize,
                           btnSize};
      bool isHover = CheckCollisionPointRec(mousePos, btnRect);

      DrawTexturedButton(texButton, btnRect, false, isHover);

      // Draw "Cidades" Text
      const char *btnName = "Cidades";
      Vector2 ts = MeasureTextEx(uiFont, btnName, 20, 1);
      DrawTextEx(
          uiFont, btnName,
          {btnRect.x + (btnSize - ts.x) / 2, btnRect.y + (btnSize - ts.y) / 2},
          20, 1, WHITE);

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHover &&
          !showBrushPopup) {
        showSocialCityList = !showSocialCityList; // Toggle city list
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
  const char *titleText = "Todas as Cidades";
  Vector2 titleSize = MeasureTextEx(uiFont, titleText, 30, 1);
  DrawTextEx(uiFont, titleText, {x + (w - titleSize.x) / 2, y + 10}, 30, 1,
             GOLD);

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
          showCityPopup = true;
          popupCityID = city.id;
          isDraggingCity = false;
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
    float btnStartY = logoY + logoH + 80; // Push buttons much lower
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

    // Title
    const char *title = "SELECIONAR SAVE";
    Vector2 titleSz = MeasureTextEx(uiFont, title, 22, 1);
    DrawTextEx(uiFont, title, {(sw - titleSz.x) / 2, 40}, 22, 1, WHITE);

    // Grid: 4 cols x 3 rows = 12 slots (centered)
    const int COLS = 4;
    const int ROWS = 3;
    const int TOTAL = COLS * ROWS;
    float cellSize = 100.0f;
    float cellPad = 16.0f;
    float gridW = COLS * cellSize + (COLS - 1) * cellPad;
    float gridH = ROWS * cellSize + (ROWS - 1) * cellPad;
    float gridX = (sw - gridW) / 2;
    float gridY = 90;

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
        const char *lbl = "Salvo";
        Vector2 lblSz = MeasureTextEx(uiFont, lbl, 12, 1);
        DrawTextEx(uiFont, lbl, {cx + (cellSize - lblSz.x) / 2, cy + 78}, 12, 1,
                   GREEN);
      } else {
        const char *lbl = "Novo Jogo";
        Vector2 lblSz = MeasureTextEx(uiFont, lbl, 11, 1);
        DrawTextEx(uiFont, lbl, {cx + (cellSize - lblSz.x) / 2, cy + 75}, 11, 1,
                   LIGHTGRAY);
      }

      // Click logic
      if (isHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (exists) {
          // Load existing save
          TraceLog(LOG_INFO, "MainMenu: Loading slot %d", slotID);
          SaveManager::LoadGame(slotID, const_cast<World &>(world));
          currentState = GameState::PLAYING;
          shouldStartGame = true;
        } else {
          // New game with random seed
          TraceLog(LOG_INFO, "MainMenu: New game from slot %d", slotID);
          uint32_t newSeed = (uint32_t)(GetTime() * 1000.0) + i;
          const_cast<World &>(world).Generate();
          currentState = GameState::PLAYING;
          shouldStartGame = true;
        }
      }
    }

    // "Voltar" button at bottom
    float voltarW = 160, voltarH = 36;
    Rectangle voltarBtn = {(sw - voltarW) / 2, gridY + gridH + 25, voltarW,
                           voltarH};
    bool voltarHover = CheckCollisionPointRec(mousePos, voltarBtn);

    // Use texButton for voltar
    Rectangle btnSrc = {0, 0, (float)texButton.width, (float)texButton.height};
    DrawTexturePro(texButton, btnSrc, voltarBtn, {0, 0}, 0.0f,
                   voltarHover ? LIGHTGRAY : WHITE);
    Vector2 voltarSz = MeasureTextEx(uiFont, "Voltar", 14, 1);
    DrawTextEx(uiFont, "Voltar",
               {voltarBtn.x + (voltarW - voltarSz.x) / 2,
                voltarBtn.y + (voltarH - voltarSz.y) / 2},
               14, 1, WHITE);

    if (voltarHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      isMainMenuNight = false;
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
  DrawTextEx(uiFont, title,
             {popX + (popW - titleSz.x) / 2, popY + 16}, 22, 1, GOLD);

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
