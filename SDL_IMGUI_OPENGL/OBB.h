#pragma once
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

struct OBB {
    glm::vec3 center;       // 世界空间中心
    glm::vec3 halfExtents;  // 沿本地轴的半尺寸
    glm::mat3 orientation;  // 列向量 = 本地轴在世界空间的方向
};

struct SATResult {
	bool overlapping = false;             // 是否重叠 
	float penetrationDepth = 0.0f;        // 最小穿透深度  
    glm::vec3 axis = { 0.0f,0.0f,0.0f };  // 从 A 指向 B 的分离轴
};

// 计算旋转矩阵
inline glm::mat3 ComputeRotationMatrix(const glm::vec3& rotationDegrees)
{
    glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), glm::radians(rotationDegrees.y), glm::vec3(0, 1, 0));
    glm::mat4 Rx = glm::rotate(glm::mat4(1.0f), glm::radians(rotationDegrees.x), glm::vec3(1, 0, 0));
    glm::mat4 Rz = glm::rotate(glm::mat4(1.0f), glm::radians(rotationDegrees.z), glm::vec3(0, 0, 1));
    return glm::mat3(Ry * Rx * Rz);
}

// 计算出世界空间下的 OBB
inline OBB ComputeOBB(const glm::vec3& position, const glm::vec3& scale,const glm::vec3& rotation, const glm::vec3& colliderSize,const glm::vec3& colliderOffset)
{
    OBB obb;
    glm::mat3 R = ComputeRotationMatrix(rotation);
    obb.orientation = R;
    obb.halfExtents = colliderSize * scale * 0.5f;
    obb.center = position + R * (colliderOffset * scale);
    return obb;
}

// 将 OBB 投影到轴上，得到最小值和最大值
inline void ProjectOBB(const OBB& obb, const glm::vec3& axis, float& outMin, float& outMax)
{
    float centerProj = glm::dot(obb.center, axis);
    float radius = 0.0f;
    for (int i = 0; i < 3; i++)
        radius += std::abs(glm::dot(obb.orientation[i], axis)) * obb.halfExtents[i];
    outMin = centerProj - radius;
    outMax = centerProj + radius;
}

// 使用分离轴定理测试两个 OBB 是否重叠，并计算穿透深度和分离轴
inline SATResult TestOBBs(const OBB& a, const OBB& b)
{
    SATResult result;
    result.overlapping = true;
    float minPenetration = FLT_MAX;
    glm::vec3 bestAxis = { 0.0f, 0.0f, 0.0f };

    glm::vec3 axes[15];
    axes[0] = a.orientation[0];
    axes[1] = a.orientation[1];
    axes[2] = a.orientation[2];
    axes[3] = b.orientation[0];
    axes[4] = b.orientation[1];
    axes[5] = b.orientation[2];

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            axes[6 + i * 3 + j] = glm::cross(a.orientation[i], b.orientation[j]);

    for (int i = 0; i < 15; i++)
    {
        glm::vec3 axis = axes[i];
        float len = glm::length(axis);
        if (len < 1e-6f) continue;
        axis /= len;

        float minA, maxA, minB, maxB;
        ProjectOBB(a, axis, minA, maxA);
        ProjectOBB(b, axis, minB, maxB);

        float overlap = std::min(maxA, maxB) - std::max(minA, minB);
        if (overlap <= 0.0f)
        {
            result.overlapping = false;
            return result;
        }

        if (overlap < minPenetration)
        {
            minPenetration = overlap;
            bestAxis = axis;
        }
    }

    if (glm::dot(b.center - a.center, bestAxis) < 0.0f)
        bestAxis = -bestAxis;

    result.penetrationDepth = minPenetration;
    result.axis = bestAxis;
    return result;
}

// 计算 OBB 的 AABB 包围盒
inline void ComputeOBBAABB(const OBB& obb, glm::vec3& outMin, glm::vec3& outMax)
{
    glm::vec3 corners[8];
    for (int i = 0; i < 8; i++)
    {
        float sx = (i & 1) ? 1.0f : -1.0f;
        float sy = (i & 2) ? 1.0f : -1.0f;
        float sz = (i & 4) ? 1.0f : -1.0f;
        corners[i] = obb.center
            + obb.orientation[0] * obb.halfExtents[0] * sx
            + obb.orientation[1] * obb.halfExtents[1] * sy
            + obb.orientation[2] * obb.halfExtents[2] * sz;
    }
    outMin = corners[0];
    outMax = corners[0];
    for (int i = 1; i < 8; i++)
    {
        outMin = glm::min(outMin, corners[i]);
        outMax = glm::max(outMax, corners[i]);
    }
}

// 射线与 OBB 相交测试（Slab 算法）
inline bool IntersectRayOBB(
    const glm::vec3& rayOrigin,
    const glm::vec3& rayDir,   // 必须归一化
    const OBB& obb,
    float& outT)
{
    float tMin = -FLT_MAX;
    float tMax = FLT_MAX;
    glm::vec3 delta = obb.center - rayOrigin;

    for (int i = 0; i < 3; i++)
    {
        glm::vec3 axis = obb.orientation[i];
        float e = glm::dot(delta, axis);
        float f = glm::dot(rayDir, axis);

        if (std::abs(f) < 1e-6f) {
            // 射线平行于该 slab
            if (e - obb.halfExtents[i] > 0.0f || e + obb.halfExtents[i] < 0.0f)
                return false;
        }
        else {
            float t1 = (e - obb.halfExtents[i]) / f;
            float t2 = (e + obb.halfExtents[i]) / f;
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            if (tMin > tMax) return false;
        }
    }

    outT = (tMin >= 0.0f) ? tMin : tMax;
    return outT >= 0.0f;
}
