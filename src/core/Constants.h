#pragma once

#include <cstdint>

// ============================================================================
// GAME CONSTANTS - Centralized configuration values
// ============================================================================
// All magic numbers should be replaced by named constants in this file.
// This improves maintainability and makes values self-documenting.
// ============================================================================

namespace Constants {

  // === SIMULATION TIMING ===
  constexpr float SECONDS_PER_YEAR = 60.0f;           // 1 minute per game year
  constexpr float CITIZEN_AGE_MULTIPLIER_CHILD = 5.0f; // Children grow 5x faster
  constexpr float CITIZEN_BASE_AGE_RATE = 0.05f;      // Base aging per second
  constexpr float HUNGER_RATE = 0.1f;                 // Hunger increase per second
  constexpr float STARVATION_DAMAGE_RATE = 1.0f;      // HP loss when starving
  constexpr float ENEMY_SCAN_INTERVAL = 0.5f;         // Seconds between enemy scans
  constexpr float COOLDOWN_RESET_THRESHOLD = 0.0f;    // Min cooldown value

  // === SPAWNING ===
  constexpr float CASTLE_SOLDIER_SPAWN_INTERVAL = 30.0f;   // Seconds
  constexpr int MAX_SOLDIERS_PER_CASTLE = 5;
  constexpr float WORLD_EVENT_CHECK_INTERVAL = 120.0f;     // 2 minutes
  constexpr float DRAGON_SPAWN_CHANCE = 0.1f;              // 10% per check
  constexpr int MAX_DRAGONS_IN_WORLD = 3;                  // Keep dragons rare and boss-like
  constexpr float ANIMAL_SPAWN_CHECK_INTERVAL = 5.0f;      // Birth check every 5s
  constexpr int MAX_ANIMALS_PER_SPECIES = 30;

  // === CITY ===
  constexpr int CITY_FOUNDATION_RADIUS = 3;
  constexpr int CITY_TERRITORY_RADIUS = 5;
  constexpr float CITY_FOUNDATION_COOLDOWN = 120.0f;  // Seconds between city founding
  constexpr int MIN_CITIES_DISTANCE = 15;             // Tiles between cities
  constexpr int BASE_CABANA_HOUSING = 2;
  constexpr int MAX_STORAGE_PER_CITY = 200;

  // === BUILDING ===
  constexpr int BUILDING_EXPANSION_WOOD_COST = 30;
  constexpr int BUILDING_EXPANSION_FOOD_COST = 30;
  constexpr float REGROWTH_CHECK_INTERVAL = 5.0f;       // Stump regrowth check
  constexpr float REGROWTH_TIME = 90.0f;                // Seconds for stump to regrow
  constexpr int MAX_STUMP_REGROWTH_ATTEMPTS = 3;

  // === COMBAT ===
  constexpr float BOAR_DAMAGE = 12.0f;
  constexpr float SLIME_DAMAGE = 6.0f;
  constexpr float DRAGON_DAMAGE = 40.0f;
  constexpr float HUMAN_UNARMED_DAMAGE = 4.0f;
  constexpr float HUMAN_ARMED_DAMAGE = 10.0f;
  constexpr float HERO_DAMAGE_BONUS = 10.0f;
  constexpr float BLOCK_CHANCE_ARMED = 0.2f;  // 20% block chance

  // === ENTITIES ===
  constexpr float HUMAN_HEALTH_UNARMED = 60.0f;      // Buffed from 20 to 60 for survival
  constexpr float HUMAN_HEALTH_ARMED = 120.0f;      // Buffed from 50 to 120
  constexpr float HUMAN_SPEED_UNARMED = 2.0f;
  constexpr float HUMAN_SPEED_ARMED = 2.1f;
  constexpr float BOAR_SPEED = 2.4f;
  constexpr float SLIME_SPEED = 1.3f;
  constexpr float DRAGON_SPEED = 3.0f;
  constexpr float DRAGON_MAX_HP = 300.0f;

  // === WORLD GENERATION ===
  constexpr int MOUNTAIN_FILTER_MIN_NEIGHBORS = 15;   // Min neighbors to keep mountain
  constexpr float MOUNTAIN_THRESHOLD_GRASS = 0.80f;
  constexpr float MOUNTAIN_THRESHOLD_OTHER = 0.85f;
  constexpr float MOUNTAIN_REGION_RADIUS_MAJOR = 0.35f; // % of map size
  constexpr float MOUNTAIN_REGION_RADIUS_MINOR = 0.15f;
  constexpr int SAFE_ZONE_PADDING = 10;

  // === AI ===
  constexpr float FLEE_RADIUS = 8.0f;
  constexpr int STUCK_TIMEOUT_GOING_TO_WORK = 40;  // Seconds
  constexpr int STUCK_TIMEOUT_WORKING = 60;        // Seconds
  constexpr float HOMELESS_REST_THRESHOLD = 10.0f; // Energy to rest
  constexpr float WITH_HOME_REST_THRESHOLD = 20.0f;
  constexpr float WANDER_CHECK_INTERVAL = 3.0f;
  constexpr int WANDER_RADIUS = 5;

  // === WAR SYSTEM ===
  constexpr float WAR_CHECK_INTERVAL = 120.0f;           // 2 minutes
  constexpr float RELATION_WAR_THRESHOLD = -50.0f;       // Natural war relation
  constexpr float RELATION_PEACE_THRESHOLD = 0.0f;       // Peace relation
  constexpr float WAR_RELATION_DECAY_PER_TICK = 0.1f;    // Worsens during war

  // === AUTOSAVE ===
  constexpr float AUTOSAVE_INTERVAL = 300.0f;  // 5 minutes
  constexpr int MAX_AUTOSAVE_SLOTS = 2;

  // === UI ===
  constexpr int UI_TOOLBAR_HEIGHT = 120;
  constexpr int UI_TAB_HEIGHT = 50;
  constexpr float AUTOSAVE_NOTIFICATION_DURATION = 3.0f;

  // === GOD POWERS ===
  constexpr float LIGHTNING_RADIUS = 4.5f;
  constexpr float LIGHTNING_DAMAGE = 40.0f;
  constexpr float FIRE_RADIUS = 3.5f;
  constexpr float FIRE_DAMAGE = 15.0f;
  constexpr float TORNADO_RADIUS = 5.0f;
  constexpr float TORNADO_DAMAGE = 20.0f;
  constexpr float FIRE_BOMB_RADIUS = 5.0f;
  constexpr float FIRE_BOMB_DAMAGE = 30.0f;
  constexpr float DARK_BOLT_RADIUS = 4.0f;
  constexpr float DARK_BOLT_DAMAGE = 50.0f;
  constexpr float THUNDER2_RADIUS = 4.5f;
  constexpr float THUNDER2_DAMAGE = 35.0f;

  // === SPAWN EFFECTS ===
  constexpr int MAX_SPAWN_PARTICLES = 10;
  constexpr float SPAWN_POPIN_TIME = 0.15f;
  constexpr int SPAWN_PARTICLE_COUNT_MIN = 6;
  constexpr int SPAWN_PARTICLE_COUNT_MAX = 10;

  // === COLD SYSTEM ===
  constexpr float NORMAL_BODY_TEMP = 37.0f;
  constexpr float COLD_DAMAGE_THRESHOLD = 30.0f;
  constexpr float COLD_DAMAGE_RATE = 3.0f;
  constexpr float COOLING_RATE_SNOW_NO_SHELTER = 2.0f;
  constexpr float COOLING_RATE_SNOW_WITH_SHELTER = 0.4f;  // 80% reduction
  constexpr float WARMING_RATE = 1.5f;

  // === LEVEL UP ===
  constexpr float LEVEL_UP_BASE_XP = 50.0f;
  constexpr float LEVEL_UP_XP_INCREMENT = 25.0f;
  constexpr float LEVEL_UP_STRENGTH_BONUS = 0.5f;
  constexpr float LEVEL_UP_SPEED_BONUS = 0.3f;
  constexpr float LEVEL_UP_ENDURANCE_BONUS = 0.5f;
  constexpr float LEVEL_UP_INTELLIGENCE_BONUS = 0.3f;
  constexpr int LEVEL_UP_HP_BONUS = 5;
  constexpr int LEVEL_UP_CARRY_BONUS = 1;

  // === REPRODUCTION ===
  constexpr float ANIMAL_REPRODUCTION_COOLDOWN = 60.0f;  // Seconds
  constexpr float ANIMAL_BABY_SPEED_BONUS = 0.8f;        // 80% of parent speed
  constexpr float ANIMAL_BABY_HP_BONUS = 0.5f;           // 50% of parent HP

  // === GAMEPLAY LIMITS ===
  constexpr int MAX_PARTICLES_IN_SCENE = 100;
  constexpr int MAX_ACTIVE_NOTIFICATIONS = 10;
  constexpr int MAX_SPAWN_EFFECTS = 100;

} // namespace Constants
