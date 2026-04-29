#pragma once
#include "RenderTypes.h"

namespace PipelineUtils
{
	// 捕获当前 GL 状态到 PipelineState 结构
    static PipelineState CaptureGLState()
    {
        PipelineState s;

        // Blend
        GLboolean blendEnabled; glGetBooleanv(GL_BLEND, &blendEnabled);
        s.blend.enabled = blendEnabled;
        GLint blendSrc, blendDst;
        glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrc);
        glGetIntegerv(GL_BLEND_DST_RGB, &blendDst);
        s.blend.srcFactor = static_cast<GLenum>(blendSrc);
        s.blend.dstFactor = static_cast<GLenum>(blendDst);

        // Depth
        GLboolean depthEnabled; glGetBooleanv(GL_DEPTH_TEST, &depthEnabled);
        s.depth.enabled = depthEnabled;
        GLint depthFunc; glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
        s.depth.func = static_cast<GLenum>(depthFunc);
        GLboolean depthMask; glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        s.depth.writeMask = depthMask;

        // Cull
        GLboolean cullEnabled; glGetBooleanv(GL_CULL_FACE, &cullEnabled);
        s.cull.enabled = cullEnabled;
        GLint cullFace; glGetIntegerv(GL_CULL_FACE_MODE, &cullFace);
        s.cull.face = static_cast<GLenum>(cullFace);
        GLint frontFace; glGetIntegerv(GL_FRONT_FACE, &frontFace);
        s.cull.winding = static_cast<GLenum>(frontFace);

        // Color mask
        GLboolean cm[4]; glGetBooleanv(GL_COLOR_WRITEMASK, cm);
        for (int i = 0; i < 4; i++) s.colorMask[i] = cm[i];

        // Viewport
        GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
        s.viewport.x = vp[0];
        s.viewport.y = vp[1];
        s.viewport.width = vp[2];
        s.viewport.height = vp[3];

        return s;
    }

	static void ApplyPipelineState(const PipelineState& state) // TUDO: 改成增量式设置,只设置与当前状态不同的部分
    {
        // Blend
        state.blend.enabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
        if (state.blend.enabled)
            glBlendFunc(state.blend.srcFactor, state.blend.dstFactor);

        // Depth
        state.depth.enabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
        glDepthFunc(state.depth.func);
        glDepthMask(state.depth.writeMask ? GL_TRUE : GL_FALSE);

        // Cull
        state.cull.enabled ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        glCullFace(state.cull.face);
        glFrontFace(state.cull.winding);

        // Color mask
        glColorMask(state.colorMask[0], state.colorMask[1],
            state.colorMask[2], state.colorMask[3]);
    }

    static void RestoreGLState(const PipelineState& saved)
    {
        ApplyPipelineState(saved);
        glViewport(saved.viewport.x, saved.viewport.y,
            saved.viewport.width, saved.viewport.height);
    }

}