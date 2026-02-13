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
const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;
const int TOOLBAR_HEIGHT = 100;
const int TAB_HEIGHT = 30;

// Enums moved to UIManager.h
// struct GameUI removed

int main(int argc, char *argv[]) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "ChapisX - Divine SandBox");
  SetTargetFPS(60);

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
  HideCursor();

  Camera2D camera = {0};
  camera.zoom = 2.0f;
  camera.offset = {SCREEN_WIDTH / 2.0f,
                   (SCREEN_HEIGHT - TOOLBAR_HEIGHT) / 2.0f};
  camera.target = {world.GetWidth() * 5.0f, world.GetHeight() * 5.0f};

  while (!WindowShouldClose()) {
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
        camera.target = Vector2Add(
            camera.target, Vector2Subtract(mouseWorldBefore, mouseWorldAfter));
      }
      if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = Vector2Scale(GetMouseDelta(), -1.0f / camera.zoom);
        camera.target = Vector2Add(camera.target, delta);
      }
    }

    // Camera Tracking (Human Popup)
    if (ui.IsHumanPopupOpen() && ui.GetPopupCitizenID() != -1) {
      // Find entity for this citizen to get position
      const auto &entities = world.GetEntities();
      for (const auto &e : entities) {
        if (e.citizenID == ui.GetPopupCitizenID()) {
          // Lerp to target for smooth tracking
          Vector2 targetPos = {e.position.x * 10.0f + 5.0f,
                               e.position.y * 10.0f + 5.0f};
          // Simple lerp: current + (target - current) * 0.1
          camera.target = Vector2Add(
              camera.target,
              Vector2Scale(Vector2Subtract(targetPos, camera.target), 0.1f));
          break;
        }
      }
    }

    // World Interaction (Painting)
    ui.Update(world, camera);

    world.Update(); // SimulateWater + UpdateEntities
    world.UpdateAnimation(GetFrameTime());
    if (IsKeyPressed(KEY_R)) {
      world.Reset((uint32_t)GetTime());
      world.Generate();
    }

    // --- DRAW ---
    BeginDrawing();
    ClearBackground(GetColor(0x1a1a2eFF));

    BeginMode2D(camera);
    worldRenderer.Draw(camera, &world.GetSimulation().GetCities());

    // Brush Preview
    // Brush Preview
    if (!ui.IsPointerOnUI() && !ui.IsBrushPopupOpen()) {
      Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
      int tx = (int)(worldPos.x / 10);
      int ty = (int)(worldPos.y / 10);
      int size =
          (ui.GetBrushSize() == BrushSize::Single) ? 1 : (int)ui.GetBrushSize();
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
        DrawText(TextFormat("Tile(%d,%d) Type:%d", tx, ty, (int)tile.type), 10,
                 60, 16, YELLOW);
      }
    }

    // UI Render
    ui.Draw(world);
    // DrawTextureEx(ui.texCursor, mousePos, 0, 0.5f, WHITE); // Handled by
    // ui.Draw()
    EndDrawing();
  }

  ui.Unload();
  CloseWindow();
  return 0;
}
