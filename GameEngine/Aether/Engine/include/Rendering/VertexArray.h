#pragma once

#include <cstdint>

namespace Aether {

class VertexArray {
public:
  VertexArray();
  ~VertexArray();

  VertexArray(const VertexArray&) = delete;
  VertexArray& operator=(const VertexArray&) = delete;

  VertexArray(VertexArray&& other) noexcept;
  VertexArray& operator=(VertexArray&& other) noexcept;

  void Bind() const;
  static void Unbind();

  uint32_t GetRendererID() const { return m_id; }

private:
  uint32_t m_id = 0;
};

} // namespace Aether
