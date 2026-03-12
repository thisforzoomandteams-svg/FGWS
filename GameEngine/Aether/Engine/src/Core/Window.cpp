#include "Core/Window.h"

#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>

#include <iostream>

namespace Aether {

static GLFWwindow* TryCreateFullscreenWindow(const WindowSpec& spec) {
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  if (!monitor) {
    std::cerr << "[Aether] No primary monitor; cannot create fullscreen window\n";
    return nullptr;
  }

  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  if (!mode) {
    std::cerr << "[Aether] glfwGetVideoMode failed; cannot create fullscreen window\n";
    return nullptr;
  }

  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
  return glfwCreateWindow(mode->width, mode->height, spec.title, monitor, nullptr);
}

Window::Window(const WindowSpec& spec) : m_vsync(spec.vsync) {
  // OpenGL context hints (needed for later phases; safe to set now).
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

  if (spec.fullscreen) {
    m_window = TryCreateFullscreenWindow(spec);
    if (!m_window) {
      std::cerr << "[Aether] Fullscreen creation failed; falling back to windowed\n";
    }
  }

  if (!m_window) {
    m_window = glfwCreateWindow(spec.width, spec.height, spec.title, nullptr, nullptr);
  }

  if (!m_window) {
    std::cerr << "[Aether] glfwCreateWindow failed\n";
    return;
  }

  m_fullscreen = spec.fullscreen;
  m_windowedW = spec.width;
  m_windowedH = spec.height;
  glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);

  glfwMakeContextCurrent(m_window);
  SetVSync(spec.vsync);
}

Window::~Window() {
  if (m_window) {
    glfwDestroyWindow(m_window);
    m_window = nullptr;
  }
}

Window::Window(Window&& other) noexcept {
  m_window = other.m_window;
  m_vsync = other.m_vsync;
  other.m_window = nullptr;
}

Window& Window::operator=(Window&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  if (m_window) {
    glfwDestroyWindow(m_window);
  }

  m_window = other.m_window;
  m_vsync = other.m_vsync;
  other.m_window = nullptr;
  return *this;
}

void Window::PollEvents() {
  glfwPollEvents();
}

void Window::SwapBuffers() {
  if (m_window) {
    glfwSwapBuffers(m_window);
  }
}

bool Window::ShouldClose() const {
  return !m_window || glfwWindowShouldClose(m_window);
}

glm::ivec2 Window::GetSize() const {
  int w = 0;
  int h = 0;
  if (m_window) {
    glfwGetFramebufferSize(m_window, &w, &h);
  }
  return {w, h};
}

void Window::SetVSync(bool enabled) {
  m_vsync = enabled;
  glfwSwapInterval(enabled ? 1 : 0);
}

void Window::SetAlwaysFullscreen(bool enabled) {
  if (!m_window) {
    return;
  }

  if (enabled == m_fullscreen) {
    return;
  }

  if (enabled) {
    glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
    glfwGetWindowSize(m_window, &m_windowedW, &m_windowedH);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) {
      std::cerr << "[Aether] SetAlwaysFullscreen: no primary monitor\n";
      return;
    }

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!mode) {
      std::cerr << "[Aether] SetAlwaysFullscreen: glfwGetVideoMode failed\n";
      return;
    }

    glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    m_fullscreen = true;
  } else {
    glfwSetWindowMonitor(m_window, nullptr, m_windowedX, m_windowedY, m_windowedW, m_windowedH, 0);
    m_fullscreen = false;
  }

  // Preserve vsync setting across monitor switches.
  SetVSync(m_vsync);
}

} // namespace Aether
