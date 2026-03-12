#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace Aether {

struct CameraInputState {
  bool lookActive = false;
  float mouseDeltaX = 0.0f;
  float mouseDeltaY = 0.0f;

  bool moveForward = false;
  bool moveBackward = false;
  bool moveLeft = false;
  bool moveRight = false;
  bool moveUp = false;
  bool moveDown = false;
  bool fast = false;
};

class Camera {
public:
  Camera() = default;

  void SetPosition(const glm::vec3& position) { m_position = position; }
  const glm::vec3& GetPosition() const { return m_position; }

  void SetYawPitch(float yawDegrees, float pitchDegrees);

  float GetFovDegrees() const { return m_fovDegrees; }
  void SetFovDegrees(float fovDegrees) { m_fovDegrees = fovDegrees; }

  glm::mat4 GetView() const;
  glm::mat4 GetProjection(float aspect) const;

  void UpdateFromInput(float deltaSeconds, const CameraInputState& input);

private:
  glm::vec3 GetForward() const;
  glm::vec3 GetRight() const;

  glm::vec3 m_position{0.0f, 0.0f, 3.0f};
  float m_yawDegrees = -90.0f;
  float m_pitchDegrees = 0.0f;
  float m_fovDegrees = 60.0f;

  float m_moveSpeed = 5.0f;
  float m_mouseSensitivity = 0.10f; // degrees per pixel
};

} // namespace Aether

