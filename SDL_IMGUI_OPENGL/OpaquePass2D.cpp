#include "OpaquePass2D.h"
#include "DrawUtils.h"

PipelineState OpaquePass2D::GetPipelineState() const
{
    PipelineState ps;
    ps.blend.enabled = false;
    ps.depth.enabled = false;
    ps.cull.enabled = false;
    ps.colorMask[0] = ps.colorMask[1] = ps.colorMask[2] = ps.colorMask[3] = true;
    return ps;
}

void OpaquePass2D::Setup(RenderContext& ctx)
{
}

void OpaquePass2D::Execute(RenderContext& ctx)
{
    for (const auto& cam : ctx.cameras2D)
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

void OpaquePass2D::Teardown(RenderContext& ctx)
{
}