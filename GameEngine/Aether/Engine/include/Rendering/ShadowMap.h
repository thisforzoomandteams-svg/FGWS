#pragma once

namespace Aether {

class ShadowMap {
public:
  ShadowMap() = default;
  ~ShadowMap();

  ShadowMap(const ShadowMap&) = delete;
  ShadowMap& operator=(const ShadowMap&) = delete;

  bool Init(int resolution = 2048);
  void Shutdown();

  void BindForWrite() const;
  void BindForRead(unsigned int textureUnit) const;

  unsigned int GetDepthTexture() const { return m_depthTex; }
  int GetResolution() const { return m_resolution; }

  bool Resize(int resolution);

private:
  unsigned int m_fbo = 0;
  unsigned int m_depthTex = 0;
  int m_resolution = 0;
};

} // namespace Aether

