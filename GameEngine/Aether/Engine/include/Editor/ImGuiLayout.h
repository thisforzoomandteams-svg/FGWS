#pragma once

namespace Aether::Editor {

// Creates a persistent dockspace and an initial canonical layout:
// - Viewport: center
// - Hierarchy: left
// - Inspector: right
// - Console: bottom
void SetupDockspace(bool* p_open = nullptr);

} // namespace Aether::Editor

