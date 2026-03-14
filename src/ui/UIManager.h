#pragma once
#include "../core/TimeManager.h"
#include "../world/World.h"
#include "raylib.h"
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

// Enums moved from main.cpp
// Enums moved from main.cpp
enum class UIState {
  Terrain,
  Nature,
  Rocks,
  Creatures,
  Social,
  Powers,
  Settings
};

enum class BrushSize {
  Single = 0, // Special case for exact 1x1 painting
  S = 1,
  M = 3,
  L = 5,
  XL = 9
};

enum class GameState { MAIN_MENU, PLAYING };

class UIManager {
public:
  // UI Notification Structure
  struct UINotification {
    std::string text;
    int iconFlagID;
    float timer;
    float maxTimer;
    float yOffset; // For stacking multiple notifications
  };

  UIManager();
  ~UIManager();

  void Load(std::function<void(const char *)> loadingCallback = nullptr);
  void Unload();
  void Update(World &world, Camera2D &camera);
  void Draw(const World &world);

  bool IsPointerOnUI() const;
  bool IsBrushPopupOpen() const { return showBrushPopup; }
  bool IsAnyPopupOpen() const;
  UIState GetCurrentTab() const { return currentTab; }
  BrushSize GetBrushSize() const { return currentBrushSize; }

  // New Accessors
  bool IsHumanPopupOpen() const { return showHumanPopup; }
  int GetPopupCitizenID() const { return popupCitizenID; }

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

  // New Popup Textures (9-slice) for City UI
  Texture2D texPopupTL, texPopupTC, texPopupTR;
  Texture2D texPopupML, texPopupMC, texPopupMR;
  Texture2D texPopupBL, texPopupBC, texPopupBR;

  // City Popup State
  bool showCityPopup = false;
  int popupCityID = -1;
  bool isRenamingCity = false;
  char cityRenameBuffer[64] = "City Name";
  float cityPopupScroll = 0.0f;
  bool showFlagSelector = false;
  float flagSelectorScroll = 0.0f;

  // Social / Kingdom List State
  bool showSocialCityList = false;
  float socialCityListScroll = 0.0f;
  Vector2 socialPopupPos = {0, 0};
  bool isDraggingSocial = false;

  // Force War State
  bool forceWarMode = false;        // True when selecting kingdoms
  int forceWarKingdomA = -1;        // First selected kingdom
  int forceWarKingdomB = -1;        // Second selected kingdom
  bool showForceWarConfirm = false; // Show confirmation popup

  // Save Popup State
  bool showSavePopup = false;
  Vector2 savePopupPos = {0, 0};
  bool isDraggingSave = false;
  bool showConfirmOverwrite = false;
  int confirmSlot = -1;

  // Options Popup State
  bool showOptionsPopup = false;
  int selectedResolution = -1; // -1 = current/custom, 0-3 = preset index
  bool isBorderlessFullscreen = false; // Track borderless state manually

  // Autosave Notification State
  bool isAutosaving = false;
  float autosaveUIRemaining = 0.0f;

  // PergaminhoBackgod (Save UI) 9-Slice Textures
  Texture2D texPergaminhoTL, texPergaminhoTC, texPergaminhoTR;
  Texture2D texPergaminhoML, texPergaminhoMC, texPergaminhoMR;
  Texture2D texPergaminhoBL, texPergaminhoBC, texPergaminhoBR;

public:
  void ShowAutosaveNotification();

  // Main Menu State
  GameState currentState = GameState::MAIN_MENU;
  bool isMainMenuNight = false;
  bool shouldStartGame = false;
  bool shouldExitGame = false;

  // Save Slot Action State
  bool showSaveActionPopup = false;
  bool showDeleteConfirmPopup = false;
  int actionSlotID = -1;

  // World Creator State
  bool showWorldCreatorPopup = false;
  int creatingSlotID = -1;
  int newWorldBaseSize = 1;   // 0=Small(64), 1=Med(128), 2=Large(256)
  int newWorldWaterLevel = 1; // 0=Low, 1=Normal, 2=High
  int newWorldTreeLevel = 1;  // 0=Low, 1=Normal, 2=High

  bool isSeedInputActive = false;
  char newWorldSeedInput[32] = "\0";

  // World Creator Assets & Anim state
  Texture2D texCreatorBg;
  std::vector<Texture2D> texPlanetAnim;
  float planetAnimTimer = 0.0f;
  int planetAnimFrame = 0;

  void UpdateMainMenu(World &world);
  void DrawMainMenu(const World &world);

private:
  // Main Menu Textures
  Texture2D texMenuDayBg;
  Texture2D texMenuDayBorder;
  Texture2D texMenuNightBg;
  Texture2D texMenuNightBorder;
  Texture2D texMenuLogo;
  Texture2D texMenuIlustration;
  Texture2D texMenuBtnStart;
  Texture2D texMenuBtnOptions;
  Texture2D texMenuBtnExit;

  // Drag State (City)
  Vector2 cityPopupPos = {0, 0};
  bool isDraggingCity = false;
  Vector2 dragOffset = {0, 0};

  // Human/Creature Popup State
  bool showHumanPopup = false;
  int popupCitizenID = -1;
  char humanRenameBuffer[64] = {0};
  bool isRenamingHuman = false;
  int humanWindowTab = 0; // 0 = Main, 1 = Abilities
  bool showProfessionSelector = false;

  // Human Spawn Menu State (popup on Human creature button)
  bool showHumanSpawnMenu = false;
  Vector2 humanSpawnMenuPos = {0, 0};
  int humanSpawnSelection = -1; // -1=none, 0=random, 1=man, 2=woman

  // Drag State (Human)
  Vector2 humanPopupPos = {0, 0};
  bool isDraggingHuman = false;

  // New Human Popup Textures (9-slice Base Pup-Up2)
  Texture2D texPopup2TL, texPopup2TC, texPopup2TR;
  Texture2D texPopup2ML, texPopup2MC, texPopup2MR;
  Texture2D texPopup2BL, texPopup2BC, texPopup2BR;

  // Social Popup Textures (Violet Background)
  Texture2D texPopup3TL, texPopup3TC, texPopup3TR;
  Texture2D texPopup3ML, texPopup3MC, texPopup3MR;
  Texture2D texPopup3BL, texPopup3BC, texPopup3BR;

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
  Texture2D texButton; // Using boto003.png as requested

  Texture2D texTabButton;  // Using ButtomAbas.png
  Texture2D texTabRedFlag; // New Tab Button (RedFlagButton.png)

  // Custom Icons
  Texture2D texEraser;
  Texture2D texIconWaterDeep;
  Texture2D texIconWaterOcean;
  Texture2D texIconWaterShallow;
  Texture2D texSaveIcon;

  // Power Icons
  Texture2D texIconLightning;
  Texture2D texIconFire;

  UIState currentTab = UIState::Terrain;
  BrushSize currentBrushSize = BrushSize::S;
  int selectedToolIndex = 0;
  float cursorScale = 0.5f;

  // Constants (Internal to UI) - dynamic based on window size
  int getScreenW() const { return GetScreenWidth(); }
  int getScreenH() const { return GetScreenHeight(); }
  static const int TOOLBAR_HEIGHT = 120;
  static const int TAB_HEIGHT = 50;

  // Helper methods
  void DrawToolbar(const World &world);
  void DrawCityPopup(const World &world); // New City UI
  void DrawHumanPopup(const World &world);
  void DrawSocialCityList(const World &world);     // New Social Popup
  void DrawSavePopup(const World &world);          // New Save Menu
  void DrawFlagSelector(const World &world);       // Flag Selector UI
  void DrawProfessionSelector(const World &world); // Profession Selector UI
  void DrawOptionsPopup();                         // Options/Settings Popup
  void DrawNotifications(const World &world);      // Draw Notifications UI
  void UpdateWarNotifications(const World &world);  // Detect new wars and notify
  void HandleInput(World &world, Camera2D &camera);

  // Notification Methods
  void AddNotification(const std::string &text, int flagID);

  // General Application State Helpers
  int lastKnownCityID = -1; // To track newly founded cities
  std::vector<UINotification> activeNotifications;

  // Track known war pairs to avoid duplicate notifications
  std::map<int, std::set<int>> knownWarPairs;
};
