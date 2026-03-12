#include "Rendering/Camera.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>

namespace Aether {

static float ClampPitch(float pitch) {
  return std::clamp(pitch, -89.0f, 89.0f);
}

void Camera::SetYawPitch(float yawDegrees, float pitchDegrees) {
  m_yawDegrees = yawDegrees;
  m_pitchDegrees = ClampPitch(pitchDegrees);
}

glm::vec3 Camera::GetForward() const {
  const float yawRad = glm::radians(m_yawDegrees);
  const float pitchRad = glm::radians(m_pitchDegrees);

  glm::vec3 forward;
  forward.x = std::cos(yawRad) * std::cos(pitchRad);
  forward.y = std::sin(pitchRad);
  forward.z = std::sin(yawRad) * std::cos(pitchRad);
  return glm::normalize(forward);
}

glm::vec3 Camera::GetRight() const {
  const glm::vec3 forward = GetForward();
  const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
  return glm::normalize(glm::cross(forward, worldUp));
}

glm::mat4 Camera::GetView() const {
  const glm::vec3 forward = GetForward();
  const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
  const glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
  const glm::vec3 up = glm::normalize(glm::cross(right, forward));
  return glm::lookAt(m_position, m_position + forward, up);
}

glm::mat4 Camera::GetProjection(float aspect) const {
  return glm::perspective(glm::radians(m_fovDegrees), aspect, 0.1f, 100.0f);
}

void Camera::UpdateFromInput(float deltaSeconds, const CameraInputState& input) {
  if (input.lookActive) {
    m_yawDegrees += input.mouseDeltaX * m_mouseSensitivity;
    m_pitchDegrees -= input.mouseDeltaY * m_mouseSensitivity;
    m_pitchDegrees = ClampPitch(m_pitchDegrees);
  }

  glm::vec3 moveDir(0.0f);
  const glm::vec3 forward = GetForward();
  const glm::vec3 right = GetRight();
  const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

  if (input.moveForward) moveDir += forward;
  if (input.moveBackward) moveDir -= forward;
  if (input.moveRight) moveDir += right;
  if (input.moveLeft) moveDir -= right;
  if (input.moveUp) moveDir += worldUp;
  if (input.moveDown) moveDir -= worldUp;

  const float speed = m_moveSpeed * (input.fast ? 3.0f : 1.0f);
  if (glm::dot(moveDir, moveDir) > 0.0f) {
    moveDir = glm::normalize(moveDir);
    m_position += moveDir * speed * deltaSeconds;
  }
}

} // namespace Aether
