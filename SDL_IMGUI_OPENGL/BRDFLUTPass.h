#pragma once
#include "RenderPass.h"

class BRDFLUTPass : public RenderPass
{
public:
    PipelineState GetPipelineState() const override;
    void Setup(RenderContext& ctx) override;
    void Execute(RenderContext& ctx) override;
    void Teardown(RenderContext& ctx) override;
    bool ManagesOwnFBO() const override { return true; }
    std::string GetName() const override { return "BRDFLUT"; }

    void SetOutputTextureID(TextureID id) { m_OutputTexID = id; }

private:
    TextureID m_OutputTexID{ INVALID_ID };
    bool m_Completed = false;
};