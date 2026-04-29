#include "ComLight.h"
#include <ext/matrix_transform.hpp>
#include <ext/matrix_clip_space.hpp>

void Light3DComponent::ComputeLightSpaceMatrix(const glm::mat4& camView, const glm::mat4& camProj)
{
    glm::mat4 lightView = glm::lookAt(position, target, glm::vec3(0.0f, 0.0f, 1.0f));

    // 1. 相机视锥体 8 角 → 光空间包围盒
    glm::mat4 invVP = glm::inverse(camProj * camView);

    glm::vec4 ndcCorners[8] = {
        {-1, -1, -1, 1}, {1, -1, -1, 1}, {1, 1, -1, 1}, {-1, 1, -1, 1},
        {-1, -1,  1, 1}, {1, -1,  1, 1}, {1, 1,  1, 1}, {-1, 1,  1, 1},
    };

    float minX = FLT_MAX, maxX = -FLT_MAX;
    float minY = FLT_MAX, maxY = -FLT_MAX;
    float minZ = FLT_MAX, maxZ = -FLT_MAX;

    for (int i = 0; i < 8; i++)
    {
        glm::vec4 worldPt = invVP * ndcCorners[i];
        worldPt /= worldPt.w;
        glm::vec4 lightPt = lightView * worldPt;

        minX = std::min(minX, lightPt.x);
        maxX = std::max(maxX, lightPt.x);
        minY = std::min(minY, lightPt.y);
        maxY = std::max(maxY, lightPt.y);
        minZ = std::min(minZ, lightPt.z);
        maxZ = std::max(maxZ, lightPt.z);
    }

    float marginX = (maxX - minX) * 0.25f;
    float marginY = (maxY - minY) * 0.25f;
    minX -= marginX; maxX += marginX;
    minY -= marginY; maxY += marginY;

    float rangeZ = maxZ - minZ;
    minZ -= rangeZ * 0.5f;
    maxZ += rangeZ * 0.5f;

    float zNear = std::max(0.1f, -maxZ);
    float zFar = std::max(zNear + 0.1f, -minZ);

    // === 第二层：Texel Snapping 消除抖动 ===
    const float shadowRes = 2048.0f;

    float worldPerTexelX = (maxX - minX) / shadowRes;
    float worldPerTexelY = (maxY - minY) / shadowRes;

    minX = floor(minX / worldPerTexelX) * worldPerTexelX;
    maxX = floor(maxX / worldPerTexelX) * worldPerTexelX;
    minY = floor(minY / worldPerTexelY) * worldPerTexelY;
    maxY = floor(maxY / worldPerTexelY) * worldPerTexelY;

    glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, zNear, zFar);
    lightSpaceMatrix = lightProj * lightView;
}


