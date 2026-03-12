#include "Rendering/Renderer.h"

#include "Core/Window.h"
#include "Rendering/Framebuffer.h"
#include "Rendering/IndexBuffer.h"
#include "Rendering/Shader.h"
#include "Rendering/VertexArray.h"
#include "Rendering/VertexBuffer.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace Aether {
namespace {

static const char* kLitVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;

uniform mat4 u_ViewProj;
uniform mat4 u_Model;

out vec3 v_WorldPos;
out vec3 v_WorldNormal;

void main() {
  vec4 worldPos = u_Model * vec4(a_Position, 1.0);
  v_WorldPos = worldPos.xyz;
  v_WorldNormal = mat3(transpose(inverse(u_Model))) * a_Normal;
  gl_Position = u_ViewProj * worldPos;
}
)";

static const char* kLitFragmentShader = R"(
#version 330 core

in vec3 v_WorldPos;
in vec3 v_WorldNormal;

out vec4 o_Color;

struct Material {
  vec3 albedo;
  vec3 specularColor;
  float specularStrength;
  float shininess;
};

struct DirectionalLight {
  vec3 direction; // direction rays travel
  vec3 color;
  float intensity;
};

struct PointLight {
  vec3 position;
  vec3 color;
  float intensity;
  float radius;
};

uniform Material u_Material;

uniform vec3 u_CameraPos;

uniform vec3 u_AmbientColor;
uniform float u_AmbientIntensity;

uniform DirectionalLight u_DirectionalLight;

uniform int u_PointLightCount;
uniform PointLight u_PointLights[8];

uniform sampler2D u_ShadowMap;
uniform mat4 u_LightSpaceMatrix;

vec3 ApplyLight(vec3 N, vec3 V, vec3 L, vec3 lightColor, float intensity) {
  float NdotL = max(dot(N, L), 0.0);
  vec3 diffuse = u_Material.albedo * lightColor * (intensity * NdotL);

  vec3 specular = vec3(0.0);
  if (u_Material.specularStrength > 0.0 && NdotL > 0.0) {
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), max(u_Material.shininess, 1.0));
    specular = u_Material.specularColor * lightColor * (intensity * (spec * u_Material.specularStrength));
  }

  return diffuse + specular;
}

float ShadowVisibility(vec3 worldPos, vec3 N, vec3 L) {
  vec4 lightSpacePos = u_LightSpaceMatrix * vec4(worldPos, 1.0);
  vec3 projCoords = lightSpacePos.xyz / max(lightSpacePos.w, 0.0001);
  projCoords = projCoords * 0.5 + 0.5;

  // Outside the shadow map or behind the far plane.
  if (projCoords.z > 1.0) return 1.0;
  if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 1.0;

  float bias = max(0.0015 * (1.0 - dot(N, L)), 0.0005);

  vec2 texelSize = 1.0 / vec2(textureSize(u_ShadowMap, 0));
  float shadow = 0.0;
  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
      shadow += (projCoords.z - bias > pcfDepth) ? 1.0 : 0.0;
    }
  }
  shadow /= 9.0;
  return 1.0 - shadow;
}

void main() {
  vec3 N = normalize(v_WorldNormal);
  vec3 V = normalize(u_CameraPos - v_WorldPos);

  vec3 color = u_AmbientColor * u_AmbientIntensity * u_Material.albedo;

  // Directional light.
  vec3 Ld = normalize(-u_DirectionalLight.direction);
  float vis = ShadowVisibility(v_WorldPos, N, Ld);
  color += ApplyLight(N, V, Ld, u_DirectionalLight.color, u_DirectionalLight.intensity) * vis;

  // Point lights.
  for (int i = 0; i < u_PointLightCount; ++i) {
    vec3 toLight = u_PointLights[i].position - v_WorldPos;
    float dist = length(toLight);
    dist = max(dist, 0.0001);
    vec3 L = toLight / dist;

    float radius = max(u_PointLights[i].radius, 0.0001);
    float atten = clamp(1.0 - (dist / radius), 0.0, 1.0);
    atten *= atten;

    color += ApplyLight(N, V, L, u_PointLights[i].color, u_PointLights[i].intensity * atten);
  }

  o_Color = vec4(color, 1.0);
}
)";

static const char* kUnlitVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProj;
uniform mat4 u_Model;

void main() {
  gl_Position = u_ViewProj * u_Model * vec4(a_Position, 1.0);
}
)";

static const char* kUnlitFragmentShader = R"(
#version 330 core

uniform vec4 u_Color;

out vec4 o_Color;

void main() {
  o_Color = u_Color;
}
)";

static const char* kDepthVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 a_Position;

uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_Model;

void main() {
  gl_Position = u_LightSpaceMatrix * u_Model * vec4(a_Position, 1.0);
}
)";

static const char* kDepthFragmentShader = R"(
#version 330 core
void main() { }
)";

static glm::vec3 SafeNormalizeOrDefault(const glm::vec3& v, const glm::vec3& fallback) {
  const float len2 = glm::dot(v, v);
  if (len2 <= 1e-8f) {
    return fallback;
  }
  return v / std::sqrt(len2);
}

} // namespace

Renderer::~Renderer() {
  Shutdown();
}

bool Renderer::Init(Window& window) {
  m_window = &window;

  const int loaded = gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress));
  if (loaded == 0) {
    std::cerr << "[Aether] gladLoadGL failed\n";
    return false;
  }

  const char* versionStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
  const char* rendererStr = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  std::cout << "[Aether] OpenGL initialized: " << (versionStr ? versionStr : "(unknown)") << "\n";
  std::cout << "[Aether] GPU: " << (rendererStr ? rendererStr : "(unknown)") << "\n";

  m_litShader = std::make_unique<Shader>(kLitVertexShader, kLitFragmentShader);
  if (!m_litShader->IsValid()) {
    std::cerr << "[Aether] Lit shader creation failed\n";
    return false;
  }

  m_unlitShader = std::make_unique<Shader>(kUnlitVertexShader, kUnlitFragmentShader);
  if (!m_unlitShader->IsValid()) {
    std::cerr << "[Aether] Unlit shader creation failed\n";
    return false;
  }

  m_depthShader = std::make_unique<Shader>(kDepthVertexShader, kDepthFragmentShader);
  if (!m_depthShader->IsValid()) {
    std::cerr << "[Aether] Depth shader creation failed\n";
    return false;
  }

  if (!m_shadowMap.Init(2048)) {
    std::cerr << "[Aether] ShadowMap init failed\n";
    return false;
  }

  // Cube vertex data (positions + normals). 24 vertices (4 per face) to keep normals correct.
  constexpr float s = 0.5f;
  const float vertices[] = {
    // Front (+Z)
    -s, -s,  s,  0.0f,  0.0f,  1.0f,
     s, -s,  s,  0.0f,  0.0f,  1.0f,
     s,  s,  s,  0.0f,  0.0f,  1.0f,
    -s,  s,  s,  0.0f,  0.0f,  1.0f,
    // Back (-Z)
     s, -s, -s,  0.0f,  0.0f, -1.0f,
    -s, -s, -s,  0.0f,  0.0f, -1.0f,
    -s,  s, -s,  0.0f,  0.0f, -1.0f,
     s,  s, -s,  0.0f,  0.0f, -1.0f,
    // Left (-X)
    -s, -s, -s, -1.0f,  0.0f,  0.0f,
    -s, -s,  s, -1.0f,  0.0f,  0.0f,
    -s,  s,  s, -1.0f,  0.0f,  0.0f,
    -s,  s, -s, -1.0f,  0.0f,  0.0f,
    // Right (+X)
     s, -s,  s,  1.0f,  0.0f,  0.0f,
     s, -s, -s,  1.0f,  0.0f,  0.0f,
     s,  s, -s,  1.0f,  0.0f,  0.0f,
     s,  s,  s,  1.0f,  0.0f,  0.0f,
    // Top (+Y)
    -s,  s,  s,  0.0f,  1.0f,  0.0f,
     s,  s,  s,  0.0f,  1.0f,  0.0f,
     s,  s, -s,  0.0f,  1.0f,  0.0f,
    -s,  s, -s,  0.0f,  1.0f,  0.0f,
    // Bottom (-Y)
    -s, -s, -s,  0.0f, -1.0f,  0.0f,
     s, -s, -s,  0.0f, -1.0f,  0.0f,
     s, -s,  s,  0.0f, -1.0f,  0.0f,
    -s, -s,  s,  0.0f, -1.0f,  0.0f,
  };

  const uint32_t indices[] = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4,
    8, 9, 10, 10, 11, 8,
    12, 13, 14, 14, 15, 12,
    16, 17, 18, 18, 19, 16,
    20, 21, 22, 22, 23, 20,
  };

  m_cubeVao = std::make_unique<VertexArray>();
  m_cubeVao->Bind();

  m_cubeVbo = std::make_unique<VertexBuffer>(vertices, static_cast<uint32_t>(sizeof(vertices)));
  m_cubeIbo = std::make_unique<IndexBuffer>(indices, static_cast<uint32_t>(sizeof(indices) / sizeof(indices[0])));

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

  VertexArray::Unbind();

  std::cout << "[Aether] Renderer initialized\n";
  return true;
}

void Renderer::Shutdown() {
  m_cubeIbo.reset();
  m_cubeVbo.reset();
  m_cubeVao.reset();
  m_shadowMap.Shutdown();
  m_depthShader.reset();
  m_unlitShader.reset();
  m_litShader.reset();
  m_window = nullptr;
}

void Renderer::BeginFrame(Framebuffer* targetOrNull) {
  if (targetOrNull) {
    targetOrNull->Bind();
    const glm::ivec2 size = targetOrNull->GetSize();
    glViewport(0, 0, size.x, size.y);
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    const glm::ivec2 size = m_window ? m_window->GetSize() : glm::ivec2{0, 0};
    glViewport(0, 0, size.x, size.y);
  }
}

void Renderer::EndFrame() {
  // No-op for now (kept for symmetry and later phases).
}

void Renderer::Clear(const glm::vec4& color) {
  glEnable(GL_DEPTH_TEST);
  glClearColor(color.r, color.g, color.b, color.a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetFrameData(const RendererFrameData& frame) {
  m_frame = frame;
  m_frame.pointLightCount = std::clamp(m_frame.pointLightCount, 0, kMaxPointLights);

  // Compute light-space matrix from the directional light (simple fixed orthographic volume).
  const glm::vec3 dir =
    SafeNormalizeOrDefault(m_frame.directionalLight.direction, glm::vec3(-0.35f, -1.0f, -0.20f));
  const float orthoSize = 15.0f;
  const float nearPlane = 0.1f;
  const float farPlane = 50.0f;

  const glm::vec3 target(0.0f, 0.0f, 0.0f);
  const glm::vec3 lightPos = -dir * 20.0f;
  glm::vec3 up(0.0f, 1.0f, 0.0f);
  if (std::abs(glm::dot(up, glm::normalize(-dir))) > 0.99f) {
    up = glm::vec3(0.0f, 0.0f, 1.0f);
  }

  const glm::mat4 lightView = glm::lookAt(lightPos, target, up);
  const glm::mat4 lightProj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
  m_frame.lightSpaceMatrix = lightProj * lightView;
}

void Renderer::BeginShadowPass() {
  if (!m_depthShader) {
    return;
  }

  m_shadowMap.BindForWrite();
  glEnable(GL_DEPTH_TEST);
  glClear(GL_DEPTH_BUFFER_BIT);

  m_depthShader->Bind();
  m_depthShader->SetMat4("u_LightSpaceMatrix", m_frame.lightSpaceMatrix);
}

void Renderer::EndShadowPass() {
  Shader::Unbind();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawCubeShadow(const glm::mat4& model) {
  if (!m_depthShader || !m_cubeVao || !m_cubeIbo) {
    return;
  }

  m_depthShader->SetMat4("u_Model", model);

  m_cubeVao->Bind();
  m_cubeIbo->Bind();
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_cubeIbo->GetCount()), GL_UNSIGNED_INT, nullptr);

  VertexArray::Unbind();
}

void Renderer::DrawCube(const glm::mat4& model, const Material& material) {
  if (!m_litShader || !m_cubeVao || !m_cubeIbo) {
    return;
  }

  const int pointCount = std::clamp(m_frame.pointLightCount, 0, kMaxPointLights);

  m_litShader->Bind();
  m_litShader->SetMat4("u_ViewProj", m_frame.viewProj);
  m_litShader->SetMat4("u_Model", model);

  m_litShader->SetVec3("u_CameraPos", m_frame.cameraPos);
  m_litShader->SetVec3("u_AmbientColor", m_frame.ambientColor);
  m_litShader->SetFloat("u_AmbientIntensity", m_frame.ambientIntensity);

  const glm::vec3 dir =
    SafeNormalizeOrDefault(m_frame.directionalLight.direction, glm::vec3(-0.3f, -1.0f, -0.2f));
  m_litShader->SetVec3("u_DirectionalLight.direction", dir);
  m_litShader->SetVec3("u_DirectionalLight.color", m_frame.directionalLight.color);
  m_litShader->SetFloat("u_DirectionalLight.intensity", m_frame.directionalLight.intensity);

  m_litShader->SetMat4("u_LightSpaceMatrix", m_frame.lightSpaceMatrix);
  // Bind shadow map depth texture for sampling.
  constexpr int kShadowTexUnit = 5;
  m_shadowMap.BindForRead(static_cast<unsigned int>(GL_TEXTURE0 + kShadowTexUnit));
  m_litShader->SetInt("u_ShadowMap", kShadowTexUnit);

  m_litShader->SetInt("u_PointLightCount", pointCount);
  for (int i = 0; i < pointCount; ++i) {
    const PointLight& pl = m_frame.pointLights[static_cast<size_t>(i)];
    const std::string idx = std::to_string(i);

    m_litShader->SetVec3(("u_PointLights[" + idx + "].position").c_str(), pl.position);
    m_litShader->SetVec3(("u_PointLights[" + idx + "].color").c_str(), pl.color);
    m_litShader->SetFloat(("u_PointLights[" + idx + "].intensity").c_str(), pl.intensity);
    m_litShader->SetFloat(("u_PointLights[" + idx + "].radius").c_str(), pl.radius);
  }

  m_litShader->SetVec3("u_Material.albedo", material.albedo);
  m_litShader->SetVec3("u_Material.specularColor", material.specularColor);
  m_litShader->SetFloat("u_Material.specularStrength", material.specularStrength);
  m_litShader->SetFloat("u_Material.shininess", material.shininess);

  m_cubeVao->Bind();
  m_cubeIbo->Bind();
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_cubeIbo->GetCount()), GL_UNSIGNED_INT, nullptr);

  VertexArray::Unbind();
  Shader::Unbind();
}

void Renderer::DrawCubeWireframe(const glm::mat4& model, const glm::vec4& color) {
  if (!m_unlitShader || !m_cubeVao || !m_cubeIbo) {
    return;
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glLineWidth(2.0f);

  m_unlitShader->Bind();
  m_unlitShader->SetMat4("u_ViewProj", m_frame.viewProj);
  m_unlitShader->SetMat4("u_Model", model);
  m_unlitShader->SetVec4("u_Color", color);

  m_cubeVao->Bind();
  m_cubeIbo->Bind();
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_cubeIbo->GetCount()), GL_UNSIGNED_INT, nullptr);

  VertexArray::Unbind();
  Shader::Unbind();

  glLineWidth(1.0f);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

} // namespace Aether
