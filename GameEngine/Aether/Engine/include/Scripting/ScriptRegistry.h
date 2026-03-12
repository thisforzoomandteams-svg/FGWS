#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Aether {

class Scene;

struct ScriptInfo {
  std::string name;        // file stem (display-friendly)
  std::string path;        // relative or absolute path used by ScriptEngine::load_file
  std::string description; // extracted from file header comments (best-effort)
};

// Tracks scripts available to the current editor/session.
// The registry is intentionally asset-source-agnostic: callers decide which folders to scan.
class ScriptRegistry {
public:
  void AddSearchPath(std::filesystem::path path);
  void ClearSearchPaths();

  // Full rescan of all search paths.
  void Refresh();

  // Cheap check (directory timestamps) to refresh when the scripts folder changes.
  // Returns true if a refresh happened.
  bool RefreshIfNeeded();

  // Future hook: could filter scripts based on the active scene. Currently returns all scripts.
  const std::vector<ScriptInfo>& GetAvailableScripts(const Scene* scene = nullptr) const;

private:
  static std::string ExtractDescription(const std::filesystem::path& filePath);
  static std::string ToLower(std::string s);

  struct DirState {
    std::filesystem::path path;
    std::filesystem::file_time_type lastWriteTime{};
    bool hasWriteTime = false;
  };

  std::vector<DirState> m_dirs;
  std::vector<ScriptInfo> m_scripts;
};

} // namespace Aether

