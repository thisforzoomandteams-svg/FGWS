#include "Aether.h"

#include "Editor/ImGuiLayer.h"
#include "Editor/ImGuiLayout.h"
#include "Editor/ViewportPanel.h"

#include "ECS/Prefab.h"
#include "ECS/Scene.h"

#include "Rendering/Camera.h"
#include "Rendering/Framebuffer.h"
#include "Rendering/Renderer.h"

#include "Scripting/ScriptEngine.h"
#include "Scripting/ScriptRegistry.h"

#include "Serialization/SceneSerializer.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <imgui.h>

#include "ImGuizmo.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "Editor/EditorState.h"

namespace {

static int ImGuiStringResizeCallback(ImGuiInputTextCallbackData* data) {
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
    auto* str = static_cast<std::string*>(data->UserData);
    str->resize(static_cast<size_t>(data->BufTextLen));
    data->Buf = str->data();
  }
  return 0;
}

static bool InputTextMultilineString(
  const char* label,
  std::string& str,
  const ImVec2& size,
  ImGuiInputTextFlags flags = 0
) {
  flags |= ImGuiInputTextFlags_CallbackResize;
  if (str.capacity() == 0) {
    str.reserve(1024);
  }
  return ImGui::InputTextMultiline(
    label,
    str.data(),
    str.capacity() + 1,
    size,
    flags,
    ImGuiStringResizeCallback,
    &str
  );
}

static std::string ToLowerCopy(const std::string& s) {
  std::string out = s;
  std::transform(
    out.begin(),
    out.end(),
    out.begin(),
    [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
  );
  return out;
}

static std::string ToLowerCopy(const char* s) {
  if (!s) {
    return {};
  }
  return ToLowerCopy(std::string(s));
}

} // namespace

class SandboxApp final : public Aether::Application {
public:
  SandboxApp(int argc, char** argv) : Aether::Application(argc, argv) {
    SetupAssetPaths();
    m_scriptRegistry.AddSearchPath("Assets/Scripts");
    m_scriptRegistry.Refresh();

    if (!m_renderer.Init(GetWindow())) {
      RequestExit(1);
      return;
    }

    m_viewportFramebuffer = std::make_unique<Aether::Framebuffer>(Aether::FramebufferSpec{1, 1});
    std::cout << "[Sandbox] Framebuffer created\n";

    if (!m_imgui.Init(GetWindow().GetNativeHandle())) {
      RequestExit(1);
      return;
    }

    m_scriptEngine.Init(&m_scene);
    m_scriptEngine.SetLogger([this](const std::string& s) { Log(s); });

    CreateDemoScene();
  }

  ~SandboxApp() override {
    m_scriptEngine.Shutdown();
    m_imgui.Shutdown();
    m_viewportFramebuffer.reset();
    m_renderer.Shutdown();
  }

protected:
  void OnUpdate(float deltaSeconds) override {
    m_lastDeltaSeconds = deltaSeconds;
    m_timeSeconds += deltaSeconds;

    // Keep the list of available scripts up to date (used by the Inspector dropdown and Scripts window).
    (void)m_scriptRegistry.RefreshIfNeeded();

    if (Aether::Editor::g_ScriptsEnabled) {
      m_scriptEngine.Tick(deltaSeconds);
    }
  }

  void OnRender() override {
    m_imgui.Begin();
    ImGuizmo::BeginFrame();

    // Allow Sandbox to inject "File" menu entries into the engine dockspace menu bar.
    Aether::Editor::g_DockspaceMenuCallback = [this]() {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Save Scene")) {
          SaveScene(m_scenePath);
        }
        if (ImGui::MenuItem("Load Scene")) {
          LoadScene(m_scenePath);
        }
        if (ImGui::MenuItem("Instantiate Prefab...")) {
          m_openInstantiatePrefabPopup = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Reset Demo Scene")) {
          CreateDemoScene();
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Window")) {
        ImGui::MenuItem("Scripts", nullptr, &m_showScriptsWindow);
        ImGui::EndMenu();
      }
    };

    Aether::Editor::SetupDockspace(nullptr);
    Aether::Editor::g_DockspaceMenuCallback = nullptr;

    DrawHierarchy();
    DrawInspector();
    DrawConsole();
    DrawScriptsWindow();

    Aether::Editor::ViewportPanelOutput vp{};
    if (m_viewportFramebuffer) {
      vp = Aether::Editor::DrawViewportPanel(*m_viewportFramebuffer, Aether::Editor::g_ShowLogo);
      if (vp.size.x > 0 && vp.size.y > 0) {
        m_viewportSize = vp.size;
      }
      m_viewportHovered = vp.windowHovered;
    }

    HandleViewportGizmo(vp);
    HandleViewportRayPick(vp);
    HandleCameraInput(vp);

    DrawPrefabPopups();

    RenderScene3D();

    // UI pass to default framebuffer.
    m_renderer.BeginFrame(nullptr);
    m_renderer.Clear({0.05f, 0.05f, 0.05f, 1.0f});
    m_imgui.End();
    m_renderer.EndFrame();
  }

private:
  bool IsEntitySelected(uint32_t id) const {
    return std::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(), id) != m_selectedEntityIds.end();
  }

  void ClearSelection() {
    m_selectedEntityIds.clear();
    m_primarySelectionId = 0;
  }

  void SelectSingle(uint32_t id) {
    m_selectedEntityIds.clear();
    if (id != 0) {
      m_selectedEntityIds.push_back(id);
      m_primarySelectionId = id;
    } else {
      m_primarySelectionId = 0;
    }
  }

  void ToggleSelection(uint32_t id) {
    if (id == 0) {
      return;
    }

    auto it = std::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(), id);
    if (it == m_selectedEntityIds.end()) {
      m_selectedEntityIds.push_back(id);
      m_primarySelectionId = id;
    } else {
      m_selectedEntityIds.erase(it);
      if (m_primarySelectionId == id) {
        m_primarySelectionId = m_selectedEntityIds.empty() ? 0 : m_selectedEntityIds.back();
      }
    }
  }

  void SelectRange(int a, int b) {
    const auto& entities = m_scene.GetEntities();
    if (entities.empty()) {
      return;
    }

    a = std::clamp(a, 0, static_cast<int>(entities.size()) - 1);
    b = std::clamp(b, 0, static_cast<int>(entities.size()) - 1);
    if (a > b) {
      std::swap(a, b);
    }

    m_selectedEntityIds.clear();
    for (int i = a; i <= b; ++i) {
      m_selectedEntityIds.push_back(entities[static_cast<size_t>(i)].id);
    }
    m_primarySelectionId = entities[static_cast<size_t>(b)].id;
  }

  std::optional<uint32_t> GetPrimarySelection() const {
    if (m_primarySelectionId != 0 && IsEntitySelected(m_primarySelectionId)) {
      return m_primarySelectionId;
    }
    if (!m_selectedEntityIds.empty()) {
      return m_selectedEntityIds.back();
    }
    return std::nullopt;
  }

  void DrawHierarchy() {
    ImGui::Begin("Hierarchy");

    if (ImGui::Button("Create Cube")) {
      const std::string name = "Entity " + std::to_string(m_scene.GetEntities().size() + 1);
      Aether::Entity& e = m_scene.CreateEntity(name);
      e.transform.position = {0.0f, 0.0f, 0.0f};
      SelectSingle(e.id);
      Log("Created entity: " + e.name + " (id=" + std::to_string(e.id) + ")");
    }

    ImGui::SameLine();
    ImGui::TextDisabled("Selected: %zu", m_selectedEntityIds.size());

    ImGui::Separator();

    const ImGuiIO& io = ImGui::GetIO();
    const auto& entities = m_scene.GetEntities();

    for (size_t idx = 0; idx < entities.size(); ++idx) {
      const Aether::Entity& e = entities[idx];
      ImGui::PushID(static_cast<int>(e.id));
      const bool selected = IsEntitySelected(e.id);

      std::string label = e.name;
      if (m_scene.GetMaterial(e.id)) label += " [M]";
      if (m_scene.GetPointLight(e.id)) label += " [PL]";
      if (m_scene.GetScript(e.id)) label += " [S]";

      if (ImGui::Selectable(label.c_str(), selected)) {
        const bool ctrl = io.KeyCtrl;
        const bool shift = io.KeyShift;
        if (shift && m_lastClickedHierarchyIndex >= 0) {
          SelectRange(m_lastClickedHierarchyIndex, static_cast<int>(idx));
        } else if (ctrl) {
          ToggleSelection(e.id);
        } else {
          SelectSingle(e.id);
        }
        m_lastClickedHierarchyIndex = static_cast<int>(idx);

        if (IsEntitySelected(e.id)) {
          Log("Selected via Hierarchy: " + e.name + " (id=" + std::to_string(e.id) + ")");
        } else {
          Log("Deselected via Hierarchy: " + e.name + " (id=" + std::to_string(e.id) + ")");
        }
      }

      if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AETHER_SCRIPT_PATH")) {
          const char* scriptPath = static_cast<const char*>(payload->Data);
          if (scriptPath && scriptPath[0] != '\0') {
            AttachScriptToEntity(e.id, scriptPath);
          }
        }
        ImGui::EndDragDropTarget();
      }

      if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Save as Prefab...")) {
          OpenSavePrefabPopup(e.id);
        }
        ImGui::EndPopup();
      }
      ImGui::PopID();
    }

    ImGui::End();
  }

  void DrawInspector() {
    ImGui::Begin("Inspector");

    if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::InputText("Path", m_scenePath, sizeof(m_scenePath));
      if (ImGui::Button("Save")) {
        SaveScene(m_scenePath);
      }
      ImGui::SameLine();
      if (ImGui::Button("Load")) {
        LoadScene(m_scenePath);
      }
      ImGui::SameLine();
      if (ImGui::Button("Reset Demo")) {
        CreateDemoScene();
      }
    }

    if (ImGui::CollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
      glm::vec3& ambientColor = m_scene.GetAmbientColor();
      float& ambientIntensity = m_scene.GetAmbientIntensity();
      ImGui::ColorEdit3("Ambient Color", &ambientColor.x);
      ImGui::DragFloat("Ambient Intensity", &ambientIntensity, 0.05f, 0.0f, 10.0f);

      Aether::DirectionalLight& dl = m_scene.GetDirectionalLight();
      const bool dirChanged = ImGui::DragFloat3("Dir Direction", &dl.direction.x, 0.02f, -1.0f, 1.0f);
      if (dirChanged) {
        const float len2 = glm::dot(dl.direction, dl.direction);
        if (len2 > 1e-6f) {
          dl.direction = dl.direction / std::sqrt(len2);
        }
      }
      ImGui::ColorEdit3("Dir Color", &dl.color.x);
      ImGui::DragFloat("Dir Intensity", &dl.intensity, 0.05f, 0.0f, 50.0f);
    }

    ImGui::Separator();

    if (m_selectedEntityIds.empty()) {
      ImGui::TextUnformatted("No selection.");
      ImGui::End();
      return;
    }

    ImGui::TextDisabled("Selected: %zu", m_selectedEntityIds.size());

    const std::optional<uint32_t> primaryId = GetPrimarySelection();
    Aether::Entity* primary = primaryId ? m_scene.FindById(*primaryId) : nullptr;
    if (!primary) {
      ImGui::TextUnformatted("Selection invalid.");
      ImGui::End();
      return;
    }

    if (m_selectedEntityIds.size() > 1) {
      ImGui::Text("Primary: %s", primary->name.c_str());
      ImGui::Separator();
      ImGui::TextUnformatted("Multiple selection: transform/material/light editing is disabled.");
      ImGui::Separator();

      DrawScriptAttachDropdown(m_selectedEntityIds);
      ImGui::End();
      return;
    }

    Aether::Entity* selected = primary;

    ImGui::Text("Entity: %s", selected->name.c_str());
    ImGui::Separator();

    ImGui::DragFloat3("Position", &selected->transform.position.x, 0.05f);
    ImGui::DragFloat3("Rotation", &selected->transform.rotationDeg.x, 0.25f);
    ImGui::DragFloat3("Scale", &selected->transform.scale.x, 0.05f, 0.01f, 1000.0f);

    ImGui::Separator();
    if (Aether::MaterialComponent* mc = m_scene.GetMaterial(selected->id)) {
      if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit3("Albedo", &mc->material.albedo.x);
        ImGui::ColorEdit3("Specular Color", &mc->material.specularColor.x);
        ImGui::DragFloat("Specular Strength", &mc->material.specularStrength, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Shininess", &mc->material.shininess, 1.0f, 1.0f, 256.0f);
      }
    } else {
      if (ImGui::Button("Add Material")) {
        (void)m_scene.AddMaterial(selected->id);
      }
    }

    ImGui::Separator();
    if (Aether::PointLightComponent* pl = m_scene.GetPointLight(selected->id)) {
      if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted("(position = entity position)");
        ImGui::ColorEdit3("Light Color", &pl->color.x);
        ImGui::DragFloat("Intensity", &pl->intensity, 0.1f, 0.0f, 200.0f);
        ImGui::DragFloat("Radius", &pl->radius, 0.1f, 0.1f, 100.0f);
        if (ImGui::Button("Remove Point Light")) {
          m_scene.RemovePointLight(selected->id);
        }
      }
    } else {
      if (ImGui::Button("Add Point Light")) {
        (void)m_scene.AddPointLight(selected->id);
      }
    }

    ImGui::Separator();
    DrawScriptAttachDropdown(m_selectedEntityIds);

    ImGui::End();
  }

  void DrawScriptAttachDropdown(const std::vector<uint32_t>& entityIds) {
    if (entityIds.empty()) {
      return;
    }

    if (!ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen)) {
      return;
    }

    // Determine current state across selection.
    std::string commonPath;
    bool mixed = false;
    bool any = false;

    for (uint32_t id : entityIds) {
      const Aether::ScriptComponent* sc = m_scene.GetScript(id);
      const std::string path = (sc && !sc->ScriptPath.empty()) ? sc->ScriptPath : std::string();
      if (!any) {
        any = true;
        commonPath = path;
      } else if (path != commonPath) {
        mixed = true;
        break;
      }
    }

    ImGui::InputTextWithHint(
      "##script_search",
      "Search scripts...",
      m_scriptDropdownSearchBuf,
      sizeof(m_scriptDropdownSearchBuf)
    );

    const char* preview = mixed ? "<mixed>" : (commonPath.empty() ? "None" : commonPath.c_str());

    const auto& scripts = m_scriptRegistry.GetAvailableScripts(&m_scene);

    if (ImGui::BeginCombo("Attach Script", preview)) {
      const bool noneSelected = (!mixed && commonPath.empty());
      if (ImGui::Selectable("None", noneSelected)) {
        ApplyScriptToEntities(entityIds, std::string{});
      }

      const std::string searchLower = ToLowerCopy(m_scriptDropdownSearchBuf);

      for (const Aether::ScriptInfo& info : scripts) {
        if (!searchLower.empty()) {
          const std::string nameLower = ToLowerCopy(info.name);
          const std::string pathLower = ToLowerCopy(info.path);
          if (nameLower.find(searchLower) == std::string::npos && pathLower.find(searchLower) == std::string::npos) {
            continue;
          }
        }

        const bool isSelected = (!mixed && info.path == commonPath);
        if (ImGui::Selectable(info.path.c_str(), isSelected)) {
          ApplyScriptToEntities(entityIds, info.path);
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
          ImGui::SetDragDropPayload("AETHER_SCRIPT_PATH", info.path.c_str(), info.path.size() + 1);
          ImGui::TextUnformatted(info.path.c_str());
          ImGui::EndDragDropSource();
        }

        if (ImGui::IsItemHovered()) {
          if (!info.description.empty()) {
            ImGui::SetTooltip("%s\n\n%s", info.description.c_str(), info.path.c_str());
          } else {
            ImGui::SetTooltip("%s", info.path.c_str());
          }
        }
      }

      ImGui::EndCombo();
    }

    if (ImGui::Button("Reload")) {
      int reloaded = 0;
      for (uint32_t id : entityIds) {
        if (m_scene.GetScript(id)) {
          if (m_scriptEngine.ReloadEntityScript(id)) {
            ++reloaded;
          }
        }
      }
      Log(std::string("[Script] Reloaded scripts for ") + std::to_string(reloaded) + " entities");
    }
  }

  void DrawConsole() {
    ImGui::Begin("Console");
    if (ImGui::Button("Clear")) {
      m_consoleLines.clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_consoleAutoScroll);

    ImGui::Separator();
    for (const std::string& line : m_consoleLines) {
      ImGui::TextUnformatted(line.c_str());
    }
    if (m_consoleAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
      ImGui::SetScrollHereY(1.0f);
    }
    ImGui::End();
  }

  void HandleViewportGizmo(const Aether::Editor::ViewportPanelOutput& vp) {
    const std::optional<uint32_t> primaryId = GetPrimarySelection();
    if (!primaryId.has_value() || m_viewportSize.x <= 0 || m_viewportSize.y <= 0) {
      return;
    }

    if (GLFWwindow* window = GetWindow().GetNativeHandle()) {
      const bool rmbDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
      const bool lookActive = vp.windowHovered && rmbDown;
      if (vp.windowHovered && !lookActive) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) m_gizmoOperation = ImGuizmo::TRANSLATE;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) m_gizmoOperation = ImGuizmo::ROTATE;
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) m_gizmoOperation = ImGuizmo::SCALE;
      }
    }

    Aether::Entity* selected = m_scene.FindById(*primaryId);
    if (!selected) {
      return;
    }

    const float aspect = static_cast<float>(m_viewportSize.x) / static_cast<float>(m_viewportSize.y);
    const glm::mat4 view = m_camera.GetView();
    const glm::mat4 proj = m_camera.GetProjection(aspect);

    glm::mat4 model = selected->GetTransformMatrix();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
    ImGuizmo::SetRect(vp.imageMin.x, vp.imageMin.y, vp.imageSize.x, vp.imageSize.y);

    ImGuizmo::Manipulate(
      glm::value_ptr(view),
      glm::value_ptr(proj),
      m_gizmoOperation,
      ImGuizmo::LOCAL,
      glm::value_ptr(model)
    );

    if (ImGuizmo::IsUsing()) {
      float t[3]{};
      float r[3]{};
      float s[3]{};
      ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), t, r, s);
      selected->transform.position = {t[0], t[1], t[2]};
      selected->transform.rotationDeg = {r[0], r[1], r[2]};
      selected->transform.scale = {s[0], s[1], s[2]};
    }
  }

  void HandleViewportRayPick(const Aether::Editor::ViewportPanelOutput& vp) {
    if (!vp.imageHovered || m_viewportSize.x <= 0 || m_viewportSize.y <= 0) {
      return;
    }

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      return;
    }

    const ImVec2 mousePos = ImGui::GetMousePos();
    const float u = (mousePos.x - vp.imageMin.x) / vp.imageSize.x;
    const float v = (mousePos.y - vp.imageMin.y) / vp.imageSize.y;

    if (!(u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f)) {
      return;
    }

    const float ndcX = (u * 2.0f) - 1.0f;
    const float ndcY = 1.0f - (v * 2.0f);

    const float aspect = static_cast<float>(m_viewportSize.x) / static_cast<float>(m_viewportSize.y);
    const glm::mat4 viewProj = m_camera.GetProjection(aspect) * m_camera.GetView();
    const glm::mat4 invViewProj = glm::inverse(viewProj);

    const glm::vec4 rayStartNdc(ndcX, ndcY, -1.0f, 1.0f);
    const glm::vec4 rayEndNdc(ndcX, ndcY, 1.0f, 1.0f);
    glm::vec4 rayStartWorld = invViewProj * rayStartNdc;
    glm::vec4 rayEndWorld = invViewProj * rayEndNdc;
    rayStartWorld /= rayStartWorld.w;
    rayEndWorld /= rayEndWorld.w;

    const glm::vec3 rayOrigin = m_camera.GetPosition();
    const glm::vec3 rayDir = glm::normalize(glm::vec3(rayEndWorld - rayStartWorld));

    const std::optional<uint32_t> hit = m_scene.RaycastSingle(rayOrigin, rayDir);
    const bool ctrl = ImGui::GetIO().KeyCtrl;
    if (hit.has_value()) {
      const Aether::Entity* e = m_scene.FindById(*hit);
      if (ctrl) {
        const bool wasSelected = IsEntitySelected(*hit);
        ToggleSelection(*hit);
        Log(std::string(wasSelected ? "Deselected via Raycast: " : "Selected via Raycast: ") +
            (e ? e->name : "<unknown>") + " (id=" + std::to_string(*hit) + ")");
      } else {
        SelectSingle(*hit);
        Log(std::string("Selected via Raycast: ") + (e ? e->name : "<unknown>") + " (id=" + std::to_string(*hit) + ")");
      }
    } else {
      if (!ctrl) {
        ClearSelection();
        Log("Selection cleared (no hit)");
      }
    }
  }

  void HandleCameraInput(const Aether::Editor::ViewportPanelOutput& vp) {
    if (GLFWwindow* window = GetWindow().GetNativeHandle()) {
      const bool rmbDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
      const bool lookActive = vp.windowHovered && rmbDown;

      glfwSetInputMode(window, GLFW_CURSOR, lookActive ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

      double mouseX = 0.0;
      double mouseY = 0.0;
      glfwGetCursorPos(window, &mouseX, &mouseY);

      float deltaX = 0.0f;
      float deltaY = 0.0f;
      if (lookActive) {
        if (m_hadMouseLastFrame && m_lookActiveLastFrame) {
          deltaX = static_cast<float>(mouseX - m_lastMouseX);
          deltaY = static_cast<float>(mouseY - m_lastMouseY);
        }
      }

      m_lastMouseX = mouseX;
      m_lastMouseY = mouseY;
      m_hadMouseLastFrame = true;
      m_lookActiveLastFrame = lookActive;

      Aether::CameraInputState input;
      input.lookActive = lookActive;
      input.mouseDeltaX = deltaX;
      input.mouseDeltaY = deltaY;

      if (lookActive) {
        input.moveForward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        input.moveBackward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        input.moveLeft = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
        input.moveRight = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        input.moveUp = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        input.moveDown =
          (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) ||
          (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);
        input.fast =
          (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
          (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
      }

      m_camera.UpdateFromInput(m_lastDeltaSeconds, input);
    }
  }

  void RenderScene3D() {
    if (!m_viewportFramebuffer || m_viewportSize.x <= 0 || m_viewportSize.y <= 0) {
      return;
    }

    const float aspect = static_cast<float>(m_viewportSize.x) / static_cast<float>(m_viewportSize.y);
    const glm::mat4 viewProj = m_camera.GetProjection(aspect) * m_camera.GetView();

    // Frame-wide render data (lights, camera, matrices).
    Aether::RendererFrameData frame;
    frame.viewProj = viewProj;
    frame.cameraPos = m_camera.GetPosition();
    frame.ambientColor = m_scene.GetAmbientColor();
    frame.ambientIntensity = m_scene.GetAmbientIntensity();
    frame.directionalLight = m_scene.GetDirectionalLight();

    int lightCount = 0;
    for (const Aether::Entity& e : m_scene.GetEntities()) {
      if (const Aether::PointLightComponent* pl = m_scene.GetPointLight(e.id)) {
        if (lightCount >= Aether::Renderer::kMaxPointLights) {
          break;
        }
        Aether::PointLight& out = frame.pointLights[static_cast<size_t>(lightCount)];
        out.position = e.transform.position;
        out.color = pl->color;
        out.intensity = pl->intensity;
        out.radius = pl->radius;
        ++lightCount;
      }
    }
    frame.pointLightCount = lightCount;
    m_renderer.SetFrameData(frame);

    // Shadow map pass (directional light).
    m_renderer.BeginShadowPass();
    for (const Aether::Entity& e : m_scene.GetEntities()) {
      m_renderer.DrawCubeShadow(e.GetTransformMatrix());
    }
    m_renderer.EndShadowPass();

    // Main pass to viewport framebuffer.
    m_renderer.BeginFrame(m_viewportFramebuffer.get());
    m_renderer.Clear({0.08f, 0.09f, 0.10f, 1.0f});

    for (const Aether::Entity& e : m_scene.GetEntities()) {
      const Aether::MaterialComponent* mc = m_scene.GetMaterial(e.id);
      const Aether::Material mat = mc ? mc->material : Aether::Material{};
      m_renderer.DrawCube(e.GetTransformMatrix(), mat);
    }

    const std::optional<uint32_t> primaryId = GetPrimarySelection();
    for (uint32_t id : m_selectedEntityIds) {
      const Aether::Entity* selected = m_scene.FindById(id);
      if (!selected) {
        continue;
      }
      const glm::mat4 outlineModel =
        selected->GetTransformMatrix() * glm::scale(glm::mat4(1.0f), glm::vec3(1.01f));
      const glm::vec4 color = (primaryId.has_value() && *primaryId == id)
        ? glm::vec4(1.0f, 1.0f, 0.35f, 1.0f)
        : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
      m_renderer.DrawCubeWireframe(outlineModel, color);
    }

    m_renderer.EndFrame();
  }

  void DrawPrefabPopups() {
    if (m_openSavePrefabPopup) {
      ImGui::OpenPopup("Save Prefab");
      m_openSavePrefabPopup = false;
    }

    if (ImGui::BeginPopupModal("Save Prefab", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::InputText("Path", m_prefabPath, sizeof(m_prefabPath));
      if (ImGui::Button("Save")) {
        std::string error;
        if (Aether::Prefab::SavePrefab(m_scene, m_prefabSaveEntityId, m_prefabPath, &error)) {
          Log(std::string("Prefab saved: ") + m_prefabPath);
          ImGui::CloseCurrentPopup();
        } else {
          Log("Prefab save failed: " + error);
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (m_openInstantiatePrefabPopup) {
      ImGui::OpenPopup("Instantiate Prefab");
      m_openInstantiatePrefabPopup = false;
    }

    if (ImGui::BeginPopupModal("Instantiate Prefab", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::InputText("Path", m_prefabPath, sizeof(m_prefabPath));
      if (ImGui::Button("Instantiate")) {
        std::string error;
        const auto id = Aether::Prefab::InstantiatePrefab(m_prefabPath, m_scene, &error);
        if (id.has_value()) {
          SelectSingle(*id);
          // New scripts will load on next tick; allow immediate reload as well.
          if (m_scene.GetScript(*id)) {
            (void)m_scriptEngine.ReloadEntityScript(*id);
          }
          Log(std::string("Prefab instantiated: ") + m_prefabPath + " (id=" + std::to_string(*id) + ")");
          ImGui::CloseCurrentPopup();
        } else {
          Log("Prefab instantiate failed: " + error);
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }

  void OpenSavePrefabPopup(uint32_t entityId) {
    m_prefabSaveEntityId = entityId;

    const Aether::Entity* e = m_scene.FindById(entityId);
    const std::string base = e ? e->name : std::string("Prefab");
    const std::string sanitized = base;

    std::snprintf(m_prefabPath, sizeof(m_prefabPath), "Assets/Prefabs/%s.prefab.json", sanitized.c_str());
    m_openSavePrefabPopup = true;
  }

  void SetupAssetPaths() {
    namespace fs = std::filesystem;

    std::error_code ec;
    fs::path cwd = fs::current_path(ec);
    if (ec) {
      Log(std::string("Asset path init: current_path failed: ") + ec.message());
      cwd = fs::path(".");
    }

    fs::path probe = cwd;
    fs::path root = cwd;
    bool found = false;
    for (int i = 0; i < 6; ++i) {
      const bool hasAssets = fs::exists(probe / "Assets", ec);
      const bool hasEngine = fs::exists(probe / "Engine", ec);
      const bool hasSandbox = fs::exists(probe / "Sandbox", ec);
      if (hasAssets && hasEngine && hasSandbox) {
        root = probe;
        found = true;
        break;
      }
      if (!probe.has_parent_path()) break;
      const fs::path parent = probe.parent_path();
      if (parent == probe) break;
      probe = parent;
    }

    m_projectRoot = root;
    m_scriptsDir = m_projectRoot / "Assets" / "Scripts";

    fs::create_directories(m_scriptsDir, ec);
    if (ec) {
      Log(std::string("Asset path init: create_directories failed: ") + ec.message());
    }

    if (found && root != cwd) {
      fs::current_path(root, ec);
      if (ec) {
        Log(std::string("Asset path init: failed to set working directory: ") + ec.message());
      } else {
        Log(std::string("Working directory set to project root: ") + root.string());
      }
    } else {
      std::error_code ec2;
      fs::path cur = fs::current_path(ec2);
      Log(std::string("Working directory: ") + (ec2 ? std::string("<unknown>") : cur.string()));
    }
  }

  void SyncScriptEditorSelection() {
    const auto& scripts = m_scriptRegistry.GetAvailableScripts(&m_scene);

    auto findIndexByPath = [&scripts](const std::string& path) -> int {
      for (size_t i = 0; i < scripts.size(); ++i) {
        if (scripts[i].path == path) {
          return static_cast<int>(i);
        }
      }
      return -1;
    };

    if (!m_openScriptPath.empty()) {
      const int idx = findIndexByPath(m_openScriptPath);
      if (idx < 0) {
        m_openScriptPath.clear();
        m_openScriptText.clear();
        m_openScriptDirty = false;
        m_selectedScriptIndex = -1;
      } else {
        m_selectedScriptIndex = idx;
      }
      return;
    }

    if (m_selectedScriptIndex < 0) {
      return;
    }

    if (m_selectedScriptIndex >= static_cast<int>(scripts.size())) {
      m_selectedScriptIndex = -1;
    }
  }

  static bool LoadTextFile(const std::filesystem::path& path, std::string& outText, std::string& outError) {
    try {
      std::ifstream in(path, std::ios::binary);
      if (!in) {
        outError = "Failed to open file for reading: " + path.string();
        return false;
      }
      std::stringstream buffer;
      buffer << in.rdbuf();
      outText = buffer.str();
      return true;
    } catch (const std::exception& ex) {
      outError = std::string("Load exception: ") + ex.what();
      return false;
    }
  }

  static bool SaveTextFile(const std::filesystem::path& path, const std::string& text, std::string& outError) {
    try {
      const std::filesystem::path parent = path.parent_path();
      if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
      }

      std::ofstream out(path, std::ios::binary);
      if (!out) {
        outError = "Failed to open file for writing: " + path.string();
        return false;
      }
      out << text;
      return true;
    } catch (const std::exception& ex) {
      outError = std::string("Save exception: ") + ex.what();
      return false;
    }
  }

  std::filesystem::path ResolvePath(const std::string& relOrAbs) const {
    namespace fs = std::filesystem;
    fs::path p(relOrAbs);
    if (p.is_relative()) {
      p = m_projectRoot / p;
    }
    return p;
  }

  bool OpenScriptInEditor(const std::string& relPath) {
    const std::filesystem::path absPath = ResolvePath(relPath);

    std::string text;
    std::string error;
    if (!LoadTextFile(absPath, text, error)) {
      Log("Script open failed: " + error);
      return false;
    }

    m_openScriptPath = relPath;
    m_openScriptText = std::move(text);
    m_openScriptDirty = false;

    SyncScriptEditorSelection();
    return true;
  }

  bool SaveOpenScript() {
    if (m_openScriptPath.empty()) {
      return false;
    }

    const std::filesystem::path absPath = ResolvePath(m_openScriptPath);

    std::string error;
    if (!SaveTextFile(absPath, m_openScriptText, error)) {
      Log("Script save failed: " + error);
      return false;
    }

    m_openScriptDirty = false;
    Log(std::string("Script saved: ") + m_openScriptPath);
    return true;
  }

  bool CreateNewScriptFile(const std::string& filename, std::string* outRelPath) {
    namespace fs = std::filesystem;

    std::string name = filename;
    name.erase(std::remove(name.begin(), name.end(), '\r'), name.end());
    name.erase(std::remove(name.begin(), name.end(), '\n'), name.end());
    while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back()))) name.pop_back();
    while (!name.empty() && std::isspace(static_cast<unsigned char>(name.front()))) name.erase(name.begin());

    if (name.empty()) {
      Log("New script failed: empty name");
      return false;
    }

    if (name.find('\\') != std::string::npos || name.find('/') != std::string::npos) {
      Log("New script failed: name must be a filename only (no directories)");
      return false;
    }

    fs::path filePath(name);
    if (filePath.extension().empty()) {
      filePath += ".lua";
    }

    fs::path absPath = m_scriptsDir / filePath;
    if (fs::exists(absPath)) {
      Log(std::string("New script failed: file already exists: ") + absPath.string());
      return false;
    }

    const std::string templateText =
      std::string("-- ") + filePath.filename().string() + "\n"
      "local t = 0\n"
      "function update(dt)\n"
      "  t = t + dt\n"
      "  -- Example: move along X\n"
      "  if this then\n"
      "    local p = this.get_position()\n"
      "    p.x = p.x + math.sin(t) * dt * 0.5\n"
      "    this.set_position(p)\n"
      "  end\n"
      "end\n";

    std::string error;
    if (!SaveTextFile(absPath, templateText, error)) {
      Log("New script failed: " + error);
      return false;
    }

    std::error_code ec;
    const fs::path rel = fs::relative(absPath, m_projectRoot, ec);
    const std::string relPath = (!ec) ? rel.generic_string() : absPath.generic_string();
    if (outRelPath) {
      *outRelPath = relPath;
    }

    Log(std::string("New script created: ") + relPath);
    return true;
  }

  bool DeleteScriptFile(const std::string& relPath) {
    namespace fs = std::filesystem;

    const fs::path absPath = ResolvePath(relPath);

    std::error_code ec;
    if (!fs::exists(absPath, ec)) {
      Log(std::string("Delete script: file not found: ") + relPath);
      return false;
    }

    if (!fs::remove(absPath, ec) || ec) {
      Log(std::string("Delete script failed: ") + (ec ? ec.message() : std::string("unknown error")));
      return false;
    }

    if (m_openScriptPath == relPath) {
      m_openScriptPath.clear();
      m_openScriptText.clear();
      m_openScriptDirty = false;
    }

    Log(std::string("Deleted script: ") + relPath);
    return true;
  }

  void RunScriptOnAttachedEntities(const std::string& relPath) {
    if (relPath.empty()) {
      return;
    }

    int ranCount = 0;

    for (auto& [id, sc] : m_scene.GetScripts()) {
      if (sc.ScriptPath != relPath) {
        continue;
      }

      sc.Instance = sol::table{};
      if (!m_scriptEngine.ReloadEntityScript(id)) {
        continue;
      }

      sol::object updateObj = sc.Instance["update"];
      if (updateObj.is<sol::protected_function>()) {
        sol::protected_function update = updateObj.as<sol::protected_function>();
        sol::protected_function_result result = update(m_lastDeltaSeconds);
        if (!result.valid()) {
          const sol::error err = result;
          Log(std::string("[Script] Run error: ") + err.what());
        } else {
          ++ranCount;
        }
      }
    }

    if (ranCount == 0) {
      Log(std::string("[Script] Run: no attached entities for '") + relPath + "'");
    } else {
      Log(std::string("[Script] Run: executed update() for ") + std::to_string(ranCount) + " entities (" + relPath + ")");
    }
  }

  void ApplyScriptToEntities(const std::vector<uint32_t>& entityIds, const std::string& relPath) {
    if (entityIds.empty()) {
      return;
    }

    if (relPath.empty()) {
      int removed = 0;
      for (uint32_t id : entityIds) {
        if (m_scene.GetScript(id)) {
          m_scene.RemoveScript(id);
          ++removed;
        }
      }
      if (entityIds.size() == 1) {
        Log(std::string("Removed ScriptComponent from entity id=") + std::to_string(entityIds[0]));
      } else {
        Log(std::string("Removed ScriptComponent from ") + std::to_string(removed) + " entities");
      }
      return;
    }

    int applied = 0;
    for (uint32_t id : entityIds) {
      Aether::ScriptComponent& sc = m_scene.AddScript(id);
      sc.ScriptPath = relPath;
      sc.Instance = sol::table{};
      if (m_scriptEngine.ReloadEntityScript(id)) {
        ++applied;
      }
    }

    if (entityIds.size() == 1) {
      const Aether::Entity* e = m_scene.FindById(entityIds[0]);
      Log(std::string("Attached script '") + relPath + "' to " + (e ? e->name : std::string("entity")) +
          " (id=" + std::to_string(entityIds[0]) + ")");
    } else {
      Log(std::string("Attached script '") + relPath + "' to " + std::to_string(applied) + " entities");
    }
  }

  void AttachScriptToEntity(uint32_t entityId, const std::string& relPath) {
    ApplyScriptToEntities(std::vector<uint32_t>{entityId}, relPath);
  }

  void DrawScriptsWindow() {
    if (!m_showScriptsWindow) {
      return;
    }

    SyncScriptEditorSelection();
    const auto& scripts = m_scriptRegistry.GetAvailableScripts(&m_scene);

    ImGui::Begin("Scripts", &m_showScriptsWindow);

    if (ImGui::Button("New")) {
      std::snprintf(m_newScriptNameBuf, sizeof(m_newScriptNameBuf), "%s", "new_script.lua");
      m_openNewScriptPopup = true;
    }
    ImGui::SameLine();

    const bool hasSelection =
      (m_selectedScriptIndex >= 0) && (m_selectedScriptIndex < static_cast<int>(scripts.size()));
    if (ImGui::Button("Open")) {
      if (hasSelection) {
        const std::string& path = scripts[static_cast<size_t>(m_selectedScriptIndex)].path;
        (void)OpenScriptInEditor(path);
      }
    }
    ImGui::SameLine();

    if (ImGui::Button("Save")) {
      (void)SaveOpenScript();
    }
    ImGui::SameLine();

    if (ImGui::Button("Delete")) {
      if (hasSelection) {
        m_deleteScriptPath = scripts[static_cast<size_t>(m_selectedScriptIndex)].path;
        m_openDeleteScriptPopup = true;
      }
    }
    ImGui::SameLine();

    if (ImGui::Button("Run")) {
      const std::string toRun =
        !m_openScriptPath.empty() ? m_openScriptPath : (hasSelection ? scripts[static_cast<size_t>(m_selectedScriptIndex)].path : std::string());
      if (!toRun.empty()) {
        if (m_openScriptDirty && m_openScriptPath == toRun) {
          (void)SaveOpenScript();
        }
        RunScriptOnAttachedEntities(toRun);
      }
    }

    ImGui::Separator();

    if (m_openNewScriptPopup) {
      ImGui::OpenPopup("New Script");
      m_openNewScriptPopup = false;
    }
    if (ImGui::BeginPopupModal("New Script", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::InputText("Filename", m_newScriptNameBuf, sizeof(m_newScriptNameBuf));
      if (ImGui::Button("Create")) {
        std::string rel;
        if (CreateNewScriptFile(m_newScriptNameBuf, &rel)) {
          m_scriptRegistry.Refresh();
          (void)OpenScriptInEditor(rel);
          ImGui::CloseCurrentPopup();
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (m_openDeleteScriptPopup) {
      ImGui::OpenPopup("Delete Script");
      m_openDeleteScriptPopup = false;
    }
    if (ImGui::BeginPopupModal("Delete Script", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Delete '%s' ?", m_deleteScriptPath.c_str());
      if (ImGui::Button("Delete")) {
        (void)DeleteScriptFile(m_deleteScriptPath);
        m_scriptRegistry.Refresh();
        m_deleteScriptPath.clear();
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (ImGui::BeginTable("ScriptsSplit", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
      ImGui::TableSetupColumn("List", ImGuiTableColumnFlags_WidthFixed, 240.0f);
      ImGui::TableSetupColumn("Editor", ImGuiTableColumnFlags_WidthStretch);

      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      ImGui::BeginChild("##script_list", ImVec2(0.0f, 0.0f), true);
      for (size_t i = 0; i < scripts.size(); ++i) {
        const bool selected = (static_cast<int>(i) == m_selectedScriptIndex);
        const Aether::ScriptInfo& info = scripts[i];
        const std::string& path = info.path;
        if (ImGui::Selectable(path.c_str(), selected)) {
          m_selectedScriptIndex = static_cast<int>(i);
          (void)OpenScriptInEditor(path);
        }
        if (ImGui::IsItemHovered()) {
          if (!info.description.empty()) {
            ImGui::SetTooltip("%s\n\n%s", info.description.c_str(), path.c_str());
          } else {
            ImGui::SetTooltip("%s", path.c_str());
          }
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
          ImGui::SetDragDropPayload("AETHER_SCRIPT_PATH", path.c_str(), path.size() + 1);
          ImGui::TextUnformatted(path.c_str());
          ImGui::EndDragDropSource();
        }
      }
      ImGui::EndChild();

      ImGui::TableSetColumnIndex(1);
      ImGui::BeginChild("##script_editor", ImVec2(0.0f, 0.0f), true);
      if (m_openScriptPath.empty()) {
        ImGui::TextUnformatted("Select a script to edit.");
      } else {
        ImGui::Text("%s%s", m_openScriptPath.c_str(), m_openScriptDirty ? " *" : "");
        ImGui::Separator();

        const ImVec2 editorSize = ImGui::GetContentRegionAvail();
        const bool changed =
          InputTextMultilineString("##code", m_openScriptText, editorSize, ImGuiInputTextFlags_AllowTabInput);
        if (changed) {
          m_openScriptDirty = true;
        }
      }
      ImGui::EndChild();

      ImGui::EndTable();
    }

    ImGui::End();
  }

  void CreateDemoScene() {
    m_scene.Clear();
    ClearSelection();
    m_lastClickedHierarchyIndex = -1;

    m_camera.SetPosition({0.0f, 1.6f, 6.0f});
    m_camera.SetYawPitch(-90.0f, -12.0f);

    m_scene.GetAmbientColor() = {0.06f, 0.06f, 0.07f};
    m_scene.GetAmbientIntensity() = 1.0f;

    Aether::DirectionalLight& dl = m_scene.GetDirectionalLight();
    dl.direction = {-0.35f, -1.0f, -0.20f};
    dl.color = {1.0f, 0.98f, 0.95f};
    dl.intensity = 1.4f;

    // Ground
    {
      Aether::Entity& e = m_scene.CreateEntity("Ground");
      e.transform.position = {0.0f, -1.25f, 0.0f};
      e.transform.scale = {8.0f, 0.5f, 8.0f};
      if (Aether::MaterialComponent* mc = m_scene.GetMaterial(e.id)) {
        mc->material.albedo = {0.18f, 0.19f, 0.22f};
        mc->material.specularStrength = 0.05f;
        mc->material.shininess = 8.0f;
      }
    }

    // Cubes
    {
      Aether::Entity& e = m_scene.CreateEntity("Cube A");
      e.transform.position = {-1.8f, 0.0f, 0.0f};
      if (Aether::MaterialComponent* mc = m_scene.GetMaterial(e.id)) {
        mc->material.albedo = {0.0f, 0.90f, 1.0f};
        mc->material.specularStrength = 0.35f;
        mc->material.shininess = 64.0f;
      }
    }

    {
      Aether::Entity& e = m_scene.CreateEntity("Cube B");
      e.transform.position = {1.6f, 0.0f, -1.2f};
      e.transform.rotationDeg = {0.0f, 35.0f, 0.0f};
      if (Aether::MaterialComponent* mc = m_scene.GetMaterial(e.id)) {
        mc->material.albedo = {1.0f, 0.28f, 0.18f};
        mc->material.specularStrength = 0.15f;
        mc->material.shininess = 16.0f;
      }
    }

    {
      Aether::Entity& e = m_scene.CreateEntity("Cube C");
      e.transform.position = {0.0f, 0.6f, 2.2f};
      e.transform.scale = {0.75f, 0.75f, 0.75f};
      if (Aether::MaterialComponent* mc = m_scene.GetMaterial(e.id)) {
        mc->material.albedo = {0.24f, 0.95f, 0.28f};
        mc->material.specularStrength = 0.55f;
        mc->material.shininess = 96.0f;
      }
    }

    // Scripted cube (uses Assets/Scripts/move.lua).
    {
      Aether::Entity& e = m_scene.CreateEntity("Scripted Cube");
      e.transform.position = {0.0f, 0.25f, -2.0f};
      if (Aether::MaterialComponent* mc = m_scene.GetMaterial(e.id)) {
        mc->material.albedo = {0.75f, 0.55f, 1.0f};
        mc->material.specularStrength = 0.25f;
        mc->material.shininess = 24.0f;
      }
      Aether::ScriptComponent& sc = m_scene.AddScript(e.id);
      sc.ScriptPath = "Assets/Scripts/move.lua";
      sc.Instance = sol::table{};
      (void)m_scriptEngine.ReloadEntityScript(e.id);
    }

    // Point light visualized as a small cube.
    {
      Aether::Entity& e = m_scene.CreateEntity("Lamp (PointLight)");
      e.transform.position = {0.0f, 2.0f, 1.5f};
      e.transform.scale = {0.2f, 0.2f, 0.2f};

      if (Aether::MaterialComponent* mc = m_scene.GetMaterial(e.id)) {
        mc->material.albedo = {1.0f, 0.92f, 0.75f};
        mc->material.specularStrength = 0.0f;
        mc->material.shininess = 1.0f;
      }

      Aether::PointLightComponent& pl = m_scene.AddPointLight(e.id);
      pl.color = {1.0f, 0.85f, 0.60f};
      pl.intensity = 30.0f;
      pl.radius = 8.0f;
    }

    Log("Scene: demo scene created (" + std::to_string(m_scene.GetEntities().size()) + " entities)");
  }

  bool SaveScene(const char* path) {
    if (!path || path[0] == '\0') {
      Log("Scene save failed: empty path");
      return false;
    }

    std::string error;
    if (!Aether::SceneSerializer::SerializeToFile(m_scene, path, &error)) {
      Log("Scene save failed: " + error);
      return false;
    }

    Log(std::string("Scene saved: ") + path);
    return true;
  }

  bool LoadScene(const char* path) {
    if (!path || path[0] == '\0') {
      Log("Scene load failed: empty path");
      return false;
    }

    Aether::Scene loaded;
    std::string error;
    if (!Aether::SceneSerializer::DeserializeFromFile(loaded, path, &error)) {
      Log("Scene load failed: " + error);
      return false;
    }

    m_scene = std::move(loaded);
    ClearSelection();
    m_lastClickedHierarchyIndex = -1;

    // Ensure scripts reload into the current Lua state.
    for (auto& [id, sc] : m_scene.GetScripts()) {
      sc.Instance = sol::table{};
    }

    Log(std::string("Scene loaded: ") + path + " (" + std::to_string(m_scene.GetEntities().size()) + " entities)");
    return true;
  }

  void Log(std::string message) {
    std::cout << "[Sandbox] " << message << "\n";
    m_consoleLines.push_back(std::move(message));
  }

  Aether::Renderer m_renderer;
  Aether::ImGuiLayer m_imgui;
  std::unique_ptr<Aether::Framebuffer> m_viewportFramebuffer;
  glm::ivec2 m_viewportSize{0, 0};

  Aether::Camera m_camera;
  bool m_viewportHovered = false;
  bool m_hadMouseLastFrame = false;
  bool m_lookActiveLastFrame = false;
  double m_lastMouseX = 0.0;
  double m_lastMouseY = 0.0;
  float m_lastDeltaSeconds = 0.0f;
  float m_timeSeconds = 0.0f;

  Aether::Scene m_scene;
  std::vector<uint32_t> m_selectedEntityIds;
  uint32_t m_primarySelectionId = 0;
  int m_lastClickedHierarchyIndex = -1;

  Aether::ScriptEngine m_scriptEngine;
  Aether::ScriptRegistry m_scriptRegistry;

  std::vector<std::string> m_consoleLines;
  bool m_consoleAutoScroll = true;

  ImGuizmo::OPERATION m_gizmoOperation = ImGuizmo::TRANSLATE;

  char m_scenePath[256] = "scene.json";

  uint32_t m_prefabSaveEntityId = 0;
  bool m_openSavePrefabPopup = false;
  bool m_openInstantiatePrefabPopup = false;
  char m_prefabPath[256] = "Assets/Prefabs/prefab.json";

  bool m_showScriptsWindow = false;
  std::filesystem::path m_projectRoot;
  std::filesystem::path m_scriptsDir;
  int m_selectedScriptIndex = -1;
  std::string m_openScriptPath;
  std::string m_openScriptText;
  bool m_openScriptDirty = false;

  char m_scriptDropdownSearchBuf[128] = "";

  bool m_openNewScriptPopup = false;
  char m_newScriptNameBuf[128] = "new_script.lua";

  bool m_openDeleteScriptPopup = false;
  std::string m_deleteScriptPath;
};

int main(int argc, char** argv) {
  SandboxApp app(argc, argv);
  return app.Run();
}
