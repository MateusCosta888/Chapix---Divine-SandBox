#include "Kingdom.h"

Kingdom CreateKingdom(int id, const std::string &name, Color color,
                      int capitalCityID) {
  Kingdom k;
  k.id = id;
  k.name = name;
  k.color = color;
  k.capitalCityID = capitalCityID;
  k.isAlive = true;
  k.age = 0.0f;

  if (capitalCityID >= 0) {
    k.cityIDs.push_back(capitalCityID);
  }

  return k;
}

int Kingdom::GetTotalPopulation() const {
  // Note: This requires access to cities vector, which we don't have here.
  // This will be implemented in SimulationManager where we have access to all
  // data.
  return 0; // Placeholder
}
