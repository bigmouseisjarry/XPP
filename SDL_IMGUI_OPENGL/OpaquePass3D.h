#pragma once
#include "RenderPass.h"

class OpaquePass3D : public RenderPass
{
public:
    OpaquePass3D() {}

    PipelineState GetPipelineState() const override;
    void Setup(RenderContext& ctx) override;
    void Execute(RenderContext& ctx) override;
    void Teardown(RenderContext& ctx) override;
    std::string GetName() const override { return "OpaquePass3D"; }
    void SetInputTexture(TextureSemantic semantic, unsigned int textureID) override;

public:
    unsigned int m_ShadowTexGLID = 0;
};