#pragma once

#include <cstdint>

namespace Aether {

class IndexBuffer {
public:
  IndexBuffer() = default;
  IndexBuffer(const uint32_t* indices, uint32_t count);
  ~IndexBuffer();

  IndexBuffer(const IndexBuffer&) = delete;
  IndexBuffer& operator=(const IndexBuffer&) = delete;

  IndexBuffer(IndexBuffer&& other) noexcept;
  IndexBuffer& operator=(IndexBuffer&& other) noexcept;

  void Bind() const;
  static void Unbind();

  uint32_t GetCount() const { return m_count; }
  uint32_t GetRendererID() const { return m_id; }

private:
  uint32_t m_id = 0;
  uint32_t m_count = 0;
};

} // namespace Aether
