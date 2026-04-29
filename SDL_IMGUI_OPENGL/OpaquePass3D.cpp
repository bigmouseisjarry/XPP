#include "OpaquePass3D.h"
#include "DrawUtils.h"
#include "Renderer.h"

PipelineState OpaquePass3D::GetPipelineState() const
{
    PipelineState ps;
    ps.blend.enabled = false;
    ps.depth.enabled = true;
    ps.depth.func = GL_LESS;
    ps.depth.writeMask = true;
    ps.cull.enabled = true;
    ps.cull.face = GL_BACK;       
    ps.cull.winding = GL_CCW;
    ps.colorMask[0] = ps.colorMask[1] = ps.colorMask[2] = ps.colorMask[3] = true;
    return ps;
}

void OpaquePass3D::Setup(RenderContext& ctx)
{
    // 绑定阴影数组纹理
    FramebufferID shadowFBOID = Renderer::Get()->GetShadowArrayFBOID();
    const Framebuffer* shadowFBO = ResourceManager::Get()->GetFramebuffer(shadowFBOID);
    if (shadowFBO)
        shadowFBO->BindDepthTexture(TextureSlot::GetSlot(TextureSemantic::ShadowMapArray));
}

void OpaquePass3D::Execute(RenderContext& ctx)
{
    for (const auto& cam : ctx.cameras3D)
    {
        for (const auto& layer : cam.layers)
        {
            if (!ctx.renderUnits.contains(layer)) continue;
            auto& vec = ctx.renderUnits.at(layer);
            DrawUtils::DrawUnits(vec, false, cam.projView, cam.viewPos,
                ctx.lights, ctx.lightUBO, ctx.perFrameUBO, ctx.perObjectUBO);
        }
    }
}

void OpaquePass3D::Teardown(RenderContext& ctx)
{
    FramebufferID shadowFBOID = Renderer::Get()->GetShadowArrayFBOID();
    const Framebuffer* shadowFBO = ResourceManager::Get()->GetFramebuffer(shadowFBOID);
    if (shadowFBO)
        shadowFBO->UnbindDepthTexture(TextureSlot::GetSlot(TextureSemantic::ShadowMapArray));
}
