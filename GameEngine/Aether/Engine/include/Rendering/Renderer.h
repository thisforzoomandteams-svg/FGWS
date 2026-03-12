#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Rendering/Lights.h"
#include "Rendering/Material.h"
#include "Rendering/ShadowMap.h"

#include "Rendering/IndexBuffer.h"
#include "Rendering/Shader.h"
#include "Rendering/VertexArray.h"
#include "Rendering/VertexBuffer.h"

#include <array>
#include <memory>

namespace Aether {

class Window;
class Framebuffer;

struct RendererFrameData {
  glm::mat4 viewProj{1.0f};
  glm::vec3 cameraPos{0.0f, 0.0f, 0.0f};

  glm::vec3 ambientColor{0.05f, 0.05f, 0.05f};
  float ambientIntensity = 1.0f;

  DirectionalLight directionalLight{};
  glm::mat4 lightSpaceMatrix{1.0f};

  int pointLightCount = 0;
  std::array<PointLight, 8> pointLights{};
};

class Renderer {
public:
  static constexpr int kMaxPointLights = 8;

  Renderer() = default;
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  bool Init(Window& window);
  void Shutdown();

  void BeginFrame(Framebuffer* targetOrNull);
  void EndFrame();

  void Clear(const glm::vec4& color);

  void SetFrameData(const RendererFrameData& frame);

  void BeginShadowPass();
  void EndShadowPass();
  void DrawCubeShadow(const glm::mat4& model);

  void DrawCube(const glm::mat4& model, const Material& material);
  void DrawCubeWireframe(const glm::mat4& model, const glm::vec4& color);

private:
  Window* m_window = nullptr;
  RendererFrameData m_frame{};

  std::unique_ptr<Shader> m_litShader;
  std::unique_ptr<Shader> m_unlitShader;
  std::unique_ptr<Shader> m_depthShader;
  ShadowMap m_shadowMap;
  std::unique_ptr<VertexArray> m_cubeVao;
  std::unique_ptr<VertexBuffer> m_cubeVbo;
  std::unique_ptr<IndexBuffer> m_cubeIbo;
};

} // namespace Aether
