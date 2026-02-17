#include "Citizen.h"
#include <cstdlib>
#include <ctime>
#include <string>

// Helper for random float in range
static float RandFloat(float min, float max) {
  return min + static_cast<float>(rand()) /
                   (static_cast<float>(RAND_MAX / (max - min)));
}

// Male first names (medieval/fantasy style)
static const char *MALE_FIRST_NAMES[] = {
    "Aldric",  "Bjorn",  "Cedric",  "Darian",  "Edmund", "Fenris", "Gavin",
    "Harald",  "Igor",   "Jasper",  "Klaus",   "Leif",   "Magnus", "Nils",
    "Oskar",   "Petrus", "Ragnar",  "Stefan",  "Thorin", "Ulric",  "Viktor",
    "Wilhelm", "Yorick", "Zoran",   "Alden",   "Bram",   "Conrad", "Dorian",
    "Erik",    "Finn",   "Gunnar",  "Halvard", "Ivan",   "Jorik",  "Kael",
    "Lothar",  "Marcus", "Nikolai", "Otto",    "Pavel"};
static const int NUM_MALE_NAMES =
    sizeof(MALE_FIRST_NAMES) / sizeof(MALE_FIRST_NAMES[0]);

// Female first names
static const char *FEMALE_FIRST_NAMES[] = {
    "Astrid", "Brenna", "Celeste", "Dagny",  "Elara",  "Freya",  "Greta",
    "Helga",  "Ingrid", "Johanna", "Kira",   "Luna",   "Maren",  "Nora",
    "Olga",   "Petra",  "Rosa",    "Sigrid", "Thea",   "Ursula", "Vera",
    "Wanda",  "Ylva",   "Zelda",   "Anja",   "Birgit", "Clara",  "Dagna",
    "Eira",   "Fiona",  "Gudrun",  "Hilda",  "Irina",  "Katla",  "Lena",
    "Mira",   "Nessa",  "Oda",     "Runa",   "Solveig"};
static const int NUM_FEMALE_NAMES =
    sizeof(FEMALE_FIRST_NAMES) / sizeof(FEMALE_FIRST_NAMES[0]);

// Last name prefixes and suffixes for combination
static const char *LAST_PREFIXES[] = {
    "Iron",  "Stone", "Black", "Storm", "Wolf",   "Oak",  "Silver", "Gold",
    "Frost", "Fire",  "Dark",  "Swift", "Strong", "Tall", "Bold",   "Red"};
static const int NUM_PREFIXES =
    sizeof(LAST_PREFIXES) / sizeof(LAST_PREFIXES[0]);

static const char *LAST_SUFFIXES[] = {
    "hammer", "forge", "wood", "shield", "blade", "heart", "son",   "berg",
    "guard",  "hand",  "helm", "born",   "wind",  "field", "brook", "fen"};
static const int NUM_SUFFIXES =
    sizeof(LAST_SUFFIXES) / sizeof(LAST_SUFFIXES[0]);

// Generate a random name
static std::string GenerateRandomName(bool isFemale) {
  std::string name;
  if (isFemale)
    name = FEMALE_FIRST_NAMES[rand() % NUM_FEMALE_NAMES];
  else
    name = MALE_FIRST_NAMES[rand() % NUM_MALE_NAMES];

  // 50% chance to add a last name
  if (rand() % 2 == 0) {
    name += " ";
    name += LAST_PREFIXES[rand() % NUM_PREFIXES];
    name += LAST_SUFFIXES[rand() % NUM_SUFFIXES];
  }

  return name;
}

Citizen CreateRandomCitizen(int id, int cityID) {
  Citizen c;
  c.id = id;
  c.cityID = cityID;
  c.isFemale = (rand() % 2 == 0);
  c.name = GenerateRandomName(c.isFemale);
  c.age = RandFloat(18.0f, 35.0f); // Start as adult
  c.health = 100.0f;
  c.maxHealth = 100.0f;
  c.hunger = RandFloat(0.0f, 20.0f);
  c.energy = RandFloat(80.0f, 100.0f);

  // Random stats
  c.stats.strength = RandFloat(3.0f, 8.0f);
  c.stats.intelligence = RandFloat(3.0f, 8.0f);
  c.stats.speed = RandFloat(3.0f, 8.0f);
  c.stats.endurance = RandFloat(3.0f, 8.0f);

  // Copy stats to genetics
  c.genes.baseStrength = c.stats.strength;
  c.genes.baseIntelligence = c.stats.intelligence;
  c.genes.baseSpeed = c.stats.speed;
  c.genes.baseEndurance = c.stats.endurance;
  c.genes.maxAge = RandFloat(70.0f, 90.0f);

  c.isAlive = true;
  return c;
}

Citizen CreateChildCitizen(int id, const Citizen &mother, const Citizen &father,
                           int cityID) {
  Citizen c;
  c.id = id;
  c.cityID = cityID;
  c.age = 0.0f; // Newborn
  c.isFemale = (rand() % 2 == 0);

  // Generate name - child gets first name, may inherit father's last name
  std::string firstName;
  if (c.isFemale)
    firstName = FEMALE_FIRST_NAMES[rand() % NUM_FEMALE_NAMES];
  else
    firstName = MALE_FIRST_NAMES[rand() % NUM_MALE_NAMES];

  // Check if father has a last name (contains space)
  size_t spacePos = father.name.find(' ');
  if (spacePos != std::string::npos) {
    // Inherit father's last name
    c.name = firstName + father.name.substr(spacePos);
  } else {
    // No last name to inherit, just first name
    c.name = firstName;
  }

  // Inherit genetics (average of parents + small mutation)
  c.genes.baseStrength =
      (mother.genes.baseStrength + father.genes.baseStrength) / 2.0f +
      RandFloat(-1.0f, 1.0f);
  c.genes.baseIntelligence =
      (mother.genes.baseIntelligence + father.genes.baseIntelligence) / 2.0f +
      RandFloat(-1.0f, 1.0f);
  c.genes.baseSpeed = (mother.genes.baseSpeed + father.genes.baseSpeed) / 2.0f +
                      RandFloat(-1.0f, 1.0f);
  c.genes.baseEndurance =
      (mother.genes.baseEndurance + father.genes.baseEndurance) / 2.0f +
      RandFloat(-1.0f, 1.0f);
  c.genes.maxAge = (mother.genes.maxAge + father.genes.maxAge) / 2.0f +
                   RandFloat(-5.0f, 5.0f);

  // Current stats start equal to base
  c.stats.strength = c.genes.baseStrength;
  c.stats.intelligence = c.genes.baseIntelligence;
  c.stats.speed = c.genes.baseSpeed;
  c.stats.endurance = c.genes.baseEndurance;

  c.health = 100.0f;
  c.maxHealth = 100.0f;
  c.hunger = 0.0f;
  c.energy = 100.0f;

  c.motherID = mother.id;
  c.fatherID = father.id;
  c.isAlive = true;

  return c;
}
