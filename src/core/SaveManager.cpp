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
         ".sav";
}

std::string SaveManager::GetLegacySaveFilePath(int slotIndex) {
  return std::string(SAVE_DIR) + "/save_slot_" + std::to_string(slotIndex) +
         ".json";
}

bool SaveManager::SaveExists(int slotIndex) {
  return fs::exists(GetSaveFilePath(slotIndex)) ||
         fs::exists(GetLegacySaveFilePath(slotIndex));
}

bool SaveManager::SaveGame(int slotIndex, const World &world) {
  EnsureSaveDirectory();

  json j;

  // Serialize World (which deeply serializes Terrain, Entities, and
  // SimulationManager)
  j["world"] = world;

  std::string path = GetSaveFilePath(slotIndex);
  std::ofstream o(path, std::ios::binary);
  if (!o.is_open()) {
    TraceLog(LOG_ERROR, "SAVE: Failed to open file for writing: %s",
             path.c_str());
    return false;
  }

  // Convert and Write to Binary MessagePack format! Extremely light weight.
  std::vector<uint8_t> v = json::to_msgpack(j);
  o.write(reinterpret_cast<const char *>(v.data()), v.size());
  o.close();

  TraceLog(LOG_INFO, "SAVE: Game natively saved (MsgPack) to '%s' [%zu bytes]",
           path.c_str(), v.size());
  return true;
}

bool SaveManager::LoadGame(int slotIndex, World &world) {
  std::string binaryPath = GetSaveFilePath(slotIndex);
  std::string legacyPath = GetLegacySaveFilePath(slotIndex);

  if (fs::exists(binaryPath)) {
    // Load binary (MessagePack)
    std::ifstream i(binaryPath, std::ios::binary);
    if (!i.is_open()) {
      TraceLog(LOG_ERROR, "SAVE: Failed to open binary file for reading: %s",
               binaryPath.c_str());
      return false;
    }

    // Extract file bytes into native memory
    std::vector<uint8_t> v((std::istreambuf_iterator<char>(i)),
                           std::istreambuf_iterator<char>());
    i.close();

    try {
      json j = json::from_msgpack(v);
      if (j.contains("world")) {
        j.at("world").get_to(world);
      }
      TraceLog(LOG_INFO, "SAVE: Game loaded from MsgPack '%s'",
               binaryPath.c_str());
      return true;
    } catch (const std::exception &e) {
      TraceLog(LOG_ERROR, "SAVE: Corrupted msgpack file! Parsing Exception: %s",
               e.what());
      return false;
    }
  } else if (fs::exists(legacyPath)) {
    // Load legacy JSON
    std::ifstream i(legacyPath);
    if (!i.is_open()) {
      TraceLog(LOG_ERROR, "SAVE: Failed to open JSON file for reading: %s",
               legacyPath.c_str());
      return false;
    }

    try {
      json j;
      i >> j;

      if (j.contains("world")) {
        j.at("world").get_to(world);
      }
      TraceLog(LOG_INFO, "SAVE: Game loaded from Legacy JSON '%s'",
               legacyPath.c_str());
      return true;
    } catch (const std::exception &e) {
      TraceLog(LOG_ERROR,
               "SAVE: Corrupted JSON legacy file! Parsing Exception: %s",
               e.what());
      return false;
    }
  }

  TraceLog(LOG_ERROR, "SAVE: No save file found for slot %d", slotIndex);
  return false;
}

bool SaveManager::DeleteSave(int slotIndex) {
  bool deleted = false;
  std::string binaryPath = GetSaveFilePath(slotIndex);
  if (fs::exists(binaryPath)) {
    fs::remove(binaryPath);
    TraceLog(LOG_INFO, "SAVE: Deleted save file '%s'", binaryPath.c_str());
    deleted = true;
  }

  std::string legacyPath = GetLegacySaveFilePath(slotIndex);
  if (fs::exists(legacyPath)) {
    fs::remove(legacyPath);
    TraceLog(LOG_INFO, "SAVE: Deleted legacy JSON save file '%s'",
             legacyPath.c_str());
    deleted = true;
  }

  return deleted;
}
