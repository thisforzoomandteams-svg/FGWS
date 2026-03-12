#pragma once

#include <glm/vec3.hpp>

namespace Aether {

struct Material {
  glm::vec3 albedo{0.0f, 0.90f, 1.0f};

  // Specular is optional: set specularStrength to 0 to disable.
  glm::vec3 specularColor{1.0f, 1.0f, 1.0f};
  float specularStrength = 0.5f;
  float shininess = 32.0f;
};

} // namespace Aether

