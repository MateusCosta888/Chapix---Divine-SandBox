#pragma once
#include "../core/TimeManager.h"
#include "../world/World.h"
#include "raylib.h"

// Enums moved from main.cpp
// Enums moved from main.cpp
enum class UIState { Terrain, Nature, Rocks, Creatures, Settings };

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
public:
  // State - Moved to public for main.cpp access (or use Get/Set)
  bool showBrushPopup = false;
  bool showTimePopup = false;
  bool popupJustOpened = false;

private:
  // Textures
  Texture2D texCursor;

  // New Taskbar Textures (9-slice)
  Texture2D texPanelTL, texPanelTC, texPanelTR;
  Texture2D texPanelML, texPanelMC, texPanelMR;
  Texture2D texPanelBL, texPanelBC, texPanelBR;

  // Font
  Font uiFont;

  // Shader
  Shader circleMaskShader;

  // Scrolling State
  float scrollOffset = 0.0f;
  float maxScroll = 0.0f;
  float contentWidth = 0.0f;
  float visibleWidth = 0.0f;

  // Button Texture
  Texture2D texButton;    // Using boto003.png as requested
  Texture2D texTabButton; // Using ButtomAbas.png

  // Custom Icons
  Texture2D texEraser;
  Texture2D texIconWaterDeep;
  Texture2D texIconWaterOcean;
  Texture2D texIconWaterShallow;

  UIState currentTab = UIState::Terrain;
  BrushSize currentBrushSize = BrushSize::S;
  int selectedToolIndex = 0;
  float cursorScale = 0.5f;

  // Constants (Internal to UI)
  static const int SCREEN_WIDTH = 1024;
  static const int SCREEN_HEIGHT = 768;
  static const int TOOLBAR_HEIGHT = 120;
  static const int TAB_HEIGHT = 50;

  // Helper methods
  void DrawToolbar(const World &world);
  void HandleInput(World &world, Camera2D &camera);
};
