#pragma once

#include <cstdint>

#include <glm/vec2.hpp>

struct GLFWwindow;

namespace Aether {

struct WindowSpec {
  const char* title = "Aether Sandbox";
  int width = 1280;
  int height = 720;
  bool fullscreen = true;
  bool vsync = true;
};

class Window {
public:
  explicit Window(const WindowSpec& spec);
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  Window(Window&& other) noexcept;
  Window& operator=(Window&& other) noexcept;

  void PollEvents();
  void SwapBuffers();
  bool ShouldClose() const;
  glm::ivec2 GetSize() const;
  void SetVSync(bool enabled);
  void SetAlwaysFullscreen(bool enabled);
  bool IsFullscreen() const { return m_fullscreen; }

  GLFWwindow* GetNativeHandle() const { return m_window; }

private:
  GLFWwindow* m_window = nullptr;
  bool m_vsync = true;
  bool m_fullscreen = false;
  int m_windowedX = 100;
  int m_windowedY = 100;
  int m_windowedW = 1280;
  int m_windowedH = 720;
};

} // namespace Aether
