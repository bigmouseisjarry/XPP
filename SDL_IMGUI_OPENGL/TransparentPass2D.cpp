#include "TransparentPass2D.h"
#include "DrawUtils.h"

PipelineState TransparentPass2D::GetPipelineState() const
{
    PipelineState ps;
    ps.blend.enabled = true;
	ps.blend.srcFactor = GL_SRC_ALPHA;
    ps.blend.dstFactor = GL_ONE_MINUS_SRC_ALPHA;
    ps.depth.enabled = false;
    ps.cull.enabled = false;
    ps.colorMask[0] = ps.colorMask[1] = ps.colorMask[2] = ps.colorMask[3] = true;
    return ps;
}

void TransparentPass2D::Setup(RenderContext& ctx)
{
}

void TransparentPass2D::Execute(RenderContext& ctx)
{
    for (const auto& cam : ctx.cameras2D)
    {
        for (const auto& layer : cam.layers)
        {
            if (!ctx.renderUnits.contains(layer)) continue;
            auto& vec = ctx.renderUnits.at(layer);
            DrawUtils::DrawUnits(vec, true, cam.projView, cam.viewPos,
                ctx.lights, ctx.lightUBO, ctx.perFrameUBO, ctx.perObjectUBO);
        }
    }
}

void TransparentPass2D::Teardown(RenderContext& ctx)
{
}