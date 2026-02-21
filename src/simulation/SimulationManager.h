#pragma once
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

  // === BUILDING MANAGEMENT ===
  int AddBuilding(const Building &building);
  Building *GetBuilding(int id);
  const Building *GetBuilding(int id) const;
  void RemoveBuilding(int id);

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

  // === ID COUNTERS ===
  int nextCitizenID = 1;
  int nextCityID = 1;
  int nextKingdomID = 1;
  int nextBuildingID = 1;

  // === UPDATE SUBSYSTEMS ===
  void UpdateCitizens(World &world, float deltaTime);
  void UpdateCities(World &world, float deltaTime);
  void UpdateKingdoms(World &world, float deltaTime);

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
};
