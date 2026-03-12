#include "Core/Application.h"

#include "Core/Window.h"

#include <GLFW/glfw3.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>

namespace Aether {

static void GlfwErrorCallback(int error, const char* description) {
  std::cerr << "[GLFW] Error " << error << ": " << (description ? description : "(null)") << "\n";
}

static bool TryParseFloat(std::string_view text, float& outValue) {
  if (text.empty()) {
    return false;
  }

  std::string owned(text);
  char* endPtr = nullptr;
  const float value = std::strtof(owned.c_str(), &endPtr);
  if (endPtr == owned.c_str() || *endPtr != '\0') {
    return false;
  }
  outValue = value;
  return true;
}

Application::Application(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i] ? argv[i] : "");
    if (arg == "--windowed") {
      m_options.windowed = true;
      continue;
    }

    constexpr std::string_view kAutoExitPrefix = "--auto-exit=";
    if (arg.rfind(kAutoExitPrefix, 0) == 0) {
      const std::string_view valueText = arg.substr(kAutoExitPrefix.size());
      float seconds = 0.0f;
      if (!TryParseFloat(valueText, seconds) || seconds < 0.0f) {
        std::cerr << "[Aether] Invalid --auto-exit value: '" << valueText << "'\n";
        m_ok = false;
      } else {
        m_options.autoExitSeconds = seconds;
      }
      continue;
    }
  }

  std::cout << "[Aether] Application starting\n";

  glfwSetErrorCallback(GlfwErrorCallback);
  if (!glfwInit()) {
    std::cerr << "[Aether] glfwInit failed\n";
    m_ok = false;
    return;
  }
  m_glfwInitialized = true;
  std::cout << "[Aether] GLFW initialized\n";

  WindowSpec spec;
  spec.fullscreen = !m_options.windowed;
  m_window = std::make_unique<Window>(spec);
  if (m_window) {
    m_window->SetAlwaysFullscreen(!m_options.windowed);
  }
  std::cout << "[Aether] Window created\n";
}

Application::~Application() {
  m_window.reset();
  if (m_glfwInitialized) {
    if (glfwGetCurrentContext() != nullptr) {
      glfwMakeContextCurrent(nullptr);
    }
    glfwTerminate();
    std::cout << "[Aether] GLFW terminated\n";
  }
}

Window& Application::GetWindow() {
  return *m_window;
}

const Window& Application::GetWindow() const {
  return *m_window;
}

void Application::RequestExit(int exitCode) {
  m_exitCode = exitCode;
  if (m_window && m_window->GetNativeHandle()) {
    glfwSetWindowShouldClose(m_window->GetNativeHandle(), GLFW_TRUE);
  }
}

int Application::Run() {
  if (!m_ok) {
    std::cerr << "[Aether] Startup failed; exiting with code 1\n";
    return 1;
  }
  if (!m_window) {
    std::cerr << "[Aether] No window; exiting with code 1\n";
    return 1;
  }

  const double startTime = glfwGetTime();
  double lastTime = startTime;

  while (!m_window->ShouldClose()) {
    const double now = glfwGetTime();
    const float deltaSeconds = static_cast<float>(now - lastTime);
    lastTime = now;

    m_window->PollEvents();

    OnUpdate(deltaSeconds);
    OnRender();

    m_window->SwapBuffers();

    if (m_options.autoExitSeconds > 0.0f) {
      const double elapsed = now - startTime;
      if (elapsed >= static_cast<double>(m_options.autoExitSeconds)) {
        std::cout << "[Aether] Auto-exit triggered after " << elapsed << "s\n";
        RequestExit(0);
      }
    }
  }

  return m_exitCode;
}

} // namespace Aether
