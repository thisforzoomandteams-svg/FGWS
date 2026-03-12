#include "ECS/Scene.h"

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <limits>

namespace Aether {

void Scene::Clear() {
  m_entities.clear();
  m_materials.clear();
  m_pointLights.clear();
  m_scripts.clear();

  m_directionalLight = DirectionalLight{};
  m_ambientColor = {0.05f, 0.05f, 0.05f};
  m_ambientIntensity = 1.0f;

  m_nextId = 1;
}

Entity& Scene::CreateEntity(std::string name) {
  return CreateEntityWithId(m_nextId, std::move(name));
}

Entity& Scene::CreateEntityWithId(uint32_t id, std::string name) {
  if (id == 0) {
    id = m_nextId;
  }

  const uint32_t entityId = id;

  Entity entity;
  entity.id = entityId;
  entity.name = std::move(name);
  m_entities.push_back(std::move(entity));

  if (entityId >= m_nextId) {
    m_nextId = entityId + 1;
  }

  // All entities are renderable cubes in this prototype, so give them a material by default.
  (void)m_materials.emplace(entityId, MaterialComponent{});

  return m_entities.back();
}

Entity* Scene::FindById(uint32_t id) {
  for (Entity& e : m_entities) {
    if (e.id == id) return &e;
  }
  return nullptr;
}

const Entity* Scene::FindById(uint32_t id) const {
  for (const Entity& e : m_entities) {
    if (e.id == id) return &e;
  }
  return nullptr;
}

Entity* Scene::FindByName(const std::string& name) {
  for (Entity& e : m_entities) {
    if (e.name == name) return &e;
  }
  return nullptr;
}

const Entity* Scene::FindByName(const std::string& name) const {
  for (const Entity& e : m_entities) {
    if (e.name == name) return &e;
  }
  return nullptr;
}

bool Scene::HasMaterial(uint32_t entityId) const {
  return m_materials.find(entityId) != m_materials.end();
}

MaterialComponent& Scene::AddMaterial(uint32_t entityId, const MaterialComponent& component) {
  m_materials[entityId] = component;
  return m_materials[entityId];
}

MaterialComponent* Scene::GetMaterial(uint32_t entityId) {
  auto it = m_materials.find(entityId);
  if (it == m_materials.end()) {
    return nullptr;
  }
  return &it->second;
}

const MaterialComponent* Scene::GetMaterial(uint32_t entityId) const {
  auto it = m_materials.find(entityId);
  if (it == m_materials.end()) {
    return nullptr;
  }
  return &it->second;
}

void Scene::RemoveMaterial(uint32_t entityId) {
  m_materials.erase(entityId);
}

bool Scene::HasPointLight(uint32_t entityId) const {
  return m_pointLights.find(entityId) != m_pointLights.end();
}

PointLightComponent& Scene::AddPointLight(uint32_t entityId, const PointLightComponent& component) {
  m_pointLights[entityId] = component;
  return m_pointLights[entityId];
}

PointLightComponent* Scene::GetPointLight(uint32_t entityId) {
  auto it = m_pointLights.find(entityId);
  if (it == m_pointLights.end()) {
    return nullptr;
  }
  return &it->second;
}

const PointLightComponent* Scene::GetPointLight(uint32_t entityId) const {
  auto it = m_pointLights.find(entityId);
  if (it == m_pointLights.end()) {
    return nullptr;
  }
  return &it->second;
}

void Scene::RemovePointLight(uint32_t entityId) {
  m_pointLights.erase(entityId);
}

bool Scene::HasScript(uint32_t entityId) const {
  return m_scripts.find(entityId) != m_scripts.end();
}

ScriptComponent& Scene::AddScript(uint32_t entityId, const ScriptComponent& component) {
  m_scripts[entityId] = component;
  return m_scripts[entityId];
}

ScriptComponent* Scene::GetScript(uint32_t entityId) {
  auto it = m_scripts.find(entityId);
  if (it == m_scripts.end()) {
    return nullptr;
  }
  return &it->second;
}

const ScriptComponent* Scene::GetScript(uint32_t entityId) const {
  auto it = m_scripts.find(entityId);
  if (it == m_scripts.end()) {
    return nullptr;
  }
  return &it->second;
}

void Scene::RemoveScript(uint32_t entityId) {
  m_scripts.erase(entityId);
}

static bool RayAabbIntersectLocal(
  const glm::vec3& origin,
  const glm::vec3& dir,
  const glm::vec3& bmin,
  const glm::vec3& bmax,
  float& outT
) {
  float tMin = -std::numeric_limits<float>::infinity();
  float tMax = std::numeric_limits<float>::infinity();

  for (int axis = 0; axis < 3; ++axis) {
    const float o = origin[axis];
    const float d = dir[axis];
    const float minv = bmin[axis];
    const float maxv = bmax[axis];

    if (std::abs(d) < 1e-6f) {
      if (o < minv || o > maxv) {
        return false;
      }
      continue;
    }

    float t1 = (minv - o) / d;
    float t2 = (maxv - o) / d;
    if (t1 > t2) {
      std::swap(t1, t2);
    }

    tMin = (t1 > tMin) ? t1 : tMin;
    tMax = (t2 < tMax) ? t2 : tMax;

    if (tMin > tMax) {
      return false;
    }
  }

  if (tMax < 0.0f) {
    return false;
  }

  outT = (tMin >= 0.0f) ? tMin : tMax;
  return true;
}

std::optional<uint32_t> Scene::RaycastSingle(const glm::vec3& rayOrigin, const glm::vec3& rayDir) const {
  const glm::vec3 bmin(-0.5f);
  const glm::vec3 bmax(0.5f);

  float bestDist = std::numeric_limits<float>::infinity();
  std::optional<uint32_t> bestId;

  for (const Entity& e : m_entities) {
    const glm::mat4 model = e.GetTransformMatrix();
    const glm::mat4 invModel = glm::inverse(model);

    const glm::vec3 originLocal = glm::vec3(invModel * glm::vec4(rayOrigin, 1.0f));
    const glm::vec3 dirLocal = glm::normalize(glm::vec3(invModel * glm::vec4(rayDir, 0.0f)));

    float tLocal = 0.0f;
    if (!RayAabbIntersectLocal(originLocal, dirLocal, bmin, bmax, tLocal)) {
      continue;
    }

    const glm::vec3 hitLocal = originLocal + dirLocal * tLocal;
    const glm::vec3 hitWorld = glm::vec3(model * glm::vec4(hitLocal, 1.0f));
    const float distWorld = glm::length(hitWorld - rayOrigin);

    if (distWorld < bestDist) {
      bestDist = distWorld;
      bestId = e.id;
    }
  }

  return bestId;
}

} // namespace Aether
