#pragma once
#include <string>

// Forward declarations to avoid heavy includes in the header
class World;

class SaveManager {
public:
  // Saves the game state for the given slot (1-12)
  static bool SaveGame(int slotIndex, const World &world);

  // Saves the game state asynchronously to avoid stalling the main thread (Disk
  // I/O)
  static void SaveGameAsync(const std::string &filename, const World &world);

  // Loads the game state for the given slot (1-12)
  static bool LoadGame(int slotIndex, World &world);

  // Checks if a save file exists for a given slot
  static bool SaveExists(int slotIndex);

  // Deletes a save file for a given slot
  static bool DeleteSave(int slotIndex);

  // Save directory name
  static constexpr const char *SAVE_DIR = "saves";

private:
  // Ensures the save directory exists, creating it if necessary
  static void EnsureSaveDirectory();

  static std::string GetSaveFilePath(int slotIndex);
};
