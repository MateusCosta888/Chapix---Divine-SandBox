#include "SaveManager.h"
#include "../simulation/SimulationManager.h"
#include "../utils/json.hpp"
#include "../world/World.h"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

std::string SaveManager::GetSaveFilePath(int slotIndex) {
  return "save_slot_" + std::to_string(slotIndex) + ".json";
}

bool SaveManager::SaveExists(int slotIndex) {
  std::ifstream f(GetSaveFilePath(slotIndex));
  return f.good();
}

bool SaveManager::SaveGame(int slotIndex, const World &world) {
  json j;

  // Serialize World (which deeply serializes Terrain, Entities, and
  // SimulationManager)
  j["world"] = world;

  std::string path = GetSaveFilePath(slotIndex);
  std::ofstream o(path);
  if (!o.is_open())
    return false;

  o << j.dump(4); // 4 spaces indent
  o.close();

  return true;
}

bool SaveManager::LoadGame(int slotIndex, World &world) {
  std::string path = GetSaveFilePath(slotIndex);
  std::ifstream i(path);
  if (!i.is_open())
    return false;

  json j;
  i >> j;

  // Deserialize World onto the existing object reference
  if (j.contains("world")) {
    j.at("world").get_to(world);
  }

  return true;
}
