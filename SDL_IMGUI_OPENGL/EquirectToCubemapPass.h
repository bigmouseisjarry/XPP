#pragma once
#include "RenderPass.h"

class EquirectToCubemapPass : public RenderPass
{
public:
    PipelineState GetPipelineState() const override;
    void Setup(RenderContext& ctx) override;
    void Execute(RenderContext& ctx) override;
    void Teardown(RenderContext& ctx) override;
    bool ManagesOwnFBO() const override { return true; }
    std::string GetName() const override { return "EquirectToCubemap"; }

    void SetSkyboxTextureID(TextureID id) { m_SkyboxTexID = id; }
    void SetOutputCubemapID(TextureID id) { m_OutputCubemapID = id; }

private:
    TextureID m_SkyboxTexID{ INVALID_ID };
    TextureID m_OutputCubemapID{ INVALID_ID };
    bool m_Completed = false;  // 一次性执行标记
};