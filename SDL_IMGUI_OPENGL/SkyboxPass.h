#pragma once
#include "RenderPass.h"

class SkyboxPass : public RenderPass
{
public:
    PipelineState GetPipelineState() const override;
    void Setup(RenderContext& ctx) override;
    void Execute(RenderContext& ctx) override;
    void Teardown(RenderContext& ctx) override;
    std::string GetName() const override { return "SkyboxPass"; }
};