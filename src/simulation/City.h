#pragma once
#include "../utils/JsonHelpers.h"
#include "Building.h"
#include "raylib.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>


// Forward declarations
struct Citizen;

// ============================================================================
// RESOURCE STORAGE
// ============================================================================
struct CityResources {
  int food = 0;
  int wood = 0;
  int stone = 0;
  int ore = 0;
  int gold = 0;
};

// ============================================================================
// CITY STRUCTURE
// ============================================================================
struct City {
  // === IDENTITY ===
  int id = -1;
  std::string name = "New Settlement";
  Color color = {100, 100, 200, 255}; // Blue-ish default
  int flagID = 0;                     // Visual flag identifier (0-79)

  // === LOCATION ===
  Vector2 center = {0, 0}; // Town hall position
  int kingdomID = -1;

  // === POPULATION ===
  std::vector<int> citizenIDs; // List of citizen IDs belonging to this city
  int populationCap = 10;      // Max population (based on housing)

  // === TERRITORY ===
  std::vector<Vector2> territory; // Tiles claimed by this city
  int territoryRadius = 5;        // Base claim radius

  // === RESOURCES ===
  CityResources resources;
  int maxStorage = 200; // Max resources can hold

  // === BUILDINGS ===
  std::vector<Building> buildings; // Buildings in this city

  // === CULTURE TRAITS (0.0 to 1.0) ===
  float cultureAggression = 0.3f; // War tendency
  float cultureDiplomacy = 0.5f;  // Trade/alliance tendency
  float cultureExpansion = 0.5f;  // Territory growth desire
  float cultureIndustry = 0.5f;   // Work ethic
  float cultureReligion = 0.3f;   // (Future: Beliefs)

  // === TECH LEVEL (unlocks buildings/units) ===
  int techLevel = 1;
  float knowledgePoints = 0.0f;

  // === STATS ===
  bool isAlive = true;
  float age = 0.0f; // How long city has existed

  // === HELPERS ===
  int GetPopulation() const { return static_cast<int>(citizenIDs.size()); }
  bool HasCapacity() const { return true; } // No population limit!
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================
City CreateCity(int id, Vector2 center, const std::string &name, Color color);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CityResources, food, wood, stone, ore, gold)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(City, id, name, color, flagID, center,
                                   kingdomID, citizenIDs, populationCap,
                                   territory, territoryRadius, resources,
                                   maxStorage, buildings, cultureAggression,
                                   cultureDiplomacy, cultureExpansion,
                                   cultureIndustry, cultureReligion, techLevel,
                                   knowledgePoints, isAlive, age)
