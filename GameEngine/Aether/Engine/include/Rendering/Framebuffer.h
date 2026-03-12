#pragma once

#include <cstdint>

#include <glm/vec2.hpp>

namespace Aether {

struct FramebufferSpec {
  int width = 1;
  int height = 1;
};

class Framebuffer {
public:
  explicit Framebuffer(const FramebufferSpec& spec);
  ~Framebuffer();

  Framebuffer(const Framebuffer&) = delete;
  Framebuffer& operator=(const Framebuffer&) = delete;

  Framebuffer(Framebuffer&& other) noexcept;
  Framebuffer& operator=(Framebuffer&& other) noexcept;

  void Bind() const;
  static void Unbind();

  void Resize(int width, int height);

  uint32_t GetColorAttachmentID() const { return m_colorAttachment; }
  glm::ivec2 GetSize() const { return {m_spec.width, m_spec.height}; }

private:
  void Invalidate();
  void Release();

  FramebufferSpec m_spec;
  uint32_t m_rendererID = 0;
  uint32_t m_colorAttachment = 0;
  uint32_t m_depthAttachment = 0;
};

} // namespace Aether

