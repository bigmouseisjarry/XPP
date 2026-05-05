#include "Renderer.h"
#include "RenderPass.h"
#include <iomanip>
#include <gtc/matrix_transform.hpp>
#include <algorithm>
#include "ShadowPass.h"
#include "OpaquePass3D.h"
#include "TransparentPass3D.h"
#include "OpaquePass2D.h"
#include "TransparentPass2D.h"
#include "PostProcessPass.h"
#include "SkyboxPass.h"

Renderer* Renderer::Get()
{
    static Renderer instance;
    return &instance;
}
// TODO: 这里的 UBO 初始化其实也可以放到 ResourceManager 里,让它来管理所有 GPU 资源,Renderer 只负责使用
// 或者干脆把 UBO 包装成一个 UniformBuffer 类, 由 Renderer 来管理和使用, ResourceManager 不涉及任何 GPU 资源的细节
void Renderer::InitUBO()            
{
    m_PerFrameUBO = std::make_unique<UniformBuffer>(sizeof(PerFrameData), static_cast<unsigned int>(UBOBinding::PerFrame));
    m_PerObjectUBO = std::make_unique<UniformBuffer>(sizeof(PerObjectData), static_cast<unsigned int>(UBOBinding::PerObject));
    m_LightUBO = std::make_unique<UniformBuffer>(sizeof(LightArrayData), static_cast<unsigned int>(UBOBinding::Lights));
}

void Renderer::Clear()
{
    m_Camera2DUnits.clear();
    m_Camera3DUnits.clear();
    m_InstancedUnits.clear();
    m_LightUnits.clear();
    for (auto& [layer, units] : m_RenderUnits)
        units.clear();

}
// 不同的pass有着不同的排序方式，以后演进
void Renderer::SortAll()
{
    for (auto& [layer, units] : m_RenderUnits)
    {
        std::sort(units.begin(), units.end(),
            [](const RenderUnit& a, const RenderUnit& b)
            {
                return a.SortKey() < b.SortKey();
            });
    }
}

void Renderer::Flush()
{
    SortAll();

    RenderContext ctx{ m_RenderUnits,m_Camera2DUnits, m_Camera3DUnits,m_LightUnits,m_InstancedUnits,
        *m_LightUBO,*m_PerFrameUBO,*m_PerObjectUBO,m_Viewport };
    
	m_Pipeline->Execute(ctx);

    Clear();
}


void Renderer::SetWindowSize(int w, int h)
{
    m_Viewport.width = w;
    m_Viewport.height = h;
    for (auto& [fboID, scale] : m_IntermediateFBOs)
    {
        Framebuffer* fbo = ResourceManager::Get()->GetFramebufferMut(fboID);
        if (fbo) fbo->Resize(static_cast<int>(w * scale.x), static_cast<int>(h * scale.y));
    }
}

void Renderer::RegisterIntermediateFBO(FramebufferID fboID, float scaleX, float scaleY)
{
    m_IntermediateFBOs.push_back({ fboID, glm::vec2(scaleX, scaleY) });
}


void Renderer::SubmitCameraUnits(const glm::mat4& projView, const std::vector<RenderLayer>& layers, RenderMode mode, const glm::vec3& viewPos)
{
    if(mode == RenderMode::Render2D)
        m_Camera2DUnits.emplace_back(projView, layers, viewPos);
	else
        m_Camera3DUnits.emplace_back(projView, layers, viewPos);
}

void Renderer::SubmitRenderUnits(const MeshID& mesh, const Material* material,const glm::mat4& model,RenderLayer renderlayer)
{
    m_RenderUnits[renderlayer].emplace_back(mesh, material, model);
}

void Renderer::SubmitLightUnits(const Light3DComponent& light, const glm::vec3& position)
{
    m_LightUnits.push_back({ position,glm::normalize(position - light.target), light.color, light.intensity, light.type,light.range,
        glm::radians(light.innerCone),glm::radians(light.outerCone),light.castShadow, -1,light.GetLightSpaceMatrix() });
}

void Renderer::SubmitInstancedUnits(MeshID mesh, const Material* material, RenderLayer layer, unsigned int instanceCount, bool additiveBlend)
{
    m_InstancedUnits.push_back({ mesh, material, layer, instanceCount, additiveBlend });
}


