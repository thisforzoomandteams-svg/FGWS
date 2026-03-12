#pragma once

#include <memory>

namespace Aether {

class Window;

class Application {
public:
  Application(int argc, char** argv);
  virtual ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  int Run();

protected:
  Window& GetWindow();
  const Window& GetWindow() const;

  void RequestExit(int exitCode);

  virtual void OnUpdate(float deltaSeconds) {}
  virtual void OnRender() {}
  virtual void OnImGuiRender() {}

private:
  struct Options {
    float autoExitSeconds = 0.0f;
    bool windowed = false;
  };

  bool m_ok = true;
  bool m_glfwInitialized = false;
  int m_exitCode = 0;
  Options m_options;
  std::unique_ptr<Window> m_window;
};

} // namespace Aether
