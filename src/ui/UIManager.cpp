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

  // World Creator Assets
  texCreatorBg = LoadTexture("assets/UI/Pup-Up Backgroud/blue-with-stars.png");
  texPlanetAnim.clear();
  for (int i = 1; i <= 77; i++) {
    char path[128];
    sprintf(path, "assets/UI/Planet/Animed Earth/Planet%03d.png", i);
    texPlanetAnim.push_back(LoadTexture(path));
  }

  if (texPergaminhoTL.id == 0)
    TraceLog(LOG_WARNING, "UI: Failed to load PergaminhoBackgod textures!");
}

void UIManager::Unload() {
  UnloadTexture(texCreatorBg);
  for (Texture2D &t : texPlanetAnim) {
    UnloadTexture(t);
  }
  texPlanetAnim.clear();

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
              world.AddSpawnEffect(nx, ny, {100, 200, 255, 255});
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
            // --- SCATTER PLACEMENT for large brushes ---
            if (size > 1) {
              // Density: small items (flowers/mushrooms) = 40%, others = 30%
              int density = (selectedToolIndex >= 4) ? 40 : 30;
              if ((rand() % 100) >= density)
                continue; // Skip this tile randomly

              // Skip water/snow tiles
              Tile &t = world.GetTile(nx, ny);
              if (t.type == TileType::DeepOcean || t.type == TileType::Ocean ||
                  t.type == TileType::ShallowOcean ||
                  t.type == TileType::Snow)
                continue;

              // Skip if already has a decoration
              if (t.decoration != DecorationType::None)
                continue;

              world.SetTileDecoration(nx, ny, decs[selectedToolIndex]);
              world.AddSpawnEffect(nx, ny, {80, 220, 100, 255});
            } else {
              // Single brush: exact placement
              Tile &t = world.GetTile(nx, ny);
              if (t.decoration != decs[selectedToolIndex]) {
                world.SetTileDecoration(nx, ny, decs[selectedToolIndex]);
                world.AddSpawnEffect(nx, ny, {80, 220, 100, 255});
              }
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
            // --- SCATTER PLACEMENT for large brushes ---
            if (size > 1) {
              // Rocks are bulky: 20% density for natural look
              int density = (selectedToolIndex == 0) ? 25 : 15; // BigRock rarer
              if ((rand() % 100) >= density)
                continue;

              // Skip water tiles
              Tile &t = world.GetTile(nx, ny);
              if (t.type == TileType::DeepOcean || t.type == TileType::Ocean ||
                  t.type == TileType::ShallowOcean)
                continue;

              // Skip if already has a decoration
              if (t.decoration != DecorationType::None)
                continue;

              world.SetTileDecoration(nx, ny, decs[selectedToolIndex]);
              world.AddSpawnEffect(nx, ny, {180, 180, 180, 255});
            } else {
              // Single brush: exact placement
              Tile &t = world.GetTile(nx, ny);
              if (t.decoration != decs[selectedToolIndex]) {
                world.SetTileDecoration(nx, ny, decs[selectedToolIndex]);
                world.AddSpawnEffect(nx, ny, {180, 180, 180, 255});
              }
            }
          } else {
            // Eraser
            Tile &t = world.GetTile(nx, ny);
            if (t.decoration != DecorationType::None)
              world.SetTileDecoration(nx, ny, DecorationType::None);
          }
        } else if (currentTab == UIState::Creatures) {
          // Creature Placement - handled separately below
        }
      }
    }

    // === CREATURE SCATTER (separate loop for controlled count) ===
    if (currentTab == UIState::Creatures) {
      EntityType creatureTypesForPlacement[] = {
          EntityType::HumanUnarmed, EntityType::Boar,  EntityType::Cow,
          EntityType::Chicken,      EntityType::Sheep, EntityType::Bull,
          EntityType::Chicken2,     EntityType::Lamb,  EntityType::Pig,
          EntityType::Turkey};

      if (selectedToolIndex >= 0 && selectedToolIndex < 10 &&
          IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (size <= 1) {
          // Single brush: exact placement
          if (tx >= padding && tx < world.GetWidth() - padding &&
              ty >= padding && ty < world.GetHeight() - padding) {
            world.AddEntity(creatureTypesForPlacement[selectedToolIndex],
                            {(float)tx + 0.5f, (float)ty + 0.5f});
            world.AddSpawnEffect(tx, ty, {255, 215, 0, 255});
          }
        } else {
          // Large brush: spawn 1-3 creatures at random spots
          int count = 1 + (rand() % 3);
          for (int c = 0; c < count; c++) {
            int rx = tx + start + (rand() % (end - start + 1));
            int ry = ty + start + (rand() % (end - start + 1));
            if (rx < padding || rx >= world.GetWidth() - padding ||
                ry < padding || ry >= world.GetHeight() - padding)
              continue;
            // Only spawn on walkable land
            if (!world.IsWalkable(rx, ry))
              continue;
            world.AddEntity(
                creatureTypesForPlacement[selectedToolIndex],
                {(float)rx + 0.5f + (float)(rand() % 5) / 10.0f - 0.25f,
                 (float)ry + 0.5f + (float)(rand() % 5) / 10.0f - 0.25f});
            world.AddSpawnEffect(rx, ry, {255, 215, 0, 255});
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
