#pragma once
#include <glm.hpp>
#include <entt.hpp>
#include "ComTransform.h"
#include "ComPhysics.h"
#include "OBB.h"

class PhysicsSystem
{
private:
    // --- 2D ---
    float m_Gravity2D = 0.8f;
    const float CELL_SIZE_2D = 64 * 2.0f;
    std::unordered_map<int64_t, std::vector<entt::entity>> m_Grid2D;

    // --- 3D ---
    float m_Gravity3D = 0.8f;
    const float CELL_SIZE_3D = 64 * 2.0f;
    std::unordered_map<int64_t, std::vector<entt::entity>> m_Grid3D;
    static constexpr float GROUNDED_THRESHOLD = 0.7f;

public:
    static PhysicsSystem* Get();

    PhysicsSystem(const PhysicsSystem&) = delete;
    PhysicsSystem& operator=(const PhysicsSystem&) = delete;

    void Update2D(entt::registry& registry, float deltaTime);
    void Update3D(entt::registry& registry, float deltaTime);

    void SetGravity2D(float g) { m_Gravity2D = g; }
    void SetGravity3D(float g) { m_Gravity3D = g; }
private:
  
    PhysicsSystem() = default;
    ~PhysicsSystem() = default;

	// --- 2D ---
    void __ResolveCollisions2D(entt::registry& registry,entt::entity movingEntity, Transform2DComponent& movingTransform,Physics2DComponent& movingPhys,glm::vec2& predictedPos,
        entt::entity otherEntity,const Transform2DComponent& otherTransform, const Physics2DComponent& otherPhys);
    bool __CheckAABB2D(const glm::vec2& posA, const glm::vec2& sizeA, const glm::vec2& posB, const glm::vec2& sizeB);
    int64_t __GetGridKey2D(float x, float y);
    void __InsertToGrid2D(entt::entity entity, const Transform2DComponent& transform, const Physics2DComponent&
        physics);
    void __RebuildGrid2D(entt::registry& registry);

	// --- 3D ---
    void __ResolveCollisions3D(entt::registry& registry,
        entt::entity entityA, Transform3DComponent& transformA, Physics3DComponent& physA,
        const OBB& obbA, glm::vec3& predictedPosA,
        entt::entity entityB, Transform3DComponent& transformB, Physics3DComponent& physB,
        const OBB& obbB, const SATResult& satResult);
    
    int64_t __GetGridKey3D(float x, float y, float z);
    void __InsertToGrid3D(entt::entity entity, const OBB& obb);
    void __RebuildGrid3D(entt::registry& registry, std::unordered_map<entt::entity, OBB>& outOBBs);
};