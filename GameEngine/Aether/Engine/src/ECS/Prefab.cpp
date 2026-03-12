#include "ECS/Prefab.h"

#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/ScriptComponent.h"
#include "ECS/Scene.h"

#include <nlohmann/json.hpp>

#include <glm/vec3.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace Aether {
namespace {

using json = nlohmann::json;

json Vec3ToJson(const glm::vec3& v) {
  return json::array({v.x, v.y, v.z});
}

bool JsonToVec3(const json& j, glm::vec3& out) {
  if (!j.is_array() || j.size() != 3) {
    return false;
  }
  if (!j[0].is_number() || !j[1].is_number() || !j[2].is_number()) {
    return false;
  }
  out.x = j[0].get<float>();
  out.y = j[1].get<float>();
  out.z = j[2].get<float>();
  return true;
}

} // namespace

bool Prefab::SavePrefab(const Scene& scene, uint32_t entityId, const std::filesystem::path& path, std::string* outError) {
  const Entity* e = scene.FindById(entityId);
  if (!e) {
    if (outError) *outError = "Entity not found";
    return false;
  }

  json root;
  root["version"] = kPrefabVersion;

  json ent;
  ent["name"] = e->name;
  ent["transform"]["pos"] = Vec3ToJson(e->transform.position);
  ent["transform"]["rot"] = Vec3ToJson(e->transform.rotationDeg);
  ent["transform"]["scale"] = Vec3ToJson(e->transform.scale);

  json comps;

  if (const MaterialComponent* mc = scene.GetMaterial(e->id)) {
    json m;
    m["albedo"] = Vec3ToJson(mc->material.albedo);
    m["specular_color"] = Vec3ToJson(mc->material.specularColor);
    m["specular_strength"] = mc->material.specularStrength;
    m["shininess"] = mc->material.shininess;
    comps["material"] = std::move(m);
  }

  if (const PointLightComponent* pl = scene.GetPointLight(e->id)) {
    json p;
    p["color"] = Vec3ToJson(pl->color);
    p["intensity"] = pl->intensity;
    p["radius"] = pl->radius;
    comps["point_light"] = std::move(p);
  }

  if (const ScriptComponent* sc = scene.GetScript(e->id)) {
    json s;
    s["path"] = sc->ScriptPath;
    comps["script"] = std::move(s);
  }

  if (!comps.empty()) {
    ent["components"] = std::move(comps);
  }

  root["entities"] = json::array({std::move(ent)});

  try {
    const std::filesystem::path parent = path.parent_path();
    if (!parent.empty()) {
      std::error_code ec;
      std::filesystem::create_directories(parent, ec);
    }

    std::ofstream out(path, std::ios::binary);
    if (!out) {
      if (outError) *outError = "Failed to open file for writing: " + path.string();
      return false;
    }
    out << root.dump(2) << "\n";
  } catch (const std::exception& ex) {
    if (outError) *outError = std::string("Save exception: ") + ex.what();
    return false;
  }

  return true;
}

std::optional<uint32_t> Prefab::InstantiatePrefab(const std::filesystem::path& path, Scene& scene, std::string* outError) {
  json root;

  try {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
      if (outError) *outError = "Failed to open file for reading: " + path.string();
      return std::nullopt;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    root = json::parse(buffer.str());
  } catch (const std::exception& ex) {
    if (outError) *outError = std::string("Parse exception: ") + ex.what();
    return std::nullopt;
  }

  if (!root.contains("version") || !root["version"].is_number_integer()) {
    if (outError) *outError = "Missing/invalid prefab 'version'";
    return std::nullopt;
  }
  const int version = root["version"].get<int>();
  if (version != kPrefabVersion) {
    if (outError) *outError = "Unsupported prefab version: " + std::to_string(version);
    return std::nullopt;
  }

  if (!root.contains("entities") || !root["entities"].is_array() || root["entities"].empty()) {
    if (outError) *outError = "Missing/invalid 'entities' array";
    return std::nullopt;
  }

  std::optional<uint32_t> firstId;

  for (const json& ent : root["entities"]) {
    const std::string name = (ent.contains("name") && ent["name"].is_string()) ? ent["name"].get<std::string>() : "PrefabEntity";
    Entity& e = scene.CreateEntity(name);
    if (!firstId) {
      firstId = e.id;
    }

    if (ent.contains("transform")) {
      const json& t = ent["transform"];
      if (t.contains("pos")) (void)JsonToVec3(t["pos"], e.transform.position);
      if (t.contains("rot")) (void)JsonToVec3(t["rot"], e.transform.rotationDeg);
      if (t.contains("scale")) (void)JsonToVec3(t["scale"], e.transform.scale);
    }

    if (ent.contains("components")) {
      const json& comps = ent["components"];

      if (comps.contains("material")) {
        const json& m = comps["material"];
        MaterialComponent* mc = scene.GetMaterial(e.id);
        if (!mc) {
          mc = &scene.AddMaterial(e.id);
        }
        if (m.contains("albedo")) (void)JsonToVec3(m["albedo"], mc->material.albedo);
        if (m.contains("specular_color")) (void)JsonToVec3(m["specular_color"], mc->material.specularColor);
        if (m.contains("specular_strength") && m["specular_strength"].is_number()) mc->material.specularStrength = m["specular_strength"].get<float>();
        if (m.contains("shininess") && m["shininess"].is_number()) mc->material.shininess = m["shininess"].get<float>();
      }

      if (comps.contains("point_light")) {
        const json& p = comps["point_light"];
        PointLightComponent& pl = scene.AddPointLight(e.id);
        if (p.contains("color")) (void)JsonToVec3(p["color"], pl.color);
        if (p.contains("intensity") && p["intensity"].is_number()) pl.intensity = p["intensity"].get<float>();
        if (p.contains("radius") && p["radius"].is_number()) pl.radius = p["radius"].get<float>();
      }

      if (comps.contains("script")) {
        const json& s = comps["script"];
        ScriptComponent& sc = scene.AddScript(e.id);
        if (s.contains("path") && s["path"].is_string()) {
          sc.ScriptPath = s["path"].get<std::string>();
        }
      }
    }
  }

  return firstId;
}

} // namespace Aether

