#include "ShadowPass.h"
#include "RenderTypes.h"
#include "Renderer.h"
#include <ext/matrix_transform.hpp>
#include <ext/matrix_clip_space.hpp>

PipelineState ShadowPass::GetPipelineState() const
{
    PipelineState ps;
    ps.blend.enabled = false;
    ps.depth.enabled = true;
    ps.depth.func = GL_LESS;
    ps.depth.writeMask = true;
    ps.cull.enabled = true;
    ps.cull.face = GL_FRONT;       // shadow pass 特殊：剔除正面
    ps.cull.winding = GL_CCW;
    ps.colorMask[0] = ps.colorMask[1] = ps.colorMask[2] = ps.colorMask[3] = false;
    return ps;
}

void ShadowPass::Setup(RenderContext& ctx)
{
	
}

void ShadowPass::Execute(RenderContext& ctx)
{
    Framebuffer* shadowFBO = ResourceManager::Get()->GetFramebufferMut(
        Renderer::Get()->GetShadowArrayFBOID());
    if (!shadowFBO) return;
	shadowFBO->Bind();

    const Shader* shadowShader = ResourceManager::Get()->GetShader(
        ResourceManager::Get()->GetShaderID("ShaderShadow.glsl"));
    if (!shadowShader) return;

    shadowShader->Bind();

    int layerIndex = 0;
    for (auto* light : ctx.lights)
    {
        if (!light->castShadow) continue;

        // 用第一个(目前的主相机) 3D 相机的 VP 来拟合阴影投影
        if (!ctx.cameras3D.empty())
        {
            const auto& cam = ctx.cameras3D[0];
            // 从 projView 反推 view：projView = projection * view
            // 需要分开的 view 和 projection，但 CameraUnit 只存了 projView
            // 所以用 Renderer 存的 projection
            glm::mat4 camProj = Renderer::Get()->GetProjection();
            glm::mat4 camView = glm::inverse(camProj) * cam.projView;
            light->ComputeLightSpaceMatrix(camView, camProj);
        }
        else
        {
            // fallback：无相机时用固定范围
            glm::mat4 lightView = glm::lookAt(light->position, light->target, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 lightProj = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, 0.1f, 50.0f);
            light->lightSpaceMatrix = lightProj * lightView;
        }

        light->shadowLayerIndex = layerIndex;

        // 绑定到数组的第 layerIndex 层
        shadowFBO->BindLayer(layerIndex);
        glClear(GL_DEPTH_BUFFER_BIT);

        // 画所有不透明物体
        const Mesh* curMesh = nullptr;
        for (auto& [layer, units] : ctx.renderUnits)
        {
            for (const auto& unit : units)
            {
                if (unit.material->m_Transparent) continue;

                const Mesh* nextMesh = ResourceManager::Get()->GetMesh(unit.mesh);
                if (curMesh != nextMesh)
                {
                    if (curMesh) curMesh->GetVAO()->UnBind();
                    curMesh = nextMesh;
                    if (curMesh) curMesh->GetVAO()->Bind();
                }
                if (!curMesh) continue;

                PerObjectData objData;
                objData.u_Model = unit.model;
                objData.u_MVP = light->lightSpaceMatrix * unit.model;
                objData.u_Color = glm::vec4(1.0f);
                ctx.perObjectUBO.SetData(&objData, sizeof(PerObjectData));

                glDrawElements(curMesh->GetDrawMode(), curMesh->GetIBO()->GetCount(), GL_UNSIGNED_INT, nullptr);
            }
        }
        if (curMesh) curMesh->GetVAO()->UnBind();

        layerIndex++;
    }

    shadowShader->UnBind();
    shadowFBO->Unbind();

}

void ShadowPass::Teardown(RenderContext& ctx)
{
   
}
