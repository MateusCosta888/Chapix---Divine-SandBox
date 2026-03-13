#include "../utils/JsonHelpers.h"
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
  bool isFemale = false;

  // === VITAL STATS ===
  float age = 0.0f;      // In game-years
  float health = 100.0f; // Current HP
  float maxHealth = 100.0f;
  float hunger = 0.0f;   // 0 = full, 100 = starving
  float energy = 100.0f; // Fatigue system
  float bodyTemperature = 37.0f; // Normal=37, below 30=cold damage

  // === HOUSING & DAILY LIFE ===
  int homeID = -1;          // ID of the building this citizen lives in
  bool isResting = false;   // True if inside home recovering
  bool isGoingHome = false; // True if traveling to home

  // === STATS & GENETICS ===
  // === STATS & GENETICS ===
  CitizenStats stats;
  CitizenGenetics genes;

  // === RPG PROGRESSION ===
  int level = 1;
  float experience = 0.0f;
  float maxExperience = 100.0f;

  // === INVENTORY (6 Slots) ===
  struct InventorySlot {
    int type = 0; // 0:None, 1:Wood, 2:Stone, 3:Food
    int amount = 0;
  };
  InventorySlot inventory[6];

  // === ABILITIES (8 Slots) ===
  bool abilities[8] = {false}; // true if unlocked/active

  // === SOCIAL ===
  int cityID = -1;    // Which city do I belong to?
  int kingdomID = -1; // Which kingdom?
  Profession profession = Profession::None;

  // === JOB CONTEXT ===
  enum class JobType {
    None,
    Farming,
    PlantingTree,
    Woodcutting,
    Mining,
    Building
  };
  JobType currentJob = JobType::None;

  // === SKILLS (0.0 to 100.0 proficiency) ===
  float skillFarming = 0.0f;
  float skillWoodcutting = 0.0f;
  float skillMining = 0.0f;
  float skillBuilding = 0.0f;
  float skillCombat = 0.0f;

  // === WORK STATE ===
  enum class WorkState {
    Idle,         // Not working, waiting for task
    Wandering,    // Moving randomly (because no task found)
    GoingToWork,  // Moving to work target
    Working,      // Actively working (chopping, farming, etc.)
    ReturningHome, // Carrying resources back to city
    Hunting       // Hunting animals for food
  };

  WorkState workState = WorkState::Idle;
  WorkState lastWorkState = WorkState::Idle; // For state change detection
  bool isWorking = false;
  float workTimer = 0.0f;
  float stateTimer = 0.0f; // Time in current work state

  // Job target (tile coordinates or entity ID)
  int targetTileX = -1;
  int targetTileY = -1;
  int targetEntityID = -1;  // Entity ID for combat tracking
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

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CitizenStats, strength, intelligence, speed,
                                   endurance)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CitizenGenetics, baseStrength,
                                   baseIntelligence, baseSpeed, baseEndurance,
                                   maxAge)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Citizen::InventorySlot, type, amount)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    Citizen, id, name, isFemale, age, health, maxHealth, hunger, energy,
    homeID, isResting, isGoingHome, stats, genes, level,
    experience, maxExperience, inventory, abilities, cityID, kingdomID,
    profession, currentJob, skillFarming, skillWoodcutting, skillMining,
    skillBuilding, skillCombat, workState, lastWorkState, isWorking, workTimer,
    stateTimer, targetTileX, targetTileY, targetEntityID, carryingResource,
    maxCarryCapacity, motherID, fatherID, spouseID, isAlive)
