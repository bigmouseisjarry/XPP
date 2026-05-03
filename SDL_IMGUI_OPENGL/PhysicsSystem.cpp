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

// 扫掠检测（Swept OBB / CCD）约束求解器（Constraint Solver）
void PhysicsSystem::Update3D(entt::registry& registry, float deltaTime)
{
    __RebuildGrid3D(registry);

    m_PreviousOverlaps = std::move(m_CurrentOverlaps);
    m_CurrentOverlaps.clear();

	// 只遍历动态物体
    auto dynamicView = registry.view<Transform3DComponent, Physics3DComponent>(
        entt::exclude<Camera3DComponent>);

    for (auto [entity, transform, physics] : dynamicView.each())
    {
        if (physics.isStatic) continue;

        bool wasGrounded = physics.isGrounded;
        physics.isGrounded = false;

        glm::vec3& vel = physics.velocity;
        if (!wasGrounded)
            vel.z -= m_Gravity3D;

        glm::vec3 predictedPos = transform.position + vel * deltaTime;
        OBB predictedOBB = ComputeOBB(predictedPos, transform.scale,
            transform.rotation, physics.colliderSize, physics.colliderOffset);

        // 收集所有重叠对
        struct CollisionInfo {
            entt::entity other;
            OBB otherOBB;
            SATResult sat;
            float depth;
        };
        std::vector<CollisionInfo> collisions;

        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                for (int dz = -1; dz <= 1; dz++) {
                    int64_t key = __GetGridKey3D(
                        predictedPos.x + dx * CELL_SIZE_3D,
                        predictedPos.y + dy * CELL_SIZE_3D,
                        predictedPos.z + dz * CELL_SIZE_3D);
                    auto it = m_Grid3D.find(key);
                    if (it == m_Grid3D.end()) continue;

                    for (auto otherEntity : it->second) {
                        if (otherEntity == entity) continue;

                        auto& otherPhys = registry.get<Physics3DComponent>(otherEntity);
                        if (!physics.isCollisionEnabled || !otherPhys.isCollisionEnabled)continue;
                        
                        if (!(physics.collisionMask & otherPhys.collisionLayer)) continue;
                        if (!(otherPhys.collisionMask & physics.collisionLayer)) continue;

                        auto obbIt = m_OBBCache3D.find(otherEntity);
                        if (obbIt == m_OBBCache3D.end()) continue;

                        SATResult satResult = TestOBBs(predictedOBB, obbIt->second);
                        if (satResult.overlapping) {
                            // 如果任一方是触发器，只记录重叠，不做物理解析
                            if (physics.isTrigger || otherPhys.isTrigger)
                            {
                                entt::entity trigger = physics.isTrigger ? entity : otherEntity;
                                entt::entity other = physics.isTrigger ? otherEntity : entity;
                                m_CurrentOverlaps.push_back({ trigger, other });
                                continue;
                            }
                            collisions.push_back({ otherEntity, obbIt->second, satResult, satResult.penetrationDepth });
                        }
                    }
                }
            }
        }

        // 按穿透深度降序排序（最深的先解析）
        std::sort(collisions.begin(), collisions.end(),
            [](const CollisionInfo& a, const CollisionInfo& b) { return a.depth > b.depth; });

        // 逐个解析
        int resolved = 0;
        for (auto& ci : collisions) {
            if (resolved >= MAX_COLLISIONS_PER_FRAME) break;

            // 重测：之前的推离可能已经解开了这个碰撞
            SATResult retest = TestOBBs(predictedOBB, ci.otherOBB);
            if (!retest.overlapping) continue;

            auto& otherTransform = registry.get<Transform3DComponent>(ci.other);
            auto& otherPhys = registry.get<Physics3DComponent>(ci.other);
            __ResolveCollisions3D(registry,
                entity, transform, physics, predictedOBB, predictedPos,
                ci.other, otherTransform, otherPhys, ci.otherOBB, retest);

            // 更新 predictedOBB
            predictedOBB = ComputeOBB(predictedPos, transform.scale,
                transform.rotation, physics.colliderSize, physics.colliderOffset);
            resolved++;
        }

        // 统一设置最终位置
        transform.SetPosition(predictedPos);
    }
}

void PhysicsSystem::__ResolveCollisions3D(entt::registry& registry,
    entt::entity entityA, Transform3DComponent& transformA, Physics3DComponent& physA,const OBB& obbA, glm::vec3& predictedPosA,
    entt::entity entityB, Transform3DComponent& transformB, Physics3DComponent& physB,const OBB& obbB, const SATResult& satResult)
{
    glm::vec3 pushDir = satResult.axis;
    float depth = satResult.penetrationDepth;

    if (physB.isStatic)
    {
        // 推离
        predictedPosA -= pushDir * depth;

        // 速度分解为法线分量和切线分量
        float velAlongNormal = glm::dot(physA.velocity, pushDir);
        glm::vec3 velNormal = pushDir * velAlongNormal;
        glm::vec3 velTangent = physA.velocity - velNormal;

        // 弹性：法线分量反弹
        float e = physA.restitution;
        glm::vec3 newVelNormal = (velAlongNormal > 0.0f)
            ? -e * velNormal
            : velNormal;

        // 摩擦：切线分量衰减
        float f = physA.friction;
        glm::vec3 newVelTangent = velTangent * (1.0f - f);

        physA.velocity = newVelNormal + newVelTangent;

        // 着地检测
        glm::vec3 pushA = -pushDir;
        if (pushA.z > GROUNDED_THRESHOLD)
            physA.isGrounded = true;
    }
    else
    {
        // 推离
        predictedPosA -= pushDir * depth;

        // 两者弹性取较小值
        float e = std::min(physA.restitution, physB.restitution);

        // A 的速度处理
        float velAAlongNormal = glm::dot(physA.velocity, pushDir);
        if (velAAlongNormal > 0.0f) {
            glm::vec3 velANormal = pushDir * velAAlongNormal;
            glm::vec3 velATangent = physA.velocity - velANormal;
            physA.velocity = -e * velANormal + velATangent * (1.0f - physA.friction);
        }

        // B 的速度处理
        float velBAlongNormal = glm::dot(physB.velocity, pushDir);
        if (velBAlongNormal < 0.0f) {
            glm::vec3 velBNormal = pushDir * velBAlongNormal;
            glm::vec3 velBTangent = physB.velocity - velBNormal;
            physB.velocity = -e * velBNormal + velBTangent * (1.0f - physB.friction);
        }

        // 着地检测
        glm::vec3 pushA = -pushDir;
        if (pushA.z > GROUNDED_THRESHOLD) physA.isGrounded = true;

    }
}

int64_t PhysicsSystem::__GetGridKey3D(float x, float y, float z) const
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

void PhysicsSystem::__RebuildGrid3D(entt::registry& registry)
{
    m_Grid3D.clear();
    m_OBBCache3D.clear();

    auto view = registry.view<Transform3DComponent, Physics3DComponent>();
    for (auto [entity, transform, physics] : view.each())
    {
        OBB obb = ComputeOBB(transform.position, transform.scale,
            transform.rotation, physics.colliderSize, physics.colliderOffset);
        m_OBBCache3D[entity] = obb;
        __InsertToGrid3D(entity, obb);
    }
}

std::vector<entt::entity> PhysicsSystem::GetTriggerEnter(entt::entity trigger) const
{
    std::vector<entt::entity> result;
    for (auto& cur : m_CurrentOverlaps) {
        if (cur.trigger != trigger) continue;
        bool wasInPrevious = false;
        for (auto& prev : m_PreviousOverlaps) {
            if (prev.trigger == trigger && prev.other == cur.other) {
                wasInPrevious = true;
                break;
            }
        }
        if (!wasInPrevious) result.push_back(cur.other);
    }
    return result;
}

std::vector<entt::entity> PhysicsSystem::GetTriggerStay(entt::entity trigger) const
{
    std::vector<entt::entity> result;
    for (auto& cur : m_CurrentOverlaps) {
        if (cur.trigger != trigger) continue;
        for (auto& prev : m_PreviousOverlaps) {
            if (prev.trigger == trigger && prev.other == cur.other) {
                result.push_back(cur.other);
                break;
            }
        }
    }
    return result;
}

std::vector<entt::entity> PhysicsSystem::GetTriggerExit(entt::entity trigger) const
{
    std::vector<entt::entity> result;
    for (auto& prev : m_PreviousOverlaps) {
        if (prev.trigger != trigger) continue;
        bool stillInCurrent = false;
        for (auto& cur : m_CurrentOverlaps) {
            if (cur.trigger == trigger && cur.other == prev.other) {
                stillInCurrent = true;
                break;
            }
        }
        if (!stillInCurrent) result.push_back(prev.other);
    }
    return result;
}

RaycastHit PhysicsSystem::Raycast(entt::registry& registry, const glm::vec3& origin, const glm::vec3& direction, float maxDistance, uint32_t layerMask)
{
    RaycastHit hit;
    glm::vec3 dir = glm::normalize(direction);
    float stepSize = CELL_SIZE_3D * 0.5f;
    int maxSteps = static_cast<int>(maxDistance / stepSize) + 1;
    maxSteps = std::min(maxSteps, 200);

    std::unordered_set<int64_t> visitedCells;

    for (int s = 0; s <= maxSteps; s++)
    {
        glm::vec3 samplePoint = origin + dir * (s * stepSize);

        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                for (int dz = -1; dz <= 1; dz++) {
                    int64_t key = __GetGridKey3D(
                        samplePoint.x + dx * CELL_SIZE_3D,
                        samplePoint.y + dy * CELL_SIZE_3D,
                        samplePoint.z + dz * CELL_SIZE_3D);
                    if (visitedCells.count(key)) continue;
                    visitedCells.insert(key);

                    auto it = m_Grid3D.find(key);
                    if (it == m_Grid3D.end()) continue;

                    for (auto entity : it->second) {
                        auto& phys = registry.get<Physics3DComponent>(entity);
                        if (!phys.isCollisionEnabled) continue;
                        if (!(layerMask & phys.collisionLayer)) continue;

                        auto obbIt = m_OBBCache3D.find(entity);
                        if (obbIt == m_OBBCache3D.end()) continue;

                        float t;
                        if (IntersectRayOBB(origin, dir, obbIt->second, t)) {
                            if (t < hit.distance && t <= maxDistance) {
                                hit.entity = entity;
                                hit.distance = t;
                                hit.point = origin + dir * t;

                                // 计算命中面法线：找 OBB 6 个面中与命中点最对齐的
                                glm::vec3 localDir = obbIt->second.center - hit.point;
                                float bestDot = -FLT_MAX;
                                for (int i = 0; i < 3; i++) {
                                    for (int sign = -1; sign <= 1; sign += 2) {
                                        float d = glm::dot(localDir, obbIt->second.orientation[i] * (float)sign);
                                        if (d > bestDot) {
                                            bestDot = d;
                                            hit.normal = -(obbIt->second.orientation[i] * (float)sign);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 已找到命中点且走过头了，提前退出
        if (hit.entity != entt::null && s * stepSize > hit.distance)
            break;
    }

    return hit;
}

const OBB* PhysicsSystem::GetOBB(entt::entity entity) const
{
    auto it = m_OBBCache3D.find(entity);
    return (it != m_OBBCache3D.end()) ? &it->second : nullptr;
}