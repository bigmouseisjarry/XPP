#pragma once
#pragma once
#include <entt.hpp>
#include <glm.hpp>
#include "ComParticle.h"

class ParticleSystem
{
public:
    static ParticleSystem* Get();

    void Update(entt::registry& registry, float deltaTime);
    void SubmitRender(entt::registry& registry);

    MeshID GetQuadMeshID() const { return m_QuadMeshID; }

private:
    ParticleSystem() = default;

    void InitResources();
    void Emit(ParticleEmitterComponent& emitter, int count);
    void UpdateEmitter(ParticleEmitterComponent& emitter, float deltaTime);

    glm::vec4 SampleColor(const std::vector<ParticleColorKey>& keys, float t);
    float     SampleSize(const std::vector<ParticleSizeKey>& keys, float t);
    glm::vec3 SamplePosition(EmitterShape shape, const glm::vec3& shapeSize, float innerRadius);
    glm::vec3 SampleDirection(const glm::vec3& dir, float angle);

    MeshID   m_QuadMeshID = { INVALID_ID };
    ShaderID m_ParticleShaderID = { INVALID_ID };
    bool     m_Initialized = false;
};