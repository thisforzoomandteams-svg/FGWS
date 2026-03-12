#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace ge2
{
    class Camera
    {
    public:
        Camera();

        void SetViewportSize(float width, float height);
        void SetPosition(const glm::vec3& pos) { m_Position = pos; }
        const glm::vec3& GetPosition() const { return m_Position; }

        void SetYawPitch(float yaw, float pitch);
        float GetYaw() const { return m_Yaw; }
        float GetPitch() const { return m_Pitch; }

        float GetFOV() const { return m_FOV; }

        glm::mat4 GetViewMatrix() const;
        glm::mat4 GetProjectionMatrix() const;

        glm::vec3 GetForward() const;
        glm::vec3 GetRight() const;

        void ScreenToWorldRay(float u, float v, glm::vec3& outOrigin, glm::vec3& outDir) const;

    private:
        glm::vec3 m_Position{0.0f, 1.5f, 5.0f};
        float m_Yaw = -90.0f;
        float m_Pitch = -10.0f;
        float m_FOV = 45.0f;
        float m_AspectRatio = 16.0f / 9.0f;
        float m_NearClip = 0.1f;
        float m_FarClip = 100.0f;
    };
}

