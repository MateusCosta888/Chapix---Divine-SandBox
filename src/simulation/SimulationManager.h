#pragma once
#include "../utils/JsonHelpers.h"
#include "Citizen.h"
#include "City.h"
#include "Kingdom.h"
#include <map>
#include <memory>
#include <vector>

// Forward declaration
class World;

// ============================================================================
// SIMULATION MANAGER
// Central system that manages all Citizens, Cities, and Kingdoms
// ============================================================================
class SimulationManager {
public:
  SimulationManager();
  ~SimulationManager();

  // === MAIN UPDATE LOOP ===
  void Update(World &world, float deltaTime);

  // === RESET (clear all data for new game) ===
  void Reset();

  // === CITIZEN MANAGEMENT ===
  int AddCitizen(const Citizen &citizen);
  Citizen *GetCitizen(int id);
  const Citizen *GetCitizen(int id) const;
  void RemoveCitizen(int id);
  const std::map<int, Citizen> &GetAllCitizens() const { return citizens; }

  // === CITY MANAGEMENT ===
  int AddCity(const City &city);
  City *GetCity(int id);
  const City *GetCity(int id) const;
  void RemoveCity(int id);
  const std::map<int, City> &GetAllCities() const { return cities; }

  // === KINGDOM MANAGEMENT ===
  int AddKingdom(const Kingdom &kingdom);
  Kingdom *GetKingdom(int id);
  const Kingdom *GetKingdom(int id) const;
  void RemoveKingdom(int id);
  const std::map<int, Kingdom> &GetAllKingdoms() const { return kingdoms; }
  void DeclareWar(int kingdomA, int kingdomB, World &world);

  // === BATTLEFIELD MANAGEMENT ===
  int AddBattlefield(const Battlefield &bf);
  Battlefield *GetBattlefield(int id);
  const Battlefield *GetBattlefield(int id) const;
  void RemoveBattlefield(int id);
  const std::map<int, Battlefield> &GetAllBattlefields() const {
    return battlefields;
  }

  // === BUILDING MANAGEMENT ===
  int AddBuilding(const Building &building);
  Building *GetBuilding(int id);
  const Building *GetBuilding(int id) const;
  void RemoveBuilding(int id);

  // Destroy all buildings at a tile position (used when water covers a tile)
  void DestroyBuildingsAtTile(int tileX, int tileY);

  // Access all buildings
  const std::map<int, Building> &GetAllBuildings() const { return buildings; }

  // Helper to sync collision for existing buildings (e.g. after load)
  void RebuildOccupationMap(World &world);

  // Expose for World to call when spawning entities
  void AddCitizenToCity(int cityID, int citizenID);

  // === HELPERS ===
  int GetNextCitizenID() { return nextCitizenID++; }
  int GetNextCityID() { return nextCityID++; }
  int GetNextKingdomID() { return nextKingdomID++; }
  int GetNextBuildingID() { return nextBuildingID++; }

  // Stats
  int GetTotalPopulation() const { return static_cast<int>(citizens.size()); }
  int GetTotalCities() const { return static_cast<int>(cities.size()); }
  int GetTotalKingdoms() const { return static_cast<int>(kingdoms.size()); }

  // Access cities map for rendering
  const std::map<int, City> &GetCities() const { return cities; }

private:
  // === CITY FOUNDING ===
  float ScoreTileForCity(World &world, int x, int y) const;
  int FoundCity(World &world, int founderCitizenID, int x, int y);

  // === CITY EXPANSION ===
  // === CITY EXPANSION ===
  void ExpandTerritory(City &city, World &world);
  void UpdateBuildingUpgrade(City &city); // New: Evolution System

  // === GOVERNMENT SYSTEM ===
  // === GOVERNMENT SYSTEM ===
  // === GOVERNMENT SYSTEM ===
  void AssignJobs(City &city);

  // === CITY EVOLUTION (New) ===
  void AttemptTerritoryExpansion(City &city, World &world);
  void AttemptConstruction(City &city, World &world);

  // === HOUSING SYSTEM ===
  void AssignHousing(City &city);

private:
  // === DATA STORAGE ===
  std::map<int, Citizen> citizens;
  std::map<int, City> cities;
  std::map<int, Kingdom> kingdoms;
  std::map<int, Building> buildings;
  std::map<int, Battlefield> battlefields;

  // === ID COUNTERS ===
  int nextCitizenID = 1;
  int nextCityID = 1;
  int nextKingdomID = 1;
  int nextBuildingID = 1;
  int nextBattlefieldID = 1;

  // === UPDATE SUBSYSTEMS ===
  void UpdateCitizens(World &world, float deltaTime);
  void UpdateCities(World &world, float deltaTime);
  void UpdateKingdoms(World &world, float deltaTime);
  void UpdateDiplomacy(World &world, float deltaTime);

  // === TIME TRACKING ===
  float gameTime = 0.0f;  // Total elapsed time
  float yearTimer = 0.0f; // Timer for year cycle
  int currentYear = 1;

  // === CONFIG ===
  float secondsPerYear = 60.0f; // Real seconds per in-game year (1 minute)

public:
  // === TIME GETTERS ===
  int GetCurrentYear() const { return currentYear; }
  float GetYearProgress() const { return yearTimer / secondsPerYear; }

  // === SAVE & LOAD ===
  NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
      SimulationManager, citizens, cities, kingdoms, buildings, battlefields,
      nextCitizenID, nextCityID, nextKingdomID, nextBuildingID,
      nextBattlefieldID, gameTime, yearTimer, currentYear, secondsPerYear)
};
