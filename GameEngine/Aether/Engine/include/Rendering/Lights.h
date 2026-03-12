#pragma once

#include <glm/vec3.hpp>

namespace Aether {

struct DirectionalLight {
  // Direction the light rays travel (will be normalized by users / renderer).
  glm::vec3 direction{-0.3f, -1.0f, -0.2f};
  glm::vec3 color{1.0f, 1.0f, 1.0f};
  float intensity = 1.0f;
};

struct PointLight {
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::vec3 color{1.0f, 1.0f, 1.0f};
  float intensity = 5.0f;
  float radius = 10.0f;
};

} // namespace Aether

