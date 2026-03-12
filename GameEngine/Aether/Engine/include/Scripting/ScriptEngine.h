#pragma once

#include <sol/sol.hpp>

#include <cstdint>
#include <functional>
#include <string>

namespace Aether {

class Scene;
struct ScriptComponent;

class ScriptEngine {
public:
  using LogFn = std::function<void(const std::string&)>;

  ScriptEngine();
  ~ScriptEngine();

  ScriptEngine(const ScriptEngine&) = delete;
  ScriptEngine& operator=(const ScriptEngine&) = delete;

  void Init(Scene* scene);
  void Shutdown();

  void SetLogger(LogFn fn);

  bool ExecuteFile(const std::string& path);
  bool ReloadEntityScript(uint32_t entityId);

  void Tick(float dt);

private:
  sol::table BuildThisTable(uint32_t entityId);
  void Log(const std::string& message) const;

  sol::state m_lua;
  Scene* m_scene = nullptr;
  LogFn m_logFn;
};

} // namespace Aether

