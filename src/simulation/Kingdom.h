#pragma once
#include "../utils/JsonHelpers.h"
#include "raylib.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Forward declarations
struct City;

// ============================================================================
// DIPLOMATIC RELATION STATUS
// ============================================================================
enum class DiplomaticStatus {
  Neutral,  // No opinion
  Friendly, // Trade partners
  Allied,   // Military alliance
  Hostile,  // At war
  Vassal,   // Subjugated
  Overlord  // Has vassals
};

// ============================================================================
// KINGDOM STRUCTURE
// ============================================================================
struct Kingdom {
  // === IDENTITY ===
  int id = -1;
  std::string name = "New Kingdom";
  Color color = {200, 100, 100, 255}; // Red-ish default

  // === LEADERSHIP ===
  int leaderCitizenID = -1; // The king/queen
  int capitalCityID = -1;   // Capital city

  // === CITIES ===
  std::vector<int> cityIDs; // All cities in this kingdom

  // === DIPLOMACY ===
  // Map of KingdomID -> Relation score (-100 to +100)
  std::map<int, float> relations;
  // Map of KingdomID -> Diplomatic Status
  std::map<int, DiplomaticStatus> diplomaticStatus;

  // === KINGDOM TRAITS (aggregate of cities) ===
  float totalAggression = 0.3f;
  float totalDiplomacy = 0.5f;

  // === STATS ===
  bool isAlive = true;
  float age = 0.0f;

  // === HELPERS ===
  int GetTotalPopulation() const; // Sum of all city populations
  int GetCityCount() const { return static_cast<int>(cityIDs.size()); }

  // Get relation with another kingdom (-100 to +100)
  float GetRelation(int otherKingdomID) const {
    auto it = relations.find(otherKingdomID);
    if (it != relations.end())
      return it->second;
    return 0.0f; // Neutral
  }

  void SetRelation(int otherKingdomID, float value) {
    if (value < -100.0f)
      value = -100.0f;
    if (value > 100.0f)
      value = 100.0f;
    relations[otherKingdomID] = value;
  }

  void ModifyRelation(int otherKingdomID, float delta) {
    SetRelation(otherKingdomID, GetRelation(otherKingdomID) + delta);
  }

  bool IsAtWarWith(int otherKingdomID) const {
    auto it = diplomaticStatus.find(otherKingdomID);
    return it != diplomaticStatus.end() &&
           it->second == DiplomaticStatus::Hostile;
  }

  bool IsAlliedWith(int otherKingdomID) const {
    auto it = diplomaticStatus.find(otherKingdomID);
    return it != diplomaticStatus.end() &&
           it->second == DiplomaticStatus::Allied;
  }
};

// ============================================================================
// BATTLEFIELD STRUCTURE
// ============================================================================
struct Battlefield {
  int id = -1;
  int kingdomA = -1;
  int kingdomB = -1;
  Vector2 centerPos = {0.0f, 0.0f};
  float timer = 60.0f; // 60 seconds duration
  int killsA = 0;
  int killsB = 0;
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================
Kingdom CreateKingdom(int id, const std::string &name, Color color,
                      int capitalCityID);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Battlefield, id, kingdomA, kingdomB,
                                   centerPos, timer, killsA, killsB)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Kingdom, id, name, color, leaderCitizenID,
                                   capitalCityID, cityIDs, relations,
                                   diplomaticStatus, totalAggression,
                                   totalDiplomacy, isAlive, age)
