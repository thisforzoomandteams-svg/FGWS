#pragma once

#include <filesystem>
#include <string>

namespace Aether {

class Scene;

class SceneSerializer {
public:
  static constexpr int kSceneVersion = 1;

  static bool SerializeToFile(const Scene& scene, const std::filesystem::path& path, std::string* outError = nullptr);
  static bool DeserializeFromFile(Scene& scene, const std::filesystem::path& path, std::string* outError = nullptr);
};

} // namespace Aether

