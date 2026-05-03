#pragma once
#include <glm.hpp>
#include <entt.hpp>
#include <unordered_set>
#include "ComTransform.h"
#include "ComPhysics.h"
#include "OBB.h"

// 射线检测命中结果
struct RaycastHit {
    entt::entity entity = entt::null;
    float distance = FLT_MAX;
    glm::vec3 point = { 0,0,0 };
    glm::vec3 normal = { 0,0,0 };
};

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
    static constexpr int MAX_COLLISIONS_PER_FRAME = 8;

    std::unordered_map<entt::entity, OBB> m_OBBCache3D;

    // TUDO: 目前先写在这里测试，日后移出
    struct TriggerOverlap {
        entt::entity trigger;
        entt::entity other;
    };
    std::vector<TriggerOverlap> m_CurrentOverlaps;          // 这一帧的重叠集合
    std::vector<TriggerOverlap> m_PreviousOverlaps;         // 上一帧的重叠集合

public:
    static PhysicsSystem* Get();

    PhysicsSystem(const PhysicsSystem&) = delete;
    PhysicsSystem& operator=(const PhysicsSystem&) = delete;

    void Update2D(entt::registry& registry, float deltaTime);
    void Update3D(entt::registry& registry, float deltaTime);

    void SetGravity2D(float g) { m_Gravity2D = g; }
    void SetGravity3D(float g) { m_Gravity3D = g; }

    const OBB* GetOBB(entt::entity entity) const;

    std::vector<entt::entity> GetTriggerEnter(entt::entity trigger) const;
    std::vector<entt::entity> GetTriggerStay(entt::entity trigger) const;
    std::vector<entt::entity> GetTriggerExit(entt::entity trigger) const;

    RaycastHit Raycast(entt::registry& registry, const glm::vec3& origin, const glm::vec3& direction, float maxDistance = FLT_MAX, uint32_t layerMask = 0xFFFFFFFF);
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
    
    int64_t __GetGridKey3D(float x, float y, float z) const;
    void __InsertToGrid3D(entt::entity entity, const OBB& obb);
    void __RebuildGrid3D(entt::registry& registry);
};