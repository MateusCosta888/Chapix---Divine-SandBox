#pragma once

// Biome Types - Determines regional climate and allowed terrain
enum class BiomeType {
  Ocean,    // Water areas (not land)
  Desert,   // Hot + Dry: Sand, Rock, Cacti, Palm trees
  Plains,   // Temperate: Grass, Sand (coast)
  Forest,   // Temperate + Humid: Trees, Grass
  Mountain, // High altitude: Rock
  Snow      // Cold: Snow, Ice, Snow trees
};

// Terrain Types - Visual representation of ground
enum class TileType {
  DeepOcean,    // Darkest, deepest water
  Ocean,        // Medium depth water
  ShallowOcean, // Light water (coastal)
  Sand,         // Beach sand (temperate)
  DesertSand,   // Desert sand (hot)
  Grass,
  Forest,
  Mountain,
  Snow
};

// Decoration Types - Objects placed on top of terrain
enum class DecorationType {
  None,
  Tree,     // Generic tree (varies by biome)
  PineTree, // Snow/Mountain tree
  PalmTree, // Desert/Beach tree
  Bush,
  Rock,    // Placeholder for generic rock (will map to Rock1)
  BigRock, // Existing big rock
  Flower,
  Mushroom,
  SmallRock, // New distinct type (Rock2)
  MediumRock // New distinct type (Rock3)
};

struct Tile {
  TileType type = TileType::DeepOcean;
  BiomeType biome = BiomeType::Ocean;

  // Decoration
  DecorationType decoration = DecorationType::None;
  int decorationVariant = 0; // 0-3 for random variations
  int variant = 0;           // Pre-calculated terrain variant

  // Simulation properties
  float height = 0.0f;      // 0.0 - 1.0
  float temperature = 0.5f; // 0.0 (cold) - 1.0 (hot)
  float humidity = 0.5f;    // 0.0 (dry) - 1.0 (wet)

  // Logic flags
  bool hasRiver = false;
};
