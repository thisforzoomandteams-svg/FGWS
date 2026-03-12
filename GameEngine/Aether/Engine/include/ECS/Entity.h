#pragma once

#include <cstdint>
#include <string>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace Aether {

struct Transform {
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::vec3 rotationDeg{0.0f, 0.0f, 0.0f};
  glm::vec3 scale{1.0f, 1.0f, 1.0f};

  glm::mat4 ToMatrix() const;
};

struct Entity {
  uint32_t id = 0;
  std::string name;
  Transform transform;

  glm::mat4 GetTransformMatrix() const { return transform.ToMatrix(); }
};

} // namespace Aether

