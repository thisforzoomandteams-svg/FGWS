#include "Rendering/ShadowMap.h"

#include <glad/gl.h>

#include <iostream>

namespace Aether {

ShadowMap::~ShadowMap() {
  Shutdown();
}

bool ShadowMap::Init(int resolution) {
  Shutdown();

  m_resolution = (resolution > 0) ? resolution : 2048;

  glGenFramebuffers(1, &m_fbo);
  glGenTextures(1, &m_depthTex);

  glBindTexture(GL_TEXTURE_2D, m_depthTex);
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_DEPTH_COMPONENT24,
    m_resolution,
    m_resolution,
    0,
    GL_DEPTH_COMPONENT,
    GL_FLOAT,
    nullptr
  );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  const float borderColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTex, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "[Aether] ShadowMap FBO incomplete: 0x" << std::hex << status << std::dec << "\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Shutdown();
    return false;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  return true;
}

void ShadowMap::Shutdown() {
  if (m_depthTex) {
    glDeleteTextures(1, &m_depthTex);
    m_depthTex = 0;
  }
  if (m_fbo) {
    glDeleteFramebuffers(1, &m_fbo);
    m_fbo = 0;
  }
  m_resolution = 0;
}

void ShadowMap::BindForWrite() const {
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glViewport(0, 0, m_resolution, m_resolution);
}

void ShadowMap::BindForRead(unsigned int textureUnit) const {
  glActiveTexture(textureUnit);
  glBindTexture(GL_TEXTURE_2D, m_depthTex);
}

bool ShadowMap::Resize(int resolution) {
  if (resolution <= 0) {
    return false;
  }
  if (resolution == m_resolution && m_fbo != 0 && m_depthTex != 0) {
    return true;
  }
  return Init(resolution);
}

} // namespace Aether

