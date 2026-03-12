#include "Rendering/VertexArray.h"

#include <glad/gl.h>

namespace Aether {

VertexArray::VertexArray() {
  glGenVertexArrays(1, &m_id);
}

VertexArray::~VertexArray() {
  if (m_id != 0) {
    glDeleteVertexArrays(1, &m_id);
    m_id = 0;
  }
}

VertexArray::VertexArray(VertexArray&& other) noexcept {
  m_id = other.m_id;
  other.m_id = 0;
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  if (m_id != 0) {
    glDeleteVertexArrays(1, &m_id);
  }
  m_id = other.m_id;
  other.m_id = 0;
  return *this;
}

void VertexArray::Bind() const {
  glBindVertexArray(m_id);
}

void VertexArray::Unbind() {
  glBindVertexArray(0);
}

} // namespace Aether

