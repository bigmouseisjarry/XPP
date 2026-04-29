#pragma once
#include "RenderPass.h"

class ShadowPass : public RenderPass
{
public:
    PipelineState GetPipelineState() const override;
    void Setup(RenderContext& ctx) override;
    void Execute(RenderContext& ctx) override;
    void Teardown(RenderContext& ctx) override;
    bool ManagesOwnFBO() const override { return true; }
    std::string GetName() const override { return "ShadowPass"; }

};