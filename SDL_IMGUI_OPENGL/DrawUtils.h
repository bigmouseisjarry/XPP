#pragma once
#include <vector>
#include <glm.hpp>

struct RenderUnit;
struct InstancedRenderUnit;
class UniformBuffer;
struct LightUnit;

namespace DrawUtils
{
    void DrawUnits(std::vector<RenderUnit>& units, bool transparent, const glm::mat4& projView, const glm::vec3& viewPos, const std::vector<LightUnit>& lights,
        UniformBuffer& lightUBO, UniformBuffer& perFrameUBO, UniformBuffer& perObjectUBO);

    void DrawInstancedUnits(std::vector<InstancedRenderUnit>& units, const glm::mat4& projView, const glm::vec3& viewPos, UniformBuffer& perFrameUBO);
}