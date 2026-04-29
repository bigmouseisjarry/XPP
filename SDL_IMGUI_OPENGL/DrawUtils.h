#pragma once
#include <vector>
#include <glm.hpp>

struct RenderUnit;
struct Light3DComponent;
class UniformBuffer;

namespace DrawUtils
{
    void DrawUnits(
        std::vector<RenderUnit>& units,
        bool transparent,
        const glm::mat4& projView,
        const glm::vec3& viewPos,
        const std::vector<Light3DComponent*>& lights,
        UniformBuffer& lightUBO,
        UniformBuffer& perFrameUBO,
        UniformBuffer& perObjectUBO
    );
}