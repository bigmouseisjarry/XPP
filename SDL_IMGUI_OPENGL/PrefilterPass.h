#pragma once
#include "RenderPass.h"

class PrefilterPass : public RenderPass
{
public:
    PipelineState GetPipelineState() const override;
    void Setup(RenderContext& ctx) override;
    void Execute(RenderContext& ctx) override;
    void Teardown(RenderContext& ctx) override;
    bool ManagesOwnFBO() const override { return true; }
    std::string GetName() const override { return "Prefilter"; }

    void SetInputCubemapID(TextureID id) { m_InputCubemapID = id; }
    void SetOutputCubemapID(TextureID id) { m_OutputCubemapID = id; }

private:
    TextureID m_InputCubemapID{ INVALID_ID };
    TextureID m_OutputCubemapID{ INVALID_ID };
    bool m_Completed = false;
};