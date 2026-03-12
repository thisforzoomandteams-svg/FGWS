#pragma once

#include <cstdint>

namespace Aether {

class VertexBuffer {
public:
  VertexBuffer() = default;
  VertexBuffer(const void* data, uint32_t sizeBytes);
  ~VertexBuffer();

  VertexBuffer(const VertexBuffer&) = delete;
  VertexBuffer& operator=(const VertexBuffer&) = delete;

  VertexBuffer(VertexBuffer&& other) noexcept;
  VertexBuffer& operator=(VertexBuffer&& other) noexcept;

  void Bind() const;
  static void Unbind();

  uint32_t GetRendererID() const { return m_id; }

private:
  uint32_t m_id = 0;
};

} // namespace Aether
