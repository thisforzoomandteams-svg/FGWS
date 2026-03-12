#include "ECS/Entity.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

namespace Aether {

glm::mat4 Transform::ToMatrix() const {
  glm::mat4 m(1.0f);
  m = glm::translate(m, position);

  // ZYX order is a reasonable default for editor-style euler controls.
  m = glm::rotate(m, glm::radians(rotationDeg.z), glm::vec3(0.0f, 0.0f, 1.0f));
  m = glm::rotate(m, glm::radians(rotationDeg.y), glm::vec3(0.0f, 1.0f, 0.0f));
  m = glm::rotate(m, glm::radians(rotationDeg.x), glm::vec3(1.0f, 0.0f, 0.0f));

  m = glm::scale(m, scale);
  return m;
}

} // namespace Aether

