#include "Scripting/ScriptEngine.h"

#include "ECS/Components/ScriptComponent.h"
#include "ECS/Scene.h"

#include <glm/vec3.hpp>

#include <cmath>
#include <iostream>

namespace Aether {

ScriptEngine::ScriptEngine() = default;

ScriptEngine::~ScriptEngine() {
  Shutdown();
}

void ScriptEngine::Init(Scene* scene) {
  m_scene = scene;

  m_lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string, sol::lib::package);

  // Engine-side log. The editor can supply a callback to mirror logs into its Console panel.
  m_lua.set_function("AetherLog", [this](const std::string& s) { Log(std::string("[LUA] ") + s); });

  Log("[Script] ScriptEngine initialized");
}

void ScriptEngine::Shutdown() {
  if (!m_scene) {
    return;
  }

  // Best-effort cleanup.
  m_scene = nullptr;
  m_logFn = nullptr;
  m_lua = sol::state{};
}

void ScriptEngine::SetLogger(LogFn fn) {
  m_logFn = std::move(fn);
}

void ScriptEngine::Log(const std::string& message) const {
  if (m_logFn) {
    m_logFn(message);
  } else {
    std::cout << message << "\n";
  }
}

bool ScriptEngine::ExecuteFile(const std::string& path) {
  sol::load_result chunk = m_lua.load_file(path);
  if (!chunk.valid()) {
    const sol::error err = chunk;
    Log(std::string("[Script] load error: ") + err.what());
    return false;
  }

  sol::protected_function script = chunk;
  const sol::protected_function_result result = script();
  if (!result.valid()) {
    const sol::error err = result;
    Log(std::string("[Script] runtime error: ") + err.what());
    return false;
  }

  return true;
}

sol::table ScriptEngine::BuildThisTable(uint32_t entityId) {
  sol::table thisTable = m_lua.create_table();

  thisTable["get_name"] = [this, entityId]() -> std::string {
    if (!m_scene) {
      return {};
    }
    const Entity* e = m_scene->FindById(entityId);
    return e ? e->name : std::string{};
  };

  thisTable["get_position"] = [this, entityId](sol::this_state s) -> sol::table {
    sol::state_view lua(s);
    sol::table t = lua.create_table();
    if (!m_scene) {
      t["x"] = 0.0f;
      t["y"] = 0.0f;
      t["z"] = 0.0f;
      return t;
    }
    const Entity* e = m_scene->FindById(entityId);
    const glm::vec3 p = e ? e->transform.position : glm::vec3(0.0f);
    t["x"] = p.x;
    t["y"] = p.y;
    t["z"] = p.z;
    return t;
  };

  auto setPosTable = [this, entityId](sol::table pos) {
    if (!m_scene) {
      return;
    }
    Entity* e = m_scene->FindById(entityId);
    if (!e) {
      return;
    }
    const float x = pos.get_or("x", e->transform.position.x);
    const float y = pos.get_or("y", e->transform.position.y);
    const float z = pos.get_or("z", e->transform.position.z);
    e->transform.position = {x, y, z};
  };

  auto setPosXYZ = [this, entityId](float x, float y, float z) {
    if (!m_scene) {
      return;
    }
    Entity* e = m_scene->FindById(entityId);
    if (!e) {
      return;
    }
    e->transform.position = {x, y, z};
  };

  thisTable["set_position"] = sol::overload(setPosTable, setPosXYZ);

  thisTable["get_rotation"] = [this, entityId](sol::this_state s) -> sol::table {
    sol::state_view lua(s);
    sol::table t = lua.create_table();
    if (!m_scene) {
      t["x"] = 0.0f;
      t["y"] = 0.0f;
      t["z"] = 0.0f;
      return t;
    }
    const Entity* e = m_scene->FindById(entityId);
    const glm::vec3 r = e ? e->transform.rotationDeg : glm::vec3(0.0f);
    t["x"] = r.x;
    t["y"] = r.y;
    t["z"] = r.z;
    return t;
  };

  auto setRotTable = [this, entityId](sol::table rot) {
    if (!m_scene) {
      return;
    }
    Entity* e = m_scene->FindById(entityId);
    if (!e) {
      return;
    }
    const float x = rot.get_or("x", e->transform.rotationDeg.x);
    const float y = rot.get_or("y", e->transform.rotationDeg.y);
    const float z = rot.get_or("z", e->transform.rotationDeg.z);
    e->transform.rotationDeg = {x, y, z};
  };

  auto setRotXYZ = [this, entityId](float x, float y, float z) {
    if (!m_scene) {
      return;
    }
    Entity* e = m_scene->FindById(entityId);
    if (!e) {
      return;
    }
    e->transform.rotationDeg = {x, y, z};
  };

  thisTable["set_rotation"] = sol::overload(setRotTable, setRotXYZ);

  thisTable["log"] = [this](const std::string& msg) { Log(std::string("[LUA] ") + msg); };

  // Minimal component helpers (string-based).
  thisTable["has_component"] = [this, entityId](const std::string& name) -> bool {
    if (!m_scene) {
      return false;
    }
    if (name == "material") return m_scene->HasMaterial(entityId);
    if (name == "point_light") return m_scene->HasPointLight(entityId);
    if (name == "script") return m_scene->HasScript(entityId);
    return false;
  };

  thisTable["add_component"] = [this, entityId](const std::string& name) {
    if (!m_scene) {
      return;
    }
    if (name == "material") {
      (void)m_scene->AddMaterial(entityId);
    } else if (name == "point_light") {
      (void)m_scene->AddPointLight(entityId);
    } else if (name == "script") {
      (void)m_scene->AddScript(entityId);
    }
  };

  return thisTable;
}

bool ScriptEngine::ReloadEntityScript(uint32_t entityId) {
  if (!m_scene) {
    Log("[Script] ReloadEntityScript failed: no scene");
    return false;
  }

  ScriptComponent* sc = m_scene->GetScript(entityId);
  if (!sc) {
    Log("[Script] ReloadEntityScript: entity has no ScriptComponent");
    return false;
  }
  if (sc->ScriptPath.empty()) {
    Log("[Script] ReloadEntityScript: empty script path");
    sc->Instance = sol::table{};
    return false;
  }

  sol::load_result chunk = m_lua.load_file(sc->ScriptPath);
  if (!chunk.valid()) {
    const sol::error err = chunk;
    Log(std::string("[Script] load error: ") + err.what());
    sc->Instance = sol::table{};
    return false;
  }

  sol::environment env(m_lua, sol::create, m_lua.globals());
  env["this"] = BuildThisTable(entityId);

  sol::protected_function script = chunk;
  sol::set_environment(env, script);

  const sol::protected_function_result result = script();
  if (!result.valid()) {
    const sol::error err = result;
    Log(std::string("[Script] runtime error: ") + err.what());
    sc->Instance = sol::table{};
    return false;
  }

  sc->Instance = env;
  Log("[Script] Loaded script for entity id=" + std::to_string(entityId) + " path='" + sc->ScriptPath + "'");
  return true;
}

void ScriptEngine::Tick(float dt) {
  if (!m_scene) {
    return;
  }

  auto& scripts = m_scene->GetScripts();
  for (auto& [entityId, sc] : scripts) {
    if (!sc.Instance.valid() && !sc.ScriptPath.empty()) {
      (void)ReloadEntityScript(entityId);
    }

    if (!sc.Instance.valid()) {
      continue;
    }

    sol::object updateObj = sc.Instance["update"];
    if (updateObj.is<sol::protected_function>()) {
      sol::protected_function update = updateObj.as<sol::protected_function>();
      const sol::protected_function_result result = update(dt);
      if (!result.valid()) {
        const sol::error err = result;
        Log(std::string("[Script] update error: ") + err.what());
      }
    }
  }
}

} // namespace Aether
