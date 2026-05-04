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
    if (m_ShadowTexGLID) {
        glActiveTexture(GL_TEXTURE0 + TextureSlot::GetSlot(TextureSemantic::ShadowMapArray));
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_ShadowTexGLID);
    }
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
    if (m_ShadowTexGLID) {
        glActiveTexture(GL_TEXTURE0 + TextureSlot::GetSlot(TextureSemantic::ShadowMapArray));
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }
}

void OpaquePass3D::SetInputTexture(TextureSemantic semantic, unsigned int textureID)
{
    if (semantic == TextureSemantic::ShadowMapArray)
        m_ShadowTexGLID = textureID;
}
