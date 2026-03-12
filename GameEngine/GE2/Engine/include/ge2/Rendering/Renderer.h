#pragma once

#include <glm/mat4x4.hpp>
#include <memory>

namespace ge2
{
    class Framebuffer;
    class Scene;

    class Renderer
    {
    public:
        Renderer();
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        void Resize(unsigned int width, unsigned int height);

        void BeginFrame(const glm::mat4& view, const glm::mat4& proj);
        void RenderScene(Scene& scene, int selectedEntityId);
        void EndFrame();

        Framebuffer& GetFramebuffer() { return *m_Framebuffer; }

    private:
        void InitGLResources();
        void ShutdownGLResources();

    private:
        std::unique_ptr<Framebuffer> m_Framebuffer;

        unsigned int m_CubeVAO = 0;
        unsigned int m_CubeVBO = 0;
        unsigned int m_CubeIBO = 0;

        unsigned int m_PlaneVAO = 0;
        unsigned int m_PlaneVBO = 0;
        unsigned int m_PlaneIBO = 0;

        unsigned int m_ShaderProgram = 0;
        unsigned int m_WireframeShader = 0;

        glm::mat4 m_View{};
        glm::mat4 m_Proj{};
    };
}

