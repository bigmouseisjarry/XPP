#pragma once
#include <entt.hpp>
#include "ComTransform.h"
#include "ComPhysics.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "Material.h"
#include <gtc/matrix_transform.hpp>
#include "PhysicsSystem.h"

namespace DebugRenderSystem
{
    inline bool s_ShowColliders = false;
    inline MeshID s_ColliderMeshID{ INVALID_ID };

    inline bool s_ShowLightRanges = false;
    inline MeshID s_LightFrustumMeshID{ INVALID_ID };

    inline void Init()
    {
        s_ColliderMeshID = ResourceManager::Get()->GetMeshID("DebugColliderBox");
        s_LightFrustumMeshID = ResourceManager::Get()->GetMeshID("DebugLightFrustum");
    }

    inline void SubmitColliderBoxes(entt::registry& registry)
    {
        if (!s_ShowColliders) return;
        if (s_ColliderMeshID.value == INVALID_ID) return;

        static Material debugMat = [] {
            Material m;
            m.m_Shader = ResourceManager::Get()->GetShaderID("ShaderLine.glsl");
			m.SetTexture(TextureSemantic::Albedo, ResourceManager::Get()->GetTextureID("resources/white.png"));
            m.Set("u_Color", glm::vec4(1.0f));
            m.m_Transparent = true;
            return m;
            }();

        auto view = registry.view<Physics3DComponent>();
        for (auto [entity, physics] : view.each())
        {
            if (!physics.isCollisionEnabled) continue;

            const OBB* obb = PhysicsSystem::Get()->GetOBB(entity);
            if (!obb) continue;

            glm::mat4 model = glm::translate(glm::mat4(1.0f), obb->center);
            model = model * glm::mat4(obb->orientation);
            model = glm::scale(model, obb->halfExtents * 2.0f);

            Renderer::Get()->SubmitRenderUnits(s_ColliderMeshID, &debugMat, model, RenderLayer::RQ_WorldObjects);
        }
    }

    inline void SubmitLightRanges(entt::registry& registry) {
        if (!s_ShowLightRanges) return;
        if (s_LightFrustumMeshID.value == INVALID_ID) return;

        static Material lightMat = [] {
            Material m;
            m.m_Shader = ResourceManager::Get()->GetShaderID("ShaderLine.glsl");
            m.SetTexture(TextureSemantic::Albedo,
                ResourceManager::Get()->GetTextureID("resources/white.png"));
            m.Set("u_Color", glm::vec4(1.0f));
            m.m_Transparent = true;
            return m;
            }();

        auto view = registry.view<Transform3DComponent, Light3DComponent>();
        for (auto [entity, transform, light] : view.each()) {
            glm::vec3 pos = transform.position;
            glm::vec3 dir = glm::normalize(light.target - transform.position);

            // 构建旋转：局部 -Z → 世界 dir
            glm::vec3 z = -dir;
            glm::vec3 x = glm::normalize(glm::cross(glm::vec3(0, 1, 0), z));
            if (glm::length(x) < 0.001f)
                x = glm::normalize(glm::cross(glm::vec3(1, 0, 0), z));
            glm::vec3 y = glm::cross(z, x);

            glm::mat4 rot(1.0f);
            rot[0] = glm::vec4(x, 0);
            rot[1] = glm::vec4(y, 0);
            rot[2] = glm::vec4(z, 0);

            float scale = light.intensity * 2.0f;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos)
                * rot
                * glm::scale(glm::mat4(1.0f), glm::vec3(scale));

            Renderer::Get()->SubmitRenderUnits(
                s_LightFrustumMeshID, &lightMat, model,
                RenderLayer::RQ_WorldObjects);
        }
    }
};