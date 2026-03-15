#include "core/AudioManager.h"
#include "core/CrashHandler.h"
#include "core/SaveManager.h"
#include "core/TimeManager.h"
#include "graphics/CloudManager.h"
#include "graphics/WorldRenderer.h"
#include "raylib.h"
#include "raymath.h"
#include "ui/UIManager.h" // Added UIManager
#include "world/World.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// UI Constants (Keep for InitWindow)
const int TOOLBAR_HEIGHT = 100;
const int TAB_HEIGHT = 30;

// Enums moved to UIManager.h
// struct GameUI removed

int main(int argc, char *argv[]) {
  // Initialize crash handler FIRST (before anything else)
  CrashHandler::Init();
  CrashHandler::SetGameContext("Initializing");

  // Dynamic resolution: detect monitor and adapt
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(800, 600, "ChapiX - Divine SandBox"); // Temp size, resized below
  Image myicon = LoadImage("assets/UI/main menu/Ilustration.png");
  SetWindowIcon(myicon);
  UnloadImage(myicon);
  int monitor = GetCurrentMonitor();
  int monW = GetMonitorWidth(monitor);
  int monH = GetMonitorHeight(monitor);
  int winW = (int)(monW * 0.85f);
  int winH = (int)(monH * 0.85f);
  SetWindowSize(winW, winH);
  SetWindowMinSize(800, 600);
  SetWindowPosition((monW - winW) / 2, (monH - winH) / 2);
  SetTargetFPS(60);

  // Initialize audio
  InitAudioDevice();

  // === LOADING SCREEN HELPER ===
  std::vector<Texture2D> planetFrames;
  for (int i = 1; i <= 77; i++) {
    char path[256];
    sprintf(path, "assets/UI/Planet/Animed Earth/Planet%03d.png", i);
    planetFrames.push_back(LoadTexture(path));
  }

  int currentPlanetFrame = 0;
  auto DrawLoadingScreen = [&](const char *statusText) {
    BeginDrawing();
    ClearBackground(GetColor(0x0d0d1aFF));
    float cx = GetScreenWidth() / 2.0f;
    float cy = GetScreenHeight() / 2.0f;

    // Draw Animated Earth
    if (!planetFrames.empty()) {
      Texture2D &tex = planetFrames[currentPlanetFrame];
      if (tex.id > 0) {
        // Increase frame index for next draw call
        currentPlanetFrame = (currentPlanetFrame + 1) % planetFrames.size();

        float scale = 3.0f; // Scale up the planet
        float w = tex.width * scale;
        float h = tex.height * scale;
        Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
        Rectangle dst = {cx - w / 2, cy - h / 2 - 30, w, h};
        DrawTexturePro(tex, src, dst, {0, 0}, 0.0f, WHITE);
      }
    }

    // "Loading..." text
    const char *loadText = "Loading...";
    int textW = MeasureText(loadText, 24);
    DrawText(loadText, (int)(cx - textW / 2), (int)(cy + 60), 24, GOLD);

    // Status text
    int statusW = MeasureText(statusText, 16);
    DrawText(statusText, (int)(cx - statusW / 2), (int)(cy + 90), 16, GRAY);

    EndDrawing();
  };

  // Show initial loading screen
  DrawLoadingScreen("Initializing...");

  {
    uint32_t seed = 0;
    if (argc > 1 && std::string(argv[1]) == "--seed") {
      seed = std::stoul(argv[2]);
    } else {
      seed = (uint32_t)GetTime();
    }

    // Show initial loading screen (render a few frames to ensure it's visible)
    for (int i = 0; i < 3; i++)
      DrawLoadingScreen("Generating world...");
    World world(128, 128, seed);
    world.Generate(DrawLoadingScreen);

    world.LoadTextures(DrawLoadingScreen);

    WorldRenderer worldRenderer(world);
    UIManager ui;
    ui.Load(DrawLoadingScreen);

    for (int i = 0; i < 2; i++)
      DrawLoadingScreen("Loading audio...");
    // Load music with progress updates to keep planet animated
    AudioManager::Get().Load(DrawLoadingScreen);
    CloudManager cloudManager;
    cloudManager.Load();

    // Free loading screen textures
    for (Texture2D &tex : planetFrames) {
      if (tex.id > 0)
        UnloadTexture(tex);
    }
    planetFrames.clear();

    HideCursor();

    Camera2D camera = {0};
    camera.zoom = 2.0f;
    camera.offset = {GetScreenWidth() / 2.0f,
                     (GetScreenHeight() - TOOLBAR_HEIGHT) / 2.0f};
    camera.target = {world.GetWidth() * 5.0f, world.GetHeight() * 5.0f};

    while (!WindowShouldClose()) {
      // Check exit signal from main menu
      if (ui.shouldExitGame)
        break;

      BeginDrawing();

      // Update music mode based on state
      if (ui.currentState == GameState::MAIN_MENU && ui.isMainMenuNight) {
        AudioManager::Get().SetMusicMode(MusicMode::SPACE);
      } else {
        AudioManager::Get().SetMusicMode(MusicMode::AMBIENT);
      }
      AudioManager::Get().Update();

      if (ui.currentState == GameState::MAIN_MENU) {
        // === MAIN MENU ===
        ClearBackground(BLACK);
        CrashHandler::SetGameContext("Main Menu");
        ui.UpdateMainMenu(world);
        ui.DrawMainMenu(world);

        // Check if we should transition to playing
        if (ui.shouldStartGame) {
          ui.shouldStartGame = false;
          // Reset camera for new world
          camera.target = {world.GetWidth() * 5.0f, world.GetHeight() * 5.0f};
          camera.zoom = 2.0f;
        }
      } else {
        // === PLAYING ===
        CrashHandler::SetGameContext("Playing");
        CrashHandler::AddContext(
            "Cidades", std::to_string(world.GetSimulation().GetTotalCities()));
        CrashHandler::AddContext(
            "Populacao",
            std::to_string(world.GetSimulation().GetTotalPopulation()));
        CrashHandler::AddContext("Entidades",
                                 std::to_string(world.GetEntities().size()));
        ui.popupJustOpened = false;

        // --- AUTOSAVE SYSTEM ---
        static float autosaveTimer = 0.0f;
        static const float AUTOSAVE_INTERVAL = 300.0f; // 5 minutes
        static int autosaveIndex = 1;

        autosaveTimer += GetFrameTime();
        if (autosaveTimer >= AUTOSAVE_INTERVAL) {
          autosaveTimer = 0.0f;
          std::string filename =
              "autosave_" + std::to_string(autosaveIndex) + ".json";
          SaveManager::SaveGameAsync(filename, world);
          ui.ShowAutosaveNotification();
          autosaveIndex = (autosaveIndex == 1) ? 2 : 1;
        }

        Vector2 mousePos = GetMousePosition();
        bool isPointerOnUI = ui.IsPointerOnUI();



        // Camera Controls
        if (!isPointerOnUI && !ui.showBrushPopup) {
          float wheel = GetMouseWheelMove();
          if (wheel != 0) {
            Vector2 mouseWorldBefore = GetScreenToWorld2D(mousePos, camera);
            camera.zoom = Clamp(camera.zoom + wheel * 0.5f, 0.5f, 10.0f);
            Vector2 mouseWorldAfter = GetScreenToWorld2D(mousePos, camera);
            camera.target =
                Vector2Add(camera.target,
                           Vector2Subtract(mouseWorldBefore, mouseWorldAfter));
          }
          if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = Vector2Scale(GetMouseDelta(), -1.0f / camera.zoom);
            camera.target = Vector2Add(camera.target, delta);
          }
        }

        // Camera Tracking (Human Popup)
        if (ui.IsHumanPopupOpen() && ui.GetPopupCitizenID() != -1) {
          const Entity *e = world.GetEntityByCitizenID(ui.GetPopupCitizenID());
          if (e) {
            Vector2 targetPos = {e->position.x * 10.0f + 5.0f,
                                 e->position.y * 10.0f + 5.0f};
            camera.target = Vector2Add(
                camera.target,
                Vector2Scale(Vector2Subtract(targetPos, camera.target), 0.1f));
          }
        }

        // Right-click entity selection for Player Control Mode (on PRESS, not drag)
        // This MUST use IsMouseButtonPressed (not Down) to avoid conflict with camera panning
        static int selectedEntityID = -1;
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
          Vector2 worldMousePos = GetScreenToWorld2D(mousePos, camera);
          float exMouseX = worldMousePos.x / 10.0f;  // exact position in world tiles
          float exMouseY = worldMousePos.y / 10.0f;
          
          // Find closest intelligent entity within radius (increased tolerance to 2.5 tiles = 25 pixels)
          float closestDist = 2.5f;
          int closestID = -1;
          for (const auto &e : world.GetEntities()) {
            if (e.IsIntelligent() && e.citizenID >= 0) {
              float dist = Vector2Distance(e.position, {exMouseX, exMouseY});
              if (dist < closestDist) {
                closestDist = dist;
                closestID = e.citizenID;
              }
            }
          }
          
          // Only set if we found someone
          if (closestID != -1) {
            selectedEntityID = closestID;
            TraceLog(LOG_INFO, "RIGHT-CLICK SELECTED: Citizen ID %d at screen pos (%.0f, %.0f)", closestID, mousePos.x, mousePos.y);
          } else {
            // Deselect if clicking on empty space
            selectedEntityID = -1;
            TraceLog(LOG_INFO, "RIGHT-CLICK: No entity found at (%.0f, %.0f)", exMouseX, exMouseY);
          }
        }

        // Player Control Mode
        static int playerControlledEntityID = -1;
        
        // ESC key to release control
        if (IsKeyPressed(KEY_ESCAPE) && playerControlledEntityID != -1) {
          Entity *ent = world.GetEntityByCitizenID(playerControlledEntityID);
          if (ent) ent->isPlayerControlled = false;
          playerControlledEntityID = -1;
          selectedEntityID = -1;
        }
        
        // E key to toggle control
        if (IsKeyPressed(KEY_E)) {
          // Toggle control of the selected human (either from right-click or popup)
          int targetID = (selectedEntityID != -1) ? selectedEntityID : ui.GetPopupCitizenID();
          if (targetID != -1) {
            const Entity *e = world.GetEntityByCitizenID(targetID);
            if (e && e->IsIntelligent()) {
              if (playerControlledEntityID == targetID) {
                // Release control
                Entity *ent = world.GetEntityByCitizenID(playerControlledEntityID);
                if (ent) ent->isPlayerControlled = false;
                playerControlledEntityID = -1;
                TraceLog(LOG_INFO, "RELEASED control of citizen %d", targetID);
              } else {
                // Take control
                if (playerControlledEntityID != -1) {
                  Entity *old = world.GetEntityByCitizenID(playerControlledEntityID);
                  if (old) old->isPlayerControlled = false;
                }
                playerControlledEntityID = targetID;
                Entity *ent = world.GetEntityByCitizenID(playerControlledEntityID);
                if (ent) ent->isPlayerControlled = true;
                TraceLog(LOG_INFO, "CONTROLLING citizen %d", targetID);
              }
            }
          }
        }
        
        // Handle player input for controlled entity
        if (playerControlledEntityID != -1) {
          Entity *controlled = world.GetEntityByCitizenID(playerControlledEntityID);
          if (controlled && controlled->isPlayerControlled) {
            // Movement
            Vector2 moveDir = {0, 0};
            if (IsKeyDown(KEY_W)) moveDir.y -= 1;
            if (IsKeyDown(KEY_S)) moveDir.y += 1;
            if (IsKeyDown(KEY_A)) moveDir.x -= 1;
            if (IsKeyDown(KEY_D)) moveDir.x += 1;
            if (Vector2Length(moveDir) > 0) {
              moveDir = Vector2Normalize(moveDir);
              controlled->state = EntityState::Walking;
              // Update Animation
              controlled->animTime += GetFrameTime();
              if (controlled->animTime > 0.15f) { // Frame speed
                controlled->animTime = 0.0f;
                controlled->currentFrame++;
              }
              Vector2 newPos = Vector2Add(controlled->position, Vector2Scale(moveDir, controlled->speed * GetFrameTime()));
              if (world.IsWalkable((int)newPos.x, (int)newPos.y)) {
                controlled->position = newPos;
              }
              // Facing
              if (fabs(moveDir.x) > fabs(moveDir.y)) {
                controlled->facingDirection = (moveDir.x > 0) ? 1 : -1;
              } else {
                controlled->facingDirection = (moveDir.y > 0) ? 0 : 2;
              }
            } else {
              controlled->state = EntityState::Idle;
              controlled->currentFrame = 0; // Reset frame when idle
            }

            // Attack
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && controlled->attackCooldown <= 0) {
              controlled->state = EntityState::Attack;
              controlled->currentFrame = 0;
              controlled->attackCooldown = controlled->attackSpeed;
              // Attack logic similar to AI
              for (auto &other : world.GetEntitiesMutable()) {
                if ((other.type == EntityType::Boar || other.type == EntityType::Slime || other.type == EntityType::Dragon) && other.health > 0) {
                  float dist = Vector2Distance(controlled->position, other.position);
                  if (dist < 1.5f) {
                    float dmg = (controlled->type == EntityType::HumanArmed) ? 10.0f : 4.0f;
                    if (controlled->isHero) dmg += 10.0f;
                    other.health -= dmg;
                    if (other.health <= 0) other.state = EntityState::Die;
                  }
                }
              }
            }

            // Camera follow
            Vector2 targetPos = {controlled->position.x * 10.0f + 5.0f, controlled->position.y * 10.0f + 5.0f};
            camera.target = Vector2Add(camera.target, Vector2Scale(Vector2Subtract(targetPos, camera.target), 0.1f));
          } else {
            // Entity died or invalid
            playerControlledEntityID = -1;
          }
        }

        // Camera Bounds Clamping (Prevent losing sandbox rendering limits)
        float mapPixelW = world.GetWidth() * 10.0f;
        float mapPixelH = world.GetHeight() * 10.0f;
        camera.target.x = Clamp(camera.target.x, 0.0f, mapPixelW);
        camera.target.y = Clamp(camera.target.y, 0.0f, mapPixelH);

        // World Interaction (Painting)
        ui.Update(world, camera);

        world.Update();
        world.UpdateAnimation(GetFrameTime());
        world.UpdateSpawnEffects(GetFrameTime());

        ClearBackground(GetColor(0x1a1a2eFF));

        BeginMode2D(camera);
        worldRenderer.Draw(camera, &world.GetSimulation().GetCities());
        world.DrawSpawnEffects(); // Spawn particles in world space

        // Draw visual feedback for selected entity (in world space)
        if (selectedEntityID != -1) {
          const Entity *selectedEnt = world.GetEntityByCitizenID(selectedEntityID);
          if (selectedEnt) {
            Vector2 worldPos = Vector2Scale(selectedEnt->position, 10.0f);
            // Draw selection circle around entity
            DrawCircleLines((int)worldPos.x, (int)worldPos.y, 15, YELLOW);
          }
        }

        // Brush Preview
        if (!ui.IsPointerOnUI() && !ui.IsBrushPopupOpen()) {
          Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
          int tx = (int)(worldPos.x / 10);
          int ty = (int)(worldPos.y / 10);
          int size = (ui.GetBrushSize() == BrushSize::Single)
                         ? 1
                         : (int)ui.GetBrushSize();
          float rectSize = size * 10.0f;
          float offX = (size == 1) ? 0 : -(size / 2) * 10.0f;
          float offY = (size == 1) ? 0 : -(size / 2) * 10.0f;

          DrawRectangleLinesEx(
              {tx * 10.0f + offX, ty * 10.0f + offY, rectSize, rectSize},
              1 / camera.zoom, WHITE);
        }

        EndMode2D();

        // Draw UI feedback for selected entity (in screen space)
        if (selectedEntityID != -1) {
          const Entity *selectedEnt = world.GetEntityByCitizenID(selectedEntityID);
          if (selectedEnt) {
            Vector2 screenPos = GetWorldToScreen2D(Vector2Scale(selectedEnt->position, 10.0f), camera);
            // Draw "Press E to Control" text
            const char *controlText = playerControlledEntityID == selectedEntityID ? "Press E to Release" : "Press E to Control";
            int textWidth = MeasureText(controlText, 16);
            DrawText(controlText, (int)(screenPos.x - textWidth/2), (int)(screenPos.y - 35), 16, YELLOW);
          }
        }

        // Decorative clouds (world-space, above world, below UI)
        cloudManager.Update(world.GetWidth() * 10.0f, GetFrameTime());
        cloudManager.Draw(camera);

        // DEBUG: Show tile info under cursor
        {
          Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
          int tx = (int)(worldPos.x / 10);
          int ty = (int)(worldPos.y / 10);
          if (tx >= 0 && tx < world.GetWidth() && ty >= 0 &&
              ty < world.GetHeight()) {
            const Tile &tile = world.GetTile(tx, ty);
            DrawText(TextFormat("Tile(%d,%d) Type:%d", tx, ty, (int)tile.type),
                     10, 60, 16, YELLOW);
          }
        }

        // UI Render
        ui.Draw(world);
      }

      EndDrawing();
    }

    cloudManager.Unload();
    AudioManager::Get().Unload();
    ui.Unload();
  }

  CloseAudioDevice();
  CloseWindow();
  return 0;
}
