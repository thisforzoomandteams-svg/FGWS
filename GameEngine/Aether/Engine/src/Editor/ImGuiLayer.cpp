#include "Editor/ImGuiLayer.h"

#include <imgui.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <iostream>

namespace Aether {

ImGuiLayer::~ImGuiLayer() {
  Shutdown();
}

bool ImGuiLayer::Init(GLFWwindow* window) {
  if (m_initialized) {
    return true;
  }

  if (!window) {
    std::cerr << "[Aether] ImGuiLayer::Init called with null window\n";
    return false;
  }

  m_window = window;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable; // keep single-viewport

  ImGui::StyleColorsDark();

  if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
    std::cerr << "[Aether] ImGui_ImplGlfw_InitForOpenGL failed\n";
    return false;
  }

  if (!ImGui_ImplOpenGL3_Init("#version 330")) {
    std::cerr << "[Aether] ImGui_ImplOpenGL3_Init failed\n";
    return false;
  }

  m_initialized = true;
  std::cout << "[Aether] ImGui initialized\n";
  return true;
}

void ImGuiLayer::Shutdown() {
  if (!m_initialized) {
    return;
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  m_initialized = false;
  m_window = nullptr;

  std::cout << "[Aether] ImGui shutdown\n";
}

void ImGuiLayer::Begin() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::End() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace Aether
