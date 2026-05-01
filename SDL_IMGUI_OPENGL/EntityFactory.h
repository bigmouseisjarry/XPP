#pragma once
#include <entt.hpp>
#include <string>
#include <span>
#include "Renderer.h"

namespace EntityFactory {
    entt::entity CreatePlayer(entt::registry& registry);
    entt::entity CreateCamera2D(entt::registry& registry, float left, float right, float bottom, float top, const std::string& name, std::span<const RenderLayer> layers);
   
    entt::entity CreateCamera3D(entt::registry& registry, float fov, float aspect, float nearPlane, float farPlane, const std::string& name, std::span<const RenderLayer> layers);
    entt::entity CreateLight3D(entt::registry& registry, const glm::vec3& position, const glm::vec3& color = { 1,1,1 }, float intensity = 1.0f, bool castShadow = true);
    entt::entity CreateModel3D(entt::registry& registry, const std::string& filepath);
    entt::entity CreateCube(entt::registry& registry);
    entt::entity CreateAxis(entt::registry& registry);
    entt::entity CreateModelHierarchy(entt::registry& registry, const std::string& filepath);
}