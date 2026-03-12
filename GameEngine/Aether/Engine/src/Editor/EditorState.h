#pragma once

#include <functional>

namespace Aether::Editor {

// Shared editor UI state flags (kept in Engine so other modules can query).
inline bool g_ShowLogo = true;
inline bool g_ScriptsEnabled = true;

// Optional hook for application-specific dockspace menu entries.
inline std::function<void()> g_DockspaceMenuCallback;

} // namespace Aether::Editor
