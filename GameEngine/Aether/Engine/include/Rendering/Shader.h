#pragma once

#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>
#include <unordered_map>

namespace Aether {

class Shader {
public:
  Shader() = default;
  Shader(const char* vertexSource, const char* fragmentSource);
  ~Shader();

  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;

  Shader(Shader&& other) noexcept;
  Shader& operator=(Shader&& other) noexcept;

  bool IsValid() const { return m_program != 0; }

  void Bind() const;
  static void Unbind();

  void SetMat4(const char* name, const glm::mat4& value) const;
  void SetVec3(const char* name, const glm::vec3& value) const;
  void SetVec4(const char* name, const glm::vec4& value) const;
  void SetInt(const char* name, int value) const;
  void SetFloat(const char* name, float value) const;

  uint32_t GetProgramID() const { return m_program; }

private:
  int GetUniformLocation(const char* name) const;

  uint32_t m_program = 0;
  mutable std::unordered_map<std::string, int> m_uniformCache;
};

} // namespace Aether
