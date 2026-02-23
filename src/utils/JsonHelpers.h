#pragma once
#include "json.hpp"
#include "raylib.h"

// Define JSON serialization for Raylib types
namespace nlohmann {
template <> struct adl_serializer<Color> {
  static void to_json(json &j, const Color &c) {
    j = json{{"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a}};
  }

  static void from_json(const json &j, Color &c) {
    j.at("r").get_to(c.r);
    j.at("g").get_to(c.g);
    j.at("b").get_to(c.b);
    j.at("a").get_to(c.a);
  }
};

template <> struct adl_serializer<Vector2> {
  static void to_json(json &j, const Vector2 &v) {
    j = json{{"x", v.x}, {"y", v.y}};
  }

  static void from_json(const json &j, Vector2 &v) {
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
  }
};
} // namespace nlohmann
