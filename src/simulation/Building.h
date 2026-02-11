#pragma once
#include "raylib.h"
#include <string>

// ============================================================================
// BUILDING TYPES
// ============================================================================
enum class BuildingType {
  None = 0,
  Cabana,         // Basic housing - 2 wood
  Casa,           // Medium housing - 5 wood, 2 stone
  Casa2,          // Large housing - 8 wood, 5 stone
  Recursos,       // Storage - 3 wood
  Mercado,        // Trade - 10 wood, 5 stone
  Taverna,        // Happiness - 8 wood, 3 stone
  Workshop,       // Crafting - 10 wood, 8 stone
  Quartel,        // Military - 15 wood, 10 stone
  Torre,          // Defense - 5 wood, 10 stone
  Castelo,        // Capital - 50 wood, 40 stone
  Docks,          // Naval - 20 wood, 5 stone
  Poco,           // Water well - 5 wood, 3 stone
  StockpileStone, // Stone Storage - 50 wood
  COUNT
};

// ============================================================================
// BUILDING COSTS
// ============================================================================
struct BuildingCost {
  int wood;
  int stone;
  int food;
};

// Get the cost to build a specific building type
inline BuildingCost GetBuildingCost(BuildingType type) {
  switch (type) {
  case BuildingType::Cabana:
    return {2, 0, 0};
  case BuildingType::Casa:
    return {5, 2, 0};
  case BuildingType::Casa2:
    return {8, 5, 0};
  case BuildingType::Recursos:
    return {3, 0, 0};
  case BuildingType::Mercado:
    return {10, 5, 0};
  case BuildingType::Taverna:
    return {8, 3, 0};
  case BuildingType::Workshop:
    return {10, 8, 0};
  case BuildingType::Quartel:
    return {15, 10, 0};
  case BuildingType::Torre:
    return {5, 10, 0};
  case BuildingType::Castelo:
    return {50, 40, 0};
  case BuildingType::Docks:
    return {20, 5, 0};
  case BuildingType::Poco:
    return {5, 3, 0};
  case BuildingType::StockpileStone:
    return {50, 0, 0};
  default:
    return {0, 0, 0};
  }
}

// Get the housing capacity provided by a building
inline int GetBuildingHousingCapacity(BuildingType type) {
  switch (type) {
  case BuildingType::Cabana:
    return 2; // 2 people per cabana
  case BuildingType::Casa:
    return 4; // 4 people per casa
  case BuildingType::Casa2:
    return 6; // 6 people per large casa
  default:
    return 0;
  }
}

struct BuildingSize {
  int width;
  int height;
};

inline BuildingSize GetBuildingSize(BuildingType type) {
  switch (type) {
  case BuildingType::Cabana:
    return {2, 2};
  case BuildingType::Casa:
    return {2, 2};
  case BuildingType::Casa2:
    return {3, 3}; // Large house
  case BuildingType::Recursos:
    return {2, 2};
  case BuildingType::StockpileStone:
    return {2, 2};
  case BuildingType::Mercado:
    return {3, 3};
  case BuildingType::Taverna:
    return {3, 2};
  case BuildingType::Workshop:
    return {3, 3};
  case BuildingType::Quartel:
    return {4, 4};
  case BuildingType::Torre:
    return {2, 2};
  case BuildingType::Castelo:
    return {6, 6};
  case BuildingType::Docks:
    return {3, 3};
  case BuildingType::Poco:
    return {2, 2};
  default:
    return {1, 1};
  }
}

// Get the name of a building type
inline const char *GetBuildingName(BuildingType type) {
  switch (type) {
  case BuildingType::Cabana:
    return "Cabana";
  case BuildingType::Casa:
    return "Casa";
  case BuildingType::Casa2:
    return "Casa Grande";
  case BuildingType::Recursos:
    return "Armazem";
  case BuildingType::StockpileStone:
    return "Estoque de Pedra";
  case BuildingType::Mercado:
    return "Mercado";
  case BuildingType::Taverna:
    return "Taverna";
  case BuildingType::Workshop:
    return "Oficina";
  case BuildingType::Quartel:
    return "Quartel";
  case BuildingType::Torre:
    return "Torre";
  case BuildingType::Castelo:
    return "Castelo";
  case BuildingType::Docks:
    return "Porto";
  case BuildingType::Poco:
    return "Poco";
  default:
    return "Desconhecido";
  }
}

// ============================================================================
// BUILDING STRUCT
// ============================================================================
struct Building {
  int id = -1;
  BuildingType type = BuildingType::None;
  int tileX = 0;
  int tileY = 0;
  int cityID = -1;
  int variant = 0; // Texture variant (0-4 for cabanas, etc.)
  float health = 100.0f;
  float maxHealth = 100.0f;
  bool isComplete = true;
  float constructionProgress = 1.0f; // 0.0 to 1.0

  bool IsHousing() const {
    return type == BuildingType::Cabana || type == BuildingType::Casa ||
           type == BuildingType::Casa2;
  }
};
