#pragma once
#include <glm.hpp>
#include "ResourceManager.h"
#include "UniformBuffer.h"

enum class LightType : int
{
    Directional = 0,
    Point = 1,
    Spot = 2
};

struct Light3DComponent {
    glm::vec3 color = { 1, 1, 1 };
    float intensity = 1.0f;
    LightType type = LightType::Directional;
    bool castShadow = true;
    int  shadowLayerIndex = -1;  // 在阴影数组中的层索引，-1 = 未分配

    glm::vec3 target = { 0, 0, 0 };  // 光源看向的目标点
    float     range = 50.0f;
    float     innerCone = 12.5f;         // 度
    float     outerCone = 30.0f;         // 度

    glm::mat4     lightSpaceMatrix = glm::mat4(1.0f);
    const glm::mat4& GetLightSpaceMatrix() const { return lightSpaceMatrix; }
};