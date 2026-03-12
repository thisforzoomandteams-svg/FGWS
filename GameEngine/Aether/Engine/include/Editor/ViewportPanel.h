#pragma once

#include <glm/vec2.hpp>

#include <imgui.h>

namespace Aether {
class Framebuffer;
}

namespace Aether::Editor {

struct ViewportPanelOutput {
  glm::ivec2 size{0, 0};
  bool windowHovered = false;
  bool imageHovered = false;
  bool imageActive = false;

  ImVec2 imageMin{0.0f, 0.0f};
  ImVec2 imageSize{0.0f, 0.0f};
};

ViewportPanelOutput DrawViewportPanel(Framebuffer& framebuffer, bool showLogo);

} // namespace Aether::Editor

