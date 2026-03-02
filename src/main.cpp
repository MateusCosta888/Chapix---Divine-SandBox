#include "core/AudioManager.h"
#include "core/CrashHandler.h"
#include "core/TimeManager.h"
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

  {
    uint32_t seed = 0;
    if (argc > 1 && std::string(argv[1]) == "--seed") {
      seed = std::stoul(argv[2]);
    } else {
      seed = (uint32_t)GetTime();
    }

    World world(128, 128, seed);
    world.Generate();
    world.LoadTextures();

    WorldRenderer worldRenderer(world);

    UIManager ui;
    ui.Load();

    // Load music
    AudioManager::Get().Load();
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

    AudioManager::Get().Unload();
    ui.Unload();
  }

  CloseAudioDevice();
  CloseWindow();
  return 0;
}
