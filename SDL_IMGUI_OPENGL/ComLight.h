#pragma once
#include <glm.hpp>
#include "ResourceManager.h"
#include "UniformBuffer.h"

struct Light3DComponent {
    glm::vec3 position = { 0, 0, 0 };
    glm::vec3 color = { 1, 1, 1 };
    float intensity = 1.0f;
    bool castShadow = true;
    
    int  shadowLayerIndex = -1;  // 在阴影数组中的层索引，-1 = 未分配

    glm::vec3 target = { 0, 0, 0 };  // 光源看向的目标点

    glm::mat4     lightSpaceMatrix = glm::mat4(1.0f);

    void ComputeLightSpaceMatrix(const glm::mat4& camView, const glm::mat4& camProj);

    const glm::mat4& GetLightSpaceMatrix() const { return lightSpaceMatrix; }
};