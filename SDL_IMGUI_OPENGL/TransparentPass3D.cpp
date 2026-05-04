#include "TransparentPass3D.h"
#include "DrawUtils.h"
#include "TextureSemantic.h"
#include "Renderer.h"

PipelineState TransparentPass3D::GetPipelineState() const
{
	PipelineState ps;
	ps.blend.enabled = true;
	ps.blend.srcFactor = GL_SRC_ALPHA;
	ps.blend.dstFactor = GL_ONE_MINUS_SRC_ALPHA;
	ps.depth.enabled = true;
	ps.depth.func = GL_LESS;
	ps.depth.writeMask = true;
	ps.cull.enabled = true;
	ps.cull.face = GL_BACK;
	ps.cull.winding = GL_CCW;
	ps.colorMask[0] = ps.colorMask[1] = ps.colorMask[2] = ps.colorMask[3] = true;
	return ps;
}

void TransparentPass3D::Setup(RenderContext& ctx)
{
	if (m_ShadowTexGLID) {
		glActiveTexture(GL_TEXTURE0 + TextureSlot::GetSlot(TextureSemantic::ShadowMapArray));
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_ShadowTexGLID);
	}
}

void TransparentPass3D::Execute(RenderContext& ctx)
{
	for (auto& cam : ctx.cameras3D)
	{
		for (auto& layer : cam.layers)
		{
			if (!ctx.renderUnits.contains(layer))continue;
			auto& vec = ctx.renderUnits[layer];

			DrawUtils::DrawUnits(vec, true, cam.projView, cam.viewPos, ctx.lights, ctx.lightUBO, ctx.perFrameUBO, ctx.perObjectUBO);
			DrawUtils::DrawInstancedUnits(ctx.instancedUnits, cam.projView, cam.viewPos, ctx.perFrameUBO);
		}
	}
}

void TransparentPass3D::Teardown(RenderContext& ctx)
{
	if (m_ShadowTexGLID) {
		glActiveTexture(GL_TEXTURE0 + TextureSlot::GetSlot(TextureSemantic::ShadowMapArray));
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}
}

void TransparentPass3D::SetInputTexture(TextureSemantic semantic, unsigned int textureID)
{
	if (semantic == TextureSemantic::ShadowMapArray)
		m_ShadowTexGLID = textureID;
}
