#include "City.h"

City CreateCity(int id, Vector2 center, const std::string &name, Color color) {
  City c;
  c.id = id;
  c.center = center;
  c.name = name;
  c.color = color;
  c.isAlive = true;
  c.age = 0.0f;

  // Initial resources
  c.resources.food = 50;
  c.resources.wood = 20;
  c.resources.stone = 10;

  // Initial territory (just the center tile)
  c.territory.push_back(center);

  return c;
}
