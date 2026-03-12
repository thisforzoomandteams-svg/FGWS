#include "ge2/Rendering/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace ge2
{
    Camera::Camera()
    {
    }

    void Camera::SetViewportSize(float width, float height)
    {
        if (height <= 0.0f)
            return;
        m_AspectRatio = width / height;
    }

    void Camera::SetYawPitch(float yaw, float pitch)
    {
        m_Yaw = yaw;
        m_Pitch = pitch;
    }

    glm::mat4 Camera::GetViewMatrix() const
    {
        glm::vec3 front = GetForward();
        return glm::lookAt(m_Position, m_Position + front, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 Camera::GetProjectionMatrix() const
    {
        return glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
    }

    glm::vec3 Camera::GetForward() const
    {
        glm::vec3 front;
        front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        front.y = sin(glm::radians(m_Pitch));
        front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        return glm::normalize(front);
    }

    glm::vec3 Camera::GetRight() const
    {
        return glm::normalize(glm::cross(GetForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    void Camera::ScreenToWorldRay(float u, float v, glm::vec3& outOrigin, glm::vec3& outDir) const
    {
        glm::mat4 proj = GetProjectionMatrix();
        glm::mat4 view = GetViewMatrix();
        glm::mat4 invVP = glm::inverse(proj * view);

        float x = 2.0f * u - 1.0f;
        float y = 1.0f - 2.0f * v;

        glm::vec4 nearPoint = invVP * glm::vec4(x, y, -1.0f, 1.0f);
        glm::vec4 farPoint  = invVP * glm::vec4(x, y,  1.0f, 1.0f);

        nearPoint /= nearPoint.w;
        farPoint  /= farPoint.w;

        outOrigin = glm::vec3(nearPoint);
        outDir = glm::normalize(glm::vec3(farPoint - nearPoint));
    }
}

