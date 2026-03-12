#pragma once

#include <glm/vec3.hpp>

namespace Aether {

struct PointLightComponent {
  glm::vec3 color{1.0f, 0.95f, 0.85f};
  float intensity = 8.0f;
  float radius = 8.0f;
};

} // namespace Aether

