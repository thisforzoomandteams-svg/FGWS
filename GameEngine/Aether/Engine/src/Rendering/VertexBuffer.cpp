#include "Rendering/VertexBuffer.h"

#include <glad/gl.h>

namespace Aether {

VertexBuffer::VertexBuffer(const void* data, uint32_t sizeBytes) {
  glGenBuffers(1, &m_id);
  glBindBuffer(GL_ARRAY_BUFFER, m_id);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeBytes), data, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer() {
  if (m_id != 0) {
    glDeleteBuffers(1, &m_id);
    m_id = 0;
  }
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept {
  m_id = other.m_id;
  other.m_id = 0;
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  if (m_id != 0) {
    glDeleteBuffers(1, &m_id);
  }
  m_id = other.m_id;
  other.m_id = 0;
  return *this;
}

void VertexBuffer::Bind() const {
  glBindBuffer(GL_ARRAY_BUFFER, m_id);
}

void VertexBuffer::Unbind() {
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace Aether

