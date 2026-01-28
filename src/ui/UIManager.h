#pragma once
#include "../world/World.h"
#include "raylib.h"


// Enums moved from main.cpp
enum class UIState { Terrain, Nature, Rocks, Settings };

enum class BrushSize {
  Single = 0, // Special case for exact 1x1 painting
  S = 1,
  M = 3,
  L = 5,
  XL = 9
};

class UIManager {
public:
  UIManager();
  ~UIManager();

  void Load();
  void Unload();
  void Update(World &world, Camera2D &camera);
  void Draw(const World &world);

  bool IsPointerOnUI() const;
  bool IsBrushPopupOpen() const { return showBrushPopup; }
  UIState GetCurrentTab() const { return currentTab; }
  BrushSize GetBrushSize() const { return currentBrushSize; }

private:
  // Textures
  Texture2D texButton;
  Texture2D texPanel;
  Texture2D texTab;
  Texture2D texCursor;

  // State
  bool showBrushPopup = false;
  bool popupJustOpened = false;
  UIState currentTab = UIState::Terrain;
  BrushSize currentBrushSize = BrushSize::S;
  int selectedToolIndex = 0;

  // Constants (Internal to UI)
  static const int SCREEN_WIDTH = 1024;
  static const int SCREEN_HEIGHT = 768;
  static const int TOOLBAR_HEIGHT = 100;
  static const int TAB_HEIGHT = 30;

  // Helper methods
  void DrawToolbar(const World &world);
  void HandleInput(World &world, Camera2D &camera);
};
