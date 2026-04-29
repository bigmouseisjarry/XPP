#include "PhysicsSystem.h"
#include <iostream>
#include "ComCamera.h"

PhysicsSystem* PhysicsSystem::Get()
{
    static PhysicsSystem instance;
    return &instance;
}

void PhysicsSystem::Update2D(entt::registry& registry, float deltaTime)
{
    __RebuildGrid2D(registry);

    // 只遍历动态物体
    auto dynamicView = registry.view<Transform2DComponent, Physics2DComponent>(
        entt::exclude<Camera2DComponent>);  // 排除相机实体（如果有 Physics2D 但不是物理体的情况）

    for (auto [entity, transform, physics] : dynamicView.each())
    {

        if (physics.isStatic) continue;  // 跳过静态体

        physics.isGrounded = false;

        // 应用重力
        glm::vec2& vel = physics.velocity;
        // if (!physics.isGrounded) vel.y -= m_Gravity2D;
		vel.y -= m_Gravity2D;


        glm::vec2 predictedPos = transform.position + vel * deltaTime;

        // 遍历网格
        bool collided = false;
        for (int dx = -1; dx <= 1 && !collided; dx++) {
            for (int dy = -1; dy <= 1 && !collided; dy++) {
                int64_t key = __GetGridKey2D(predictedPos.x + dx * CELL_SIZE_2D, predictedPos.y + dy * CELL_SIZE_2D);
                auto it = m_Grid2D.find(key);
                if (it == m_Grid2D.end()) continue;

                for (auto otherEntity : it->second) {
                    if (otherEntity == entity) continue;

                    auto& otherTransform = registry.get<Transform2DComponent>(otherEntity);
                    auto& otherPhys = registry.get<Physics2DComponent>(otherEntity);

                    if (!physics.isCollisionEnabled || !otherPhys.isCollisionEnabled)
                        continue;

                    if (__CheckAABB2D(predictedPos, physics.colliderSize,
                        otherTransform.position, otherPhys.colliderSize))
                    {
						__ResolveCollisions2D(registry, entity, transform, physics, predictedPos,
                            otherEntity, otherTransform, otherPhys);
                        collided = true;
                        break;
                    }
                }
            }
        }

        if (!collided) {
            transform.SetPosition(predictedPos);
        }
    }
}

void PhysicsSystem::__ResolveCollisions2D(entt::registry& registry, entt::entity movingEntity, Transform2DComponent& movingTransform, Physics2DComponent& movingPhys, 
    glm::vec2& predictedPos, entt::entity otherEntity, const Transform2DComponent& otherTransform, const Physics2DComponent& otherPhys)
{
    glm::vec2 vel = movingPhys.velocity;
    glm::vec2 otherPos = otherTransform.position;
    glm::vec2 otherSize = otherPhys.colliderSize;
    glm::vec2 mySize = movingPhys.colliderSize;

    if (otherPhys.isStatic)
    {
        if (vel.y < 0)
        {
            movingPhys.isGrounded = true;
            vel.y = 0;
            predictedPos.y = otherPos.y + otherSize.y / 2.0f + mySize.y / 2.0f;
        }
        else
        {
            vel.y = 0;
        }

        vel.x = 0;
    }

    movingPhys.velocity = vel;
    movingTransform.SetPosition(predictedPos);
}

bool PhysicsSystem::__CheckAABB2D(const glm::vec2& posA, const glm::vec2& sizeA, const glm::vec2& posB, const glm::vec2& sizeB)
{
    return (std::abs(posA.x - posB.x) * 2 < (sizeA.x + sizeB.x)) &&
        (std::abs(posA.y - posB.y) * 2 < (sizeA.y + sizeB.y));
}

int64_t PhysicsSystem::__GetGridKey2D(float x, float y)
{
    int gx = static_cast<int>(std::floor(x / CELL_SIZE_2D));
    int gy = static_cast<int>(std::floor(y / CELL_SIZE_2D));
    return (static_cast<int64_t>(gx) << 32) | static_cast<uint32_t>(gy);
}

void PhysicsSystem::__InsertToGrid2D(entt::entity entity, const Transform2DComponent& transform, const Physics2DComponent& physics)
{
    glm::vec2 pos = transform.position;
    glm::vec2 size = physics.colliderSize;

    float minX = pos.x - size.x / 2.0f;
    float maxX = pos.x + size.x / 2.0f;
    float minY = pos.y - size.y / 2.0f;
    float maxY = pos.y + size.y / 2.0f;

    int gxMin = static_cast<int>(std::floor(minX / CELL_SIZE_2D));
    int gxMax = static_cast<int>(std::floor(maxX / CELL_SIZE_2D));
    int gyMin = static_cast<int>(std::floor(minY / CELL_SIZE_2D));
    int gyMax = static_cast<int>(std::floor(maxY / CELL_SIZE_2D));

    for (int gx = gxMin; gx <= gxMax; gx++) {
        for (int gy = gyMin; gy <= gyMax; gy++) {
            int64_t key = (static_cast<int64_t>(gx) << 32) | static_cast<uint32_t>(gy);
            m_Grid2D[key].push_back(entity);
        }
    }
}

void PhysicsSystem::__RebuildGrid2D(entt::registry& registry)
{
    m_Grid2D.clear();

    auto view = registry.view<Transform2DComponent, Physics2DComponent>();
    for (auto [entity, transform, physics] : view.each())
    {
        __InsertToGrid2D(entity, transform, physics);
    }
}


// 扫掠检测（Swept OBB / CCD）
void PhysicsSystem::Update3D(entt::registry& registry, float deltaTime)
{
    std::unordered_map<entt::entity, OBB> obbCache;
    __RebuildGrid3D(registry, obbCache);

	// 只遍历动态物体
    auto dynamicView = registry.view<Transform3DComponent, Physics3DComponent>(
        entt::exclude<Camera3DComponent>);

    for (auto [entity, transform, physics] : dynamicView.each())
    {
        if (physics.isStatic) continue;

        physics.isGrounded = false;

        glm::vec3& vel = physics.velocity;
        if (!physics.isGrounded) vel.z -= m_Gravity3D;

        glm::vec3 predictedPos = transform.position + vel * deltaTime;
        OBB predictedOBB = ComputeOBB(predictedPos, transform.scale,
            transform.rotation, physics.colliderSize, physics.colliderOffset);

        bool collided = false;
        for (int dx = -1; dx <= 1 && !collided; dx++) {
            for (int dy = -1; dy <= 1 && !collided; dy++) {
                for (int dz = -1; dz <= 1 && !collided; dz++) {
                    int64_t key = __GetGridKey3D(
                        predictedPos.x + dx * CELL_SIZE_3D,
                        predictedPos.y + dy * CELL_SIZE_3D,
                        predictedPos.z + dz * CELL_SIZE_3D);
                    auto it = m_Grid3D.find(key);
                    if (it == m_Grid3D.end()) continue;

                    for (auto otherEntity : it->second) {
                        if (otherEntity == entity) continue;

                        auto& otherPhys = registry.get<Physics3DComponent>(otherEntity);

                        if (!physics.isCollisionEnabled || !otherPhys.isCollisionEnabled)
                            continue;

                        auto obbIt = obbCache.find(otherEntity);
                        if (obbIt == obbCache.end()) continue;

                        SATResult satResult = TestOBBs(predictedOBB, obbIt->second);
                        if (satResult.overlapping)
                        {
                            auto& otherTransform = registry.get<Transform3DComponent>(otherEntity);
                            __ResolveCollisions3D(registry,
                                entity, transform, physics, predictedOBB, predictedPos,
                                otherEntity, otherTransform, otherPhys, obbIt->second, satResult);
                            collided = true;
							break;          // TODO: 这里简单地处理为只响应第一个碰撞，后续可以改进为同时响应多个碰撞
                        }
                    }
                }
            }
        }

        if (!collided) {
            transform.SetPosition(predictedPos);
        }
    }
}


void PhysicsSystem::__ResolveCollisions3D(entt::registry& registry,
    entt::entity entityA, Transform3DComponent& transformA, Physics3DComponent& physA,
    const OBB& obbA, glm::vec3& predictedPosA,
    entt::entity entityB, Transform3DComponent& transformB, Physics3DComponent& physB,
    const OBB& obbB, const SATResult& satResult)
{
	glm::vec3 pushDir = satResult.axis;
	float depth = satResult.penetrationDepth;

    if (physB.isStatic)
    {
        predictedPosA -= pushDir * depth;
        transformA.SetPosition(predictedPosA);

        // 去掉 A 在“碰撞方向”的速度分量
        float velAlongNormal = glm::dot(physA.velocity, pushDir);
        if (velAlongNormal > 0.0f)
            physA.velocity -= pushDir * velAlongNormal;
		// 如果推力有向上的分量（大于设定的阈值），认为 A 着地了
        glm::vec3 pushA = -pushDir;
        if (pushA.z > GROUNDED_THRESHOLD)
            physA.isGrounded = true;
    }
    else
    {
        float halfDepth = depth * 0.5f;

        predictedPosA -= pushDir * halfDepth;
        transformA.SetPosition(predictedPosA);

        transformB.Move(pushDir * halfDepth);

        float velAAlongNormal = glm::dot(physA.velocity, pushDir);
        float velBAlongNormal = glm::dot(physB.velocity, pushDir);

        if (velAAlongNormal > 0.0f)
            physA.velocity -= pushDir * velAAlongNormal;
        if (velBAlongNormal < 0.0f)
            physB.velocity -= pushDir * velBAlongNormal;

        glm::vec3 pushA = -pushDir;
        glm::vec3 pushB = pushDir;
        if (pushA.z > GROUNDED_THRESHOLD) physA.isGrounded = true;
        if (pushB.z > GROUNDED_THRESHOLD) physB.isGrounded = true;
    }
}

int64_t PhysicsSystem::__GetGridKey3D(float x, float y, float z)
{
    int gx = static_cast<int>(std::floor(x / CELL_SIZE_3D));
    int gy = static_cast<int>(std::floor(y / CELL_SIZE_3D));
    int gz = static_cast<int>(std::floor(z / CELL_SIZE_3D));
    return static_cast<int64_t>(gx) * 73856093ll
        ^ static_cast<int64_t>(gy) * 19349669ll
        ^ static_cast<int64_t>(gz) * 83492791ll;
}

void PhysicsSystem::__InsertToGrid3D(entt::entity entity, const OBB& obb)
{
    glm::vec3 aabbMin, aabbMax;
    ComputeOBBAABB(obb, aabbMin, aabbMax);

    int gxMin = static_cast<int>(std::floor(aabbMin.x / CELL_SIZE_3D));
    int gxMax = static_cast<int>(std::floor(aabbMax.x / CELL_SIZE_3D));
    int gyMin = static_cast<int>(std::floor(aabbMin.y / CELL_SIZE_3D));
    int gyMax = static_cast<int>(std::floor(aabbMax.y / CELL_SIZE_3D));
    int gzMin = static_cast<int>(std::floor(aabbMin.z / CELL_SIZE_3D));
    int gzMax = static_cast<int>(std::floor(aabbMax.z / CELL_SIZE_3D));

    for (int gx = gxMin; gx <= gxMax; gx++)
        for (int gy = gyMin; gy <= gyMax; gy++)
            for (int gz = gzMin; gz <= gzMax; gz++)
            {
                int64_t key = static_cast<int64_t>(gx) * 73856093ll
                    ^ static_cast<int64_t>(gy) * 19349669ll
                    ^ static_cast<int64_t>(gz) * 83492791ll;
                m_Grid3D[key].push_back(entity);
            }
}

void PhysicsSystem::__RebuildGrid3D(entt::registry& registry,std::unordered_map<entt::entity, OBB>& outOBBs)
{
    m_Grid3D.clear();
    outOBBs.clear();

    auto view = registry.view<Transform3DComponent, Physics3DComponent>();
    for (auto [entity, transform, physics] : view.each())
    {
        OBB obb = ComputeOBB(transform.position, transform.scale,
            transform.rotation, physics.colliderSize, physics.colliderOffset);
        outOBBs[entity] = obb;
        __InsertToGrid3D(entity, obb);
    }
}
