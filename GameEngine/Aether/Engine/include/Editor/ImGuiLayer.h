#pragma once

struct GLFWwindow;

namespace Aether {

class ImGuiLayer {
public:
  ImGuiLayer() = default;
  ~ImGuiLayer();

  ImGuiLayer(const ImGuiLayer&) = delete;
  ImGuiLayer& operator=(const ImGuiLayer&) = delete;

  bool Init(GLFWwindow* window);
  void Shutdown();

  void Begin();
  void End();

private:
  GLFWwindow* m_window = nullptr;
  bool m_initialized = false;
};

} // namespace Aether

