#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace Aether {

class Scene;

class Prefab {
public:
  static constexpr int kPrefabVersion = 1;

  static bool SavePrefab(const Scene& scene, uint32_t entityId, const std::filesystem::path& path, std::string* outError = nullptr);
  static std::optional<uint32_t> InstantiatePrefab(const std::filesystem::path& path, Scene& scene, std::string* outError = nullptr);
};

} // namespace Aether

