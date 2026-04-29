#pragma once
#pragma once
#include "RenderPass.h"

class OpaquePass2D : public RenderPass
{
public:
    OpaquePass2D() {}
	PipelineState GetPipelineState() const override;
    void Setup(RenderContext& ctx) override;
    void Execute(RenderContext& ctx) override;
    void Teardown(RenderContext& ctx) override;
    std::string GetName() const override { return "OpaquePass2D"; }
};