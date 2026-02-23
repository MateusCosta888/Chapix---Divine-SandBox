#pragma once
#include <string>

// Forward declarations to avoid heavy includes in the header
class World;

class SaveManager {
public:
  // Saves the game state for the given slot (1-3)
  static bool SaveGame(int slotIndex, const World &world);

  // Loads the game state for the given slot (1-3)
  static bool LoadGame(int slotIndex, World &world);

  // Checks if a save file exists for a given slot
  static bool SaveExists(int slotIndex);

private:
  static std::string GetSaveFilePath(int slotIndex);
};
