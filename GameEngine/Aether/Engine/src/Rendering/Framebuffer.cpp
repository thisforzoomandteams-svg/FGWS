#include "Rendering/Framebuffer.h"

#include <glad/gl.h>

#include <iostream>

namespace Aether {

Framebuffer::Framebuffer(const FramebufferSpec& spec) : m_spec(spec) {
  if (m_spec.width < 1) m_spec.width = 1;
  if (m_spec.height < 1) m_spec.height = 1;
  Invalidate();
}

Framebuffer::~Framebuffer() {
  Release();
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept {
  m_spec = other.m_spec;
  m_rendererID = other.m_rendererID;
  m_colorAttachment = other.m_colorAttachment;
  m_depthAttachment = other.m_depthAttachment;

  other.m_rendererID = 0;
  other.m_colorAttachment = 0;
  other.m_depthAttachment = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  Release();

  m_spec = other.m_spec;
  m_rendererID = other.m_rendererID;
  m_colorAttachment = other.m_colorAttachment;
  m_depthAttachment = other.m_depthAttachment;

  other.m_rendererID = 0;
  other.m_colorAttachment = 0;
  other.m_depthAttachment = 0;
  return *this;
}

void Framebuffer::Bind() const {
  glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);
}

void Framebuffer::Unbind() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Resize(int width, int height) {
  if (width < 1 || height < 1) {
    return;
  }
  if (m_spec.width == width && m_spec.height == height) {
    return;
  }

  m_spec.width = width;
  m_spec.height = height;
  Invalidate();
}

void Framebuffer::Invalidate() {
  Release();

  glGenFramebuffers(1, &m_rendererID);
  glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);

  glGenTextures(1, &m_colorAttachment);
  glBindTexture(GL_TEXTURE_2D, m_colorAttachment);
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA8,
    m_spec.width,
    m_spec.height,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    nullptr
  );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorAttachment, 0);

  glGenRenderbuffers(1, &m_depthAttachment);
  glBindRenderbuffer(GL_RENDERBUFFER, m_depthAttachment);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_spec.width, m_spec.height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthAttachment);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "[Aether] Framebuffer incomplete\n";
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Release() {
  if (m_depthAttachment != 0) {
    glDeleteRenderbuffers(1, &m_depthAttachment);
    m_depthAttachment = 0;
  }
  if (m_colorAttachment != 0) {
    glDeleteTextures(1, &m_colorAttachment);
    m_colorAttachment = 0;
  }
  if (m_rendererID != 0) {
    glDeleteFramebuffers(1, &m_rendererID);
    m_rendererID = 0;
  }
}

} // namespace Aether

