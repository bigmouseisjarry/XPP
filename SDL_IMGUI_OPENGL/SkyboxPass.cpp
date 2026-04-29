#include "SkyboxPass.h"
#include "ResourceManager.h"
#include "FullscreenQuad.h"
#include <gtc/matrix_transform.hpp>
#include "Renderer.h"
PipelineState SkyboxPass::GetPipelineState() const
{
    PipelineState ps;
    ps.blend.enabled = false;
    ps.depth.enabled = true;
    ps.depth.func = GL_LEQUAL;     // 清除后 depth=1.0，GL_LESS 会失败
    ps.depth.writeMask = true;
    ps.cull.enabled = false;       // 全屏四边形无需剔除
    ps.colorMask[0] = ps.colorMask[1] = ps.colorMask[2] = ps.colorMask[3] = true;
    return ps;
}

void SkyboxPass::Setup(RenderContext& ctx)
{
    if (ctx.cameras3D.empty()) return;

	const auto& cam = ctx.cameras3D[0];         // TODO: 目前只支持一个摄像机，后续可以改成遍历所有摄像机

    TextureID skyboxTexID = Renderer::Get()->GetSkyboxTexID();
    const Texture* skyboxTex = ResourceManager::Get()->GetTexture(skyboxTexID);

    // 绑定 HDR 纹理到 slot 7
    unsigned int slot = TextureSlot::GetSlot(TextureSemantic::Skybox);
    if (skyboxTex)
        skyboxTex->Bind(slot);

    const Shader* shader = ResourceManager::Get()->GetShader(
        ResourceManager::Get()->GetShaderID("ShaderSkybox.glsl"));
    if (!shader) return;

    shader->Bind();
    shader->Set1i("u_SkyboxMap", slot);
    shader->SetMat4f("u_InvProjView", glm::inverse(cam.projView));
    shader->SetVec3f("u_ViewPos", cam.viewPos);
}

void SkyboxPass::Execute(RenderContext& ctx)
{
    const Shader* shader = ResourceManager::Get()->GetShader(
        ResourceManager::Get()->GetShaderID("ShaderSkybox.glsl"));
    FullscreenQuad::Draw(shader);
}

void SkyboxPass::Teardown(RenderContext& ctx)
{
    unsigned int slot = TextureSlot::GetSlot(TextureSemantic::Skybox);
    const Texture* skyboxTex = ResourceManager::Get()->GetTexture(Renderer::Get()->GetSkyboxTexID());
    skyboxTex->UnBind(slot);
}