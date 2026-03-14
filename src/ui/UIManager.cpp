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

void UIManager::Load(std::function<void(const char *)> loadingCallback) {
  if (loadingCallback)
    loadingCallback("Loading UI assets...");
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

  // Load Power Icons
  texIconLightning =
      LoadTexture("assets/Effects/Pixel Effects "
                  "Gigapack/PNG/Lightning/lightning_strike_001/"
                  "lightning_strike_001_large_violet/frame0003.png");
  texIconFire = LoadTexture(
      "assets/Fire Sprites/png/orange/loops/burning_loop_1/burning_loop_1.png");

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
  UnloadTexture(texIconLightning);
  UnloadTexture(texIconFire);
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

bool UIManager::IsAnyPopupOpen() const {
  return showHumanPopup || showCityPopup || showSavePopup || showOptionsPopup ||
         showHumanSpawnMenu || showProfessionSelector || showFlagSelector ||
         showConfirmOverwrite || showSaveActionPopup ||
         showDeleteConfirmPopup || showForceWarConfirm ||
         showWorldCreatorPopup || showSocialCityList;
}

void UIManager::Update(World &world, Camera2D &camera) {
  popupJustOpened = false;
  HandleInput(world, camera);
}

void UIManager::HandleInput(World &world, Camera2D &camera) {
  Vector2 mousePos = GetMousePosition();
  bool isPointerOnUI = IsPointerOnUI() || IsAnyPopupOpen(); // Block ALL popups

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
        // Note: This O(N) spatial loop is fine for UI clicks as it happens
        // rarely and spatial partitioned queries are out of scope.
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
            if (t.type != TileType::Bedrock) {
              world.ClearTileContents(nx, ny);
              world.SetTileType(nx, ny, TileType::Bedrock);
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
                  t.type == TileType::ShallowOcean || t.type == TileType::Snow)
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
          EntityType::Turkey,       EntityType::Slime};

      if (selectedToolIndex >= 0 && selectedToolIndex < 11 &&
          IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !showHumanSpawnMenu) {

        // Determine the entity type to spawn
        EntityType spawnType = creatureTypesForPlacement[selectedToolIndex];

        // If Human button (index 0), use the spawn menu selection
        if (selectedToolIndex == 0 && humanSpawnSelection >= 0) {
          if (humanSpawnSelection == 2) {
            spawnType = EntityType::HumanWoman; // Explicit woman
          } else if (humanSpawnSelection == 1) {
            spawnType =
                EntityType::HumanUnarmed; // Explicit man (no random swap)
          }
          // humanSpawnSelection == 0 is Random, uses default HumanUnarmed (with
          // random swap in AddEntity)
        }

        // Should we skip gender randomization? (for explicit Man/Woman
        // selection)
        bool skipGender = (selectedToolIndex == 0 && humanSpawnSelection > 0);

        if (size <= 1) {
          // Single brush: exact placement
          if (tx >= padding && tx < world.GetWidth() - padding &&
              ty >= padding && ty < world.GetHeight() - padding) {
            world.AddEntity(spawnType, {(float)tx + 0.5f, (float)ty + 0.5f},
                            skipGender);
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
            if (!world.IsWalkable(rx, ry))
              continue;
            world.AddEntity(
                spawnType,
                {(float)rx + 0.5f + (float)(rand() % 5) / 10.0f - 0.25f,
                 (float)ry + 0.5f + (float)(rand() % 5) / 10.0f - 0.25f},
                skipGender);
            world.AddSpawnEffect(rx, ry, {255, 215, 0, 255});
          }
        }
      }
    } else if (currentTab == UIState::Powers) {
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !isPointerOnUI &&
          !showBrushPopup) {
        if (selectedToolIndex == 0) {
          world.TriggerGodPower(0, tx, ty); // Lightning
        } else if (selectedToolIndex == 1) {
          world.TriggerGodPower(1, tx, ty); // Fire
        }
      }
    }
  }
}

void UIManager::Draw(const World &world) {
  // --- NOTIFICATION TRACKING ---
  // Check for newly founded cities to spawn the notification popup
  const auto &cities = world.GetSimulation().GetCities();
  int maxCityID = -1;
  const City *newestCity = nullptr;
  for (const auto &pair : cities) {
    if (pair.first > maxCityID) {
      maxCityID = pair.first;
      newestCity = &pair.second;
    }
  }

  // If a new city was discovered (and it's not the initial boot load where
  // lastKnown is -1)
  if (lastKnownCityID != -1 && maxCityID > lastKnownCityID &&
      newestCity != nullptr) {
    AddNotification(TextFormat("Cidade Fundada: %s", newestCity->name.c_str()),
                    newestCity->flagID);
  }

  // Set tracking variable to prevent showing again
  if (maxCityID > lastKnownCityID) {
    lastKnownCityID = maxCityID;
  }

  // Update war notifications based on current diplomacy state
  UpdateWarNotifications(world);

  Vector2 mousePos = GetMousePosition();

  DrawToolbar(world);

  // Draw Standalone Calendar Window
  int currentYear = const_cast<World &>(world).GetSimulation().GetCurrentYear();
  float progress = const_cast<World &>(world).GetSimulation().GetYearProgress();
  int currentMonth = (int)(progress * 12.0f) + 1;
  if (currentMonth > 12)
    currentMonth = 12;

  const char *dateLabel =
      TextFormat("Ano: %d       Mes: %d", currentYear, currentMonth);
  Vector2 dateSize = MeasureTextEx(uiFont, dateLabel, 16, 1);

  float calW = dateSize.x + 30; // Dynamic width: text + padding
  if (calW < 160)
    calW = 160; // Minimum width
  float calH = 50;
  float calX = getScreenW() - calW - 20;
  float calY = 20;

  // Background
  DrawRectangle(calX, calY, calW, calH, ColorAlpha(BLACK, 0.7f));
  DrawRectangleLines(calX, calY, calW, calH, GOLD);

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

  // Draw Profession Selector
  if (showProfessionSelector) {
    DrawProfessionSelector(world);
  }

  // Draw Save Popup
  if (showSavePopup) {
    DrawSavePopup(world);
  }

  // Draw Options Popup
  if (showOptionsPopup) {
    DrawOptionsPopup();
  }

  // Draw Active Notifications
  DrawNotifications(world);

  // Draw Autosave Notification
  if (isAutosaving) {
    autosaveUIRemaining -= GetFrameTime();
    if (autosaveUIRemaining <= 0.0f) {
      isAutosaving = false;
    } else {
      const char *text = "Autosaving...";
      int textWidth = MeasureText(text, 20);
      DrawTextEx(uiFont, text, {(float)GetScreenWidth() - textWidth - 20, 20},
                 20, 1, GREEN);
    }
  }

  // Draw Cursor (Always Top)
  DrawTextureEx(texCursor, mousePos, 0.0f, cursorScale, WHITE);
}

void UIManager::ShowAutosaveNotification() {
  isAutosaving = true;
  autosaveUIRemaining = 2.0f;
}

void UIManager::AddNotification(const std::string &text, int flagID) {
  UINotification notif;
  notif.text = text;
  notif.iconFlagID = flagID;
  notif.timer = 5.0f; // 5 seconds total lifespan
  notif.maxTimer = 5.0f;
  notif.yOffset =
      activeNotifications.size() * 60.0f; // Stack slightly separated

  activeNotifications.push_back(notif);
}

void UIManager::UpdateWarNotifications(const World &world) {
  const auto &kingdoms = world.GetSimulation().GetAllKingdoms();

  // Build current active war pairs (canonical order: min,max)
  std::map<int, std::set<int>> currentWarPairs;
  for (const auto &pair : kingdoms) {
    int a = pair.first;
    const Kingdom &kingdom = pair.second;
    for (int b : kingdom.activeWarKingdoms) {
      if (b <= a)
        continue;
      currentWarPairs[a].insert(b);
    }
  }

  // Remove stale pairs from knownWarPairs
  for (auto it = knownWarPairs.begin(); it != knownWarPairs.end();) {
    int a = it->first;
    auto &setB = it->second;
    for (auto bit = setB.begin(); bit != setB.end();) {
      int b = *bit;
      if (currentWarPairs[a].count(b) == 0) {
        bit = setB.erase(bit);
      } else {
        ++bit;
      }
    }
    if (setB.empty())
      it = knownWarPairs.erase(it);
    else
      ++it;
  }

  // Add new war notifications
  for (const auto &pair : currentWarPairs) {
    int a = pair.first;
    for (int b : pair.second) {
      if (knownWarPairs[a].count(b) > 0)
        continue;

      // New war detected
      const Kingdom *kA = nullptr;
      const Kingdom *kB = nullptr;
      auto itA = kingdoms.find(a);
      auto itB = kingdoms.find(b);
      if (itA != kingdoms.end())
        kA = &itA->second;
      if (itB != kingdoms.end())
        kB = &itB->second;
      if (!kA || !kB)
        continue;

      // Determine icon flag from capital if available
      int flagID = 0;
      if (kA->capitalCityID >= 0) {
        const City *cap = world.GetSimulation().GetCity(kA->capitalCityID);
        if (cap)
          flagID = cap->flagID;
      }

      AddNotification(
          TextFormat("Guerra: %s vs %s", kA->name.c_str(), kB->name.c_str()),
          flagID);
      knownWarPairs[a].insert(b);
    }
  }
}

void UIManager::DrawNotifications(const World &world) {
  float dt = GetFrameTime();
  const ResourceManager &rm = world.GetResourceManager();

  if (!rm.IsLoaded())
    return;

  Texture2D texLeft = const_cast<ResourceManager &>(rm).texNotifLeft;
  Texture2D texMid = const_cast<ResourceManager &>(rm).texNotifMid;
  Texture2D texRight = const_cast<ResourceManager &>(rm).texNotifRight;

  // Clean up expired notifications
  activeNotifications.erase(
      std::remove_if(activeNotifications.begin(), activeNotifications.end(),
                     [](const UINotification &n) { return n.timer <= 0.0f; }),
      activeNotifications.end());

  float topY = 100.0f; // Baseline: Below the calendar UI

  for (size_t i = 0; i < activeNotifications.size(); i++) {
    UINotification &n = activeNotifications[i];

    // Update timer
    n.timer -= dt;

    // Dimensions
    int textWidth = MeasureTextEx(uiFont, n.text.c_str(), 18, 1).x;
    float iconWidth = 32.0f;
    float spacing = 10.0f;
    float totalContentWidth = iconWidth + spacing + textWidth;

    // Dynamic Mid calculation
    float leftW = texLeft.width;
    float rightW = texRight.width;
    float padding = 20.0f;
    float midPartsNeeded =
        std::ceil((totalContentWidth + padding) / texMid.width);
    float renderedMidW = midPartsNeeded * texMid.width;

    // Total physical width of the banner
    float bannerWidth = leftW + renderedMidW + rightW;

    // Animation Logic (Slide from right edge)
    // 5.0s -> 4.5s (Slide in)
    // 4.5s -> 0.5s (Hold)
    // 0.5s -> 0.0s (Slide out)
    float animProgress = 0.0f;
    if (n.timer > n.maxTimer - 0.5f) {
      // Sliding IN
      animProgress = 1.0f - ((n.timer - (n.maxTimer - 0.5f)) / 0.5f);
    } else if (n.timer < 0.5f) {
      // Sliding OUT
      animProgress = n.timer / 0.5f;
    } else {
      // Hold
      animProgress = 1.0f;
    }

    // Lerp from Out of Screen to In Screen
    float onScreenX = getScreenW() - bannerWidth - 20.0f;
    float offScreenX = getScreenW() + 50.0f;
    float startX = offScreenX + (onScreenX - offScreenX) * animProgress;

    // Render Y Coordinate
    // Target offset drops to 0 smoothly if elements below were erased
    float targetOffset = i * 70.0f;
    n.yOffset += (targetOffset - n.yOffset) * (dt * 5.0f); // Smooth catchup

    float startY = topY + n.yOffset; // Stack downwards below calendar

    // Draw the 3 splits
    DrawTextureV(texLeft, {startX, startY}, WHITE);

    float cx = startX + leftW;
    for (int m = 0; m < midPartsNeeded; m++) {
      DrawTextureV(texMid, {cx, startY}, WHITE);
      cx += texMid.width;
    }

    DrawTextureV(texRight, {cx, startY}, WHITE);

    // Draw Content
    float contentX = startX + leftW + 5.0f;
    float contentY = startY + texMid.height / 2.0f - 16.0f; // Centered vertical

    // Draw Flag Icon
    if (n.iconFlagID >= 0 && n.iconFlagID < (int)rm.cityFlags.size()) {
      Texture2D flagTex =
          const_cast<ResourceManager &>(rm).cityFlags[n.iconFlagID];
      DrawTexturePro(flagTex,
                     {0, 0, (float)flagTex.width, (float)flagTex.height},
                     {contentX, contentY, iconWidth, 32.0f}, // Assume 32 height
                     {0, 0}, 0.0f, WHITE);
    }

    // Draw Text
    contentX += iconWidth + spacing;
    DrawTextEx(uiFont, n.text.c_str(), {contentX, contentY + 6.0f}, 18, 1,
               WHITE);
  }
}
