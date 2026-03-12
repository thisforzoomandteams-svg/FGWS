#include "Rendering/Shader.h"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>

namespace Aether {

static uint32_t CompileShader(uint32_t type, const char* source) {
  const uint32_t shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  int compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (compiled == GL_FALSE) {
    int length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string infoLog(static_cast<size_t>(length), '\0');
    glGetShaderInfoLog(shader, length, &length, infoLog.data());
    std::cerr << "[Aether] Shader compile failed: " << infoLog << "\n";
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

Shader::Shader(const char* vertexSource, const char* fragmentSource) {
  const uint32_t vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
  if (vs == 0) {
    return;
  }
  const uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
  if (fs == 0) {
    glDeleteShader(vs);
    return;
  }

  m_program = glCreateProgram();
  glAttachShader(m_program, vs);
  glAttachShader(m_program, fs);
  glLinkProgram(m_program);

  glDeleteShader(vs);
  glDeleteShader(fs);

  int linked = 0;
  glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
  if (linked == GL_FALSE) {
    int length = 0;
    glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &length);
    std::string infoLog(static_cast<size_t>(length), '\0');
    glGetProgramInfoLog(m_program, length, &length, infoLog.data());
    std::cerr << "[Aether] Program link failed: " << infoLog << "\n";
    glDeleteProgram(m_program);
    m_program = 0;
  }
}

Shader::~Shader() {
  if (m_program != 0) {
    glDeleteProgram(m_program);
    m_program = 0;
  }
}

Shader::Shader(Shader&& other) noexcept {
  m_program = other.m_program;
  m_uniformCache = std::move(other.m_uniformCache);
  other.m_program = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  if (m_program != 0) {
    glDeleteProgram(m_program);
  }
  m_program = other.m_program;
  m_uniformCache = std::move(other.m_uniformCache);
  other.m_program = 0;
  return *this;
}

void Shader::Bind() const {
  glUseProgram(m_program);
}

void Shader::Unbind() {
  glUseProgram(0);
}

int Shader::GetUniformLocation(const char* name) const {
  auto it = m_uniformCache.find(name);
  if (it != m_uniformCache.end()) {
    return it->second;
  }

  const int location = glGetUniformLocation(m_program, name);
  m_uniformCache.emplace(name, location);
  if (location == -1) {
    std::cerr << "[Aether] Uniform not found: " << name << "\n";
  }
  return location;
}

void Shader::SetMat4(const char* name, const glm::mat4& value) const {
  const int location = GetUniformLocation(name);
  if (location != -1) {
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
  }
}

void Shader::SetVec3(const char* name, const glm::vec3& value) const {
  const int location = GetUniformLocation(name);
  if (location != -1) {
    glUniform3fv(location, 1, glm::value_ptr(value));
  }
}

void Shader::SetVec4(const char* name, const glm::vec4& value) const {
  const int location = GetUniformLocation(name);
  if (location != -1) {
    glUniform4fv(location, 1, glm::value_ptr(value));
  }
}

void Shader::SetInt(const char* name, int value) const {
  const int location = GetUniformLocation(name);
  if (location != -1) {
    glUniform1i(location, value);
  }
}

void Shader::SetFloat(const char* name, float value) const {
  const int location = GetUniformLocation(name);
  if (location != -1) {
    glUniform1f(location, value);
  }
}

} // namespace Aether
