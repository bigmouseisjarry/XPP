#pragma once
#include <vector>
#include <glm.hpp>

struct RenderUnit;
struct Light3DComponent;
struct InstancedRenderUnit;
class UniformBuffer;

namespace DrawUtils
{
    void DrawUnits(std::vector<RenderUnit>& units, bool transparent, const glm::mat4& projView, const glm::vec3& viewPos, const std::vector<Light3DComponent*>& lights,
        UniformBuffer& lightUBO, UniformBuffer& perFrameUBO, UniformBuffer& perObjectUBO);

    void DrawInstancedUnits(std::vector<InstancedRenderUnit>& units, const glm::mat4& projView, const glm::vec3& viewPos, UniformBuffer& perFrameUBO);
}