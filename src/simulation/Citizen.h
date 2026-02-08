#pragma once
#include <cstdint>
#include <string>

// Forward declarations
struct City;
struct Kingdom;

// ============================================================================
// PROFESSION SYSTEM
// ============================================================================
enum class Profession {
  None,       // Unemployed / Child
  Gatherer,   // Collects berries, basic food
  Lumberjack, // Chops trees for wood
  Farmer,     // Works fields for food
  Miner,      // Extracts stone/ore
  Builder,    // Constructs buildings
  Soldier,    // Defends/Attacks
  Leader      // Manages city
};

// ============================================================================
// CITIZEN STATS
// ============================================================================
struct CitizenStats {
  float strength = 5.0f;     // Physical power (carry, chop, fight)
  float intelligence = 5.0f; // Learning speed, tech contribution
  float speed = 5.0f;        // Movement speed modifier
  float endurance = 5.0f;    // How long before fatigue
};

// ============================================================================
// CITIZEN GENETICS (for inheritance)
// ============================================================================
struct CitizenGenetics {
  float baseStrength = 5.0f;
  float baseIntelligence = 5.0f;
  float baseSpeed = 5.0f;
  float baseEndurance = 5.0f;
  float maxAge = 80.0f; // Genetic lifespan
};

// ============================================================================
// CITIZEN STRUCTURE
// ============================================================================
struct Citizen {
  // === IDENTITY ===
  int id = -1;
  std::string name = "Unnamed";

  // === VITAL STATS ===
  float age = 0.0f;      // In game-years
  float health = 100.0f; // Current HP
  float maxHealth = 100.0f;
  float hunger = 0.0f;   // 0 = full, 100 = starving
  float energy = 100.0f; // Fatigue system

  // === STATS & GENETICS ===
  CitizenStats stats;
  CitizenGenetics genes;

  // === SOCIAL ===
  int cityID = -1;    // Which city do I belong to?
  int kingdomID = -1; // Which kingdom?
  Profession profession = Profession::None;

  // === SKILLS (0.0 to 100.0 proficiency) ===
  float skillFarming = 0.0f;
  float skillWoodcutting = 0.0f;
  float skillMining = 0.0f;
  float skillBuilding = 0.0f;
  float skillCombat = 0.0f;

  // === WORK STATE ===
  enum class WorkState {
    Idle,         // Not working, wandering
    GoingToWork,  // Moving to work target
    Working,      // Actively working (chopping, farming, etc.)
    ReturningHome // Carrying resources back to city
  };

  WorkState workState = WorkState::Idle;
  bool isWorking = false;
  float workTimer = 0.0f;

  // Job target (tile coordinates or entity ID)
  int targetTileX = -1;
  int targetTileY = -1;
  int carryingResource = 0; // How much resource currently carrying
  int maxCarryCapacity = 5; // Max resources can carry at once

  // === FAMILY ===
  int motherID = -1;
  int fatherID = -1;
  int spouseID = -1;

  // === FLAGS ===
  bool isAlive = true;
  bool isAdult() const { return age >= 16.0f; }
  bool isElder() const { return age >= 60.0f; }
  bool isChild() const { return age < 16.0f; }
  bool isStarving() const { return hunger >= 80.0f; }
};

// ============================================================================
// HELPER: Create a new citizen with randomized stats
// ============================================================================
Citizen CreateRandomCitizen(int id, int cityID = -1);
Citizen CreateChildCitizen(int id, const Citizen &mother, const Citizen &father,
                           int cityID);
