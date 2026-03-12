#include "Editor/ViewportPanel.h"

#include "EditorState.h"

#include "Rendering/Framebuffer.h"

#include <algorithm>
#include <cfloat>

namespace Aether::Editor {

ViewportPanelOutput DrawViewportPanel(Framebuffer& framebuffer, bool showLogo) {
  ViewportPanelOutput out;

  ImGui::Begin("Viewport");
  out.windowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_None);

  const ImVec2 avail = ImGui::GetContentRegionAvail();
  const int viewportW = (avail.x > 0.0f) ? static_cast<int>(avail.x) : 0;
  const int viewportH = (avail.y > 0.0f) ? static_cast<int>(avail.y) : 0;
  if (viewportW > 0 && viewportH > 0) {
    framebuffer.Resize(viewportW, viewportH);
    out.size = {viewportW, viewportH};
  }

  const uint32_t colorTex = framebuffer.GetColorAttachmentID();
  out.imageMin = ImGui::GetCursorScreenPos();
  out.imageSize = ImVec2(static_cast<float>(viewportW), static_cast<float>(viewportH));

  ImGui::Image(
    (void*)(intptr_t)colorTex,
    out.imageSize,
    ImVec2(0.0f, 1.0f),
    ImVec2(1.0f, 0.0f)
  );
  out.imageHovered = ImGui::IsItemHovered();
  out.imageActive = ImGui::IsItemActive();

  if (showLogo && viewportW > 0 && viewportH > 0) {
    // SVG reference (not parsed): see README / prompt.
    static constexpr const char* kLogoSvg = R"(
<svg>
<circle cx="32" cy="32" r="30" fill="#00E5FF" />
<polygon points="32,10 12,54 52,54" fill="#002BFF"/>
<text x="32" y="40" font-size="28" text-anchor="middle" fill="#fff">A</text>
</svg>
)";
    (void)kLogoSvg;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 center(out.imageMin.x + out.imageSize.x * 0.5f, out.imageMin.y + out.imageSize.y * 0.5f);
    const float r = std::clamp(std::min(out.imageSize.x, out.imageSize.y) * 0.08f, 24.0f, 64.0f);
    const float scale = r / 30.0f;

    const ImU32 cCircle = IM_COL32(0, 229, 255, 220);
    const ImU32 cTri = IM_COL32(0, 43, 255, 220);
    const ImU32 cText = IM_COL32(255, 255, 255, 235);

    dl->AddCircleFilled(center, r, cCircle, 48);

    const ImVec2 p0(center.x + 0.0f * scale, center.y + -22.0f * scale);
    const ImVec2 p1(center.x + -20.0f * scale, center.y + 22.0f * scale);
    const ImVec2 p2(center.x + 20.0f * scale, center.y + 22.0f * scale);
    dl->AddTriangleFilled(p0, p1, p2, cTri);

    const float fontSize = r * 0.95f;
    ImFont* font = ImGui::GetFont();
    const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, "A");
    const ImVec2 textPos(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f - r * 0.05f);
    dl->AddText(font, fontSize, textPos, cText, "A");
  }

  ImGui::End();
  return out;
}

} // namespace Aether::Editor

