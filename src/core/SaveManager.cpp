#include "SaveManager.h"
#include "../simulation/SimulationManager.h"
#include "../utils/json.hpp"
#include "../world/World.h"
#include <filesystem>
#include <fstream>
#include <iostream>

using json = nlohmann::json;
namespace fs = std::filesystem;

void SaveManager::EnsureSaveDirectory() {
  if (!fs::exists(SAVE_DIR)) {
    fs::create_directories(SAVE_DIR);
    TraceLog(LOG_INFO, "SAVE: Created save directory '%s'", SAVE_DIR);
  }
}

std::string SaveManager::GetSaveFilePath(int slotIndex) {
  return std::string(SAVE_DIR) + "/save_slot_" + std::to_string(slotIndex) +
         ".json";
}

bool SaveManager::SaveExists(int slotIndex) {
  return fs::exists(GetSaveFilePath(slotIndex));
}

bool SaveManager::SaveGame(int slotIndex, const World &world) {
  EnsureSaveDirectory();

  json j;

  // Serialize World (which deeply serializes Terrain, Entities, and
  // SimulationManager)
  j["world"] = world;

  std::string path = GetSaveFilePath(slotIndex);
  std::ofstream o(path);
  if (!o.is_open()) {
    TraceLog(LOG_ERROR, "SAVE: Failed to open file for writing: %s",
             path.c_str());
    return false;
  }

  o << j.dump(4); // 4 spaces indent
  o.close();

  TraceLog(LOG_INFO, "SAVE: Game saved to '%s'", path.c_str());
  return true;
}

bool SaveManager::LoadGame(int slotIndex, World &world) {
  std::string path = GetSaveFilePath(slotIndex);
  std::ifstream i(path);
  if (!i.is_open()) {
    TraceLog(LOG_ERROR, "SAVE: Failed to open file for reading: %s",
             path.c_str());
    return false;
  }

  json j;
  i >> j;

  // Deserialize World onto the existing object reference
  if (j.contains("world")) {
    j.at("world").get_to(world);
  }

  TraceLog(LOG_INFO, "SAVE: Game loaded from '%s'", path.c_str());
  return true;
}

bool SaveManager::DeleteSave(int slotIndex) {
  std::string path = GetSaveFilePath(slotIndex);
  if (fs::exists(path)) {
    fs::remove(path);
    TraceLog(LOG_INFO, "SAVE: Deleted save file '%s'", path.c_str());
    return true;
  }
  return false;
}
