#include "Serialization/SceneSerializer.h"

#include "ECS/Scene.h"

#include <nlohmann/json.hpp>

#include <glm/vec3.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace Aether {
namespace {

using json = nlohmann::json;

json Vec3ToJson(const glm::vec3& v) {
  return json::array({v.x, v.y, v.z});
}

bool JsonToVec3(const json& j, glm::vec3& out, std::string* outError) {
  if (!j.is_array() || j.size() != 3) {
    if (outError) *outError = "Expected vec3 array of size 3";
    return false;
  }
  if (!j[0].is_number() || !j[1].is_number() || !j[2].is_number()) {
    if (outError) *outError = "Expected vec3 numeric array";
    return false;
  }
  out.x = j[0].get<float>();
  out.y = j[1].get<float>();
  out.z = j[2].get<float>();
  return true;
}

} // namespace

bool SceneSerializer::SerializeToFile(const Scene& scene, const std::filesystem::path& path, std::string* outError) {
  json root;
  root["aether_scene_version"] = kSceneVersion;

  root["environment"]["ambient"]["color"] = Vec3ToJson(scene.GetAmbientColor());
  root["environment"]["ambient"]["intensity"] = scene.GetAmbientIntensity();

  const DirectionalLight& dl = scene.GetDirectionalLight();
  root["environment"]["directional_light"]["direction"] = Vec3ToJson(dl.direction);
  root["environment"]["directional_light"]["color"] = Vec3ToJson(dl.color);
  root["environment"]["directional_light"]["intensity"] = dl.intensity;

  json entities = json::array();
  for (const Entity& e : scene.GetEntities()) {
    json ent;
    ent["id"] = e.id;
    ent["name"] = e.name;
    ent["transform"]["position"] = Vec3ToJson(e.transform.position);
    ent["transform"]["rotation_deg"] = Vec3ToJson(e.transform.rotationDeg);
    ent["transform"]["scale"] = Vec3ToJson(e.transform.scale);

    json comps;

    if (const MaterialComponent* mat = scene.GetMaterial(e.id)) {
      json m;
      m["albedo"] = Vec3ToJson(mat->material.albedo);
      m["specular_color"] = Vec3ToJson(mat->material.specularColor);
      m["specular_strength"] = mat->material.specularStrength;
      m["shininess"] = mat->material.shininess;
      comps["material"] = std::move(m);
    }

    if (const PointLightComponent* pl = scene.GetPointLight(e.id)) {
      json p;
      p["color"] = Vec3ToJson(pl->color);
      p["intensity"] = pl->intensity;
      p["radius"] = pl->radius;
      comps["point_light"] = std::move(p);
    }

    if (const ScriptComponent* sc = scene.GetScript(e.id)) {
      json s;
      s["path"] = sc->ScriptPath;
      comps["script"] = std::move(s);
    }

    if (!comps.empty()) {
      ent["components"] = std::move(comps);
    }

    entities.push_back(std::move(ent));
  }

  root["entities"] = std::move(entities);

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
  } catch (const std::exception& e) {
    if (outError) *outError = std::string("Serialize exception: ") + e.what();
    return false;
  }

  return true;
}

bool SceneSerializer::DeserializeFromFile(Scene& scene, const std::filesystem::path& path, std::string* outError) {
  json root;

  try {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
      if (outError) *outError = "Failed to open file for reading: " + path.string();
      return false;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    root = json::parse(buffer.str());
  } catch (const std::exception& e) {
    if (outError) *outError = std::string("Parse exception: ") + e.what();
    return false;
  }

  if (!root.contains("aether_scene_version") || !root["aether_scene_version"].is_number_integer()) {
    if (outError) *outError = "Missing/invalid 'aether_scene_version'";
    return false;
  }
  const int version = root["aether_scene_version"].get<int>();
  if (version != kSceneVersion) {
    if (outError) *outError = "Unsupported scene version: " + std::to_string(version);
    return false;
  }

  scene.Clear();

  if (root.contains("environment")) {
    const json& env = root["environment"];

    if (env.contains("ambient")) {
      const json& amb = env["ambient"];
      if (amb.contains("color")) {
        (void)JsonToVec3(amb["color"], scene.GetAmbientColor(), outError);
      }
      if (amb.contains("intensity") && amb["intensity"].is_number()) {
        scene.GetAmbientIntensity() = amb["intensity"].get<float>();
      }
    }

    if (env.contains("directional_light")) {
      const json& jdl = env["directional_light"];
      DirectionalLight& dl = scene.GetDirectionalLight();
      if (jdl.contains("direction")) {
        (void)JsonToVec3(jdl["direction"], dl.direction, outError);
      }
      if (jdl.contains("color")) {
        (void)JsonToVec3(jdl["color"], dl.color, outError);
      }
      if (jdl.contains("intensity") && jdl["intensity"].is_number()) {
        dl.intensity = jdl["intensity"].get<float>();
      }
    }
  }

  if (!root.contains("entities") || !root["entities"].is_array()) {
    if (outError) *outError = "Missing/invalid 'entities' array";
    return false;
  }

  for (const json& ent : root["entities"]) {
    if (!ent.contains("id") || !ent["id"].is_number_unsigned()) {
      continue;
    }
    const uint32_t id = ent["id"].get<uint32_t>();
    const std::string name = (ent.contains("name") && ent["name"].is_string()) ? ent["name"].get<std::string>() : "Entity";

    Entity& e = scene.CreateEntityWithId(id, name);

    if (ent.contains("transform")) {
      const json& t = ent["transform"];
      if (t.contains("position")) (void)JsonToVec3(t["position"], e.transform.position, outError);
      if (t.contains("rotation_deg")) (void)JsonToVec3(t["rotation_deg"], e.transform.rotationDeg, outError);
      if (t.contains("scale")) (void)JsonToVec3(t["scale"], e.transform.scale, outError);
    }

    if (ent.contains("components")) {
      const json& comps = ent["components"];

      if (comps.contains("material")) {
        const json& m = comps["material"];
        MaterialComponent* mat = scene.GetMaterial(id);
        if (!mat) {
          mat = &scene.AddMaterial(id);
        }
        if (m.contains("albedo")) (void)JsonToVec3(m["albedo"], mat->material.albedo, outError);
        if (m.contains("specular_color")) (void)JsonToVec3(m["specular_color"], mat->material.specularColor, outError);
        if (m.contains("specular_strength") && m["specular_strength"].is_number()) {
          mat->material.specularStrength = m["specular_strength"].get<float>();
        }
        if (m.contains("shininess") && m["shininess"].is_number()) {
          mat->material.shininess = m["shininess"].get<float>();
        }
      }

      if (comps.contains("point_light")) {
        const json& p = comps["point_light"];
        PointLightComponent& pl = scene.AddPointLight(id);
        if (p.contains("color")) (void)JsonToVec3(p["color"], pl.color, outError);
        if (p.contains("intensity") && p["intensity"].is_number()) {
          pl.intensity = p["intensity"].get<float>();
        }
        if (p.contains("radius") && p["radius"].is_number()) {
          pl.radius = p["radius"].get<float>();
        }
      }

      if (comps.contains("script")) {
        const json& s = comps["script"];
        ScriptComponent& sc = scene.AddScript(id);
        if (s.contains("path") && s["path"].is_string()) {
          sc.ScriptPath = s["path"].get<std::string>();
        }
      }
    }
  }

  return true;
}

} // namespace Aether
