#pragma once

#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/ScriptComponent.h"
#include "ECS/Entity.h"
#include "Rendering/Lights.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec3.hpp>

namespace Aether {

class Scene {
public:
  void Clear();

  Entity& CreateEntity(std::string name);
  Entity& CreateEntityWithId(uint32_t id, std::string name);

  std::vector<Entity>& GetEntities() { return m_entities; }
  const std::vector<Entity>& GetEntities() const { return m_entities; }

  Entity* FindById(uint32_t id);
  const Entity* FindById(uint32_t id) const;

  Entity* FindByName(const std::string& name);
  const Entity* FindByName(const std::string& name) const;

  bool HasMaterial(uint32_t entityId) const;
  MaterialComponent& AddMaterial(uint32_t entityId, const MaterialComponent& component = {});
  MaterialComponent* GetMaterial(uint32_t entityId);
  const MaterialComponent* GetMaterial(uint32_t entityId) const;
  void RemoveMaterial(uint32_t entityId);

  bool HasPointLight(uint32_t entityId) const;
  PointLightComponent& AddPointLight(uint32_t entityId, const PointLightComponent& component = {});
  PointLightComponent* GetPointLight(uint32_t entityId);
  const PointLightComponent* GetPointLight(uint32_t entityId) const;
  void RemovePointLight(uint32_t entityId);

  bool HasScript(uint32_t entityId) const;
  ScriptComponent& AddScript(uint32_t entityId, const ScriptComponent& component = {});
  ScriptComponent* GetScript(uint32_t entityId);
  const ScriptComponent* GetScript(uint32_t entityId) const;
  void RemoveScript(uint32_t entityId);

  std::unordered_map<uint32_t, ScriptComponent>& GetScripts() { return m_scripts; }
  const std::unordered_map<uint32_t, ScriptComponent>& GetScripts() const { return m_scripts; }

  DirectionalLight& GetDirectionalLight() { return m_directionalLight; }
  const DirectionalLight& GetDirectionalLight() const { return m_directionalLight; }

  glm::vec3& GetAmbientColor() { return m_ambientColor; }
  const glm::vec3& GetAmbientColor() const { return m_ambientColor; }
  float& GetAmbientIntensity() { return m_ambientIntensity; }
  float GetAmbientIntensity() const { return m_ambientIntensity; }

  std::optional<uint32_t> RaycastSingle(const glm::vec3& rayOrigin, const glm::vec3& rayDir) const;

private:
  std::vector<Entity> m_entities;
  std::unordered_map<uint32_t, MaterialComponent> m_materials;
  std::unordered_map<uint32_t, PointLightComponent> m_pointLights;
  std::unordered_map<uint32_t, ScriptComponent> m_scripts;

  DirectionalLight m_directionalLight{};
  glm::vec3 m_ambientColor{0.05f, 0.05f, 0.05f};
  float m_ambientIntensity = 1.0f;

  uint32_t m_nextId = 1;
};

} // namespace Aether
