#pragma once
#include "RenderPass.h"

class TransparentPass3D : public RenderPass
{
public:
    TransparentPass3D() {}

	PipelineState GetPipelineState() const override;
    void Setup(RenderContext& ctx) override;
    void Execute(RenderContext& ctx) override;
    void Teardown(RenderContext& ctx) override;
    std::string GetName() const override { return "TransparentPass3D"; }
};