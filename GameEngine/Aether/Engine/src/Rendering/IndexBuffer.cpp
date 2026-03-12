#include "Rendering/IndexBuffer.h"

#include <glad/gl.h>

namespace Aether {

IndexBuffer::IndexBuffer(const uint32_t* indices, uint32_t count) : m_count(count) {
  glGenBuffers(1, &m_id);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    static_cast<GLsizeiptr>(sizeof(uint32_t) * static_cast<size_t>(count)),
    indices,
    GL_STATIC_DRAW
  );
}

IndexBuffer::~IndexBuffer() {
  if (m_id != 0) {
    glDeleteBuffers(1, &m_id);
    m_id = 0;
  }
  m_count = 0;
}

IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept {
  m_id = other.m_id;
  m_count = other.m_count;
  other.m_id = 0;
  other.m_count = 0;
}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  if (m_id != 0) {
    glDeleteBuffers(1, &m_id);
  }
  m_id = other.m_id;
  m_count = other.m_count;
  other.m_id = 0;
  other.m_count = 0;
  return *this;
}

void IndexBuffer::Bind() const {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
}

void IndexBuffer::Unbind() {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

} // namespace Aether

