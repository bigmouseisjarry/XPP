#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "ComLight.h"
#include "TextureSemantic.h"
#include "RenderTypes.h"
#include "ResourceManager.h"

struct RenderContext
{
    std::unordered_map<RenderLayer, std::vector<RenderUnit>>& renderUnits;
    const std::vector<CameraUnit>& cameras2D;
    const std::vector<CameraUnit>& cameras3D;
    const std::vector<Light3DComponent*>& lights;
    std::vector<InstancedRenderUnit>& instancedUnits;
    UniformBuffer& lightUBO;
    UniformBuffer& perFrameUBO;
    UniformBuffer& perObjectUBO;
    ViewportState viewport;
};

struct DeclaredInput {
    TextureSemantic semantic;
    int level = 0;
};

struct DeclaredOutput {
    TextureSemantic semantic;
    int colorIndex = 0;
    int level = 0;
};

class RenderPass
{
public:
    virtual ~RenderPass() = default;

    virtual PipelineState GetPipelineState() const = 0;  // 声明所需 GL 状态

    virtual void Setup(RenderContext& ctx) = 0;             // 准备
    virtual void Execute(RenderContext& ctx) = 0;           // 执行
    virtual void Teardown(RenderContext& ctx) = 0;          // 收尾
    virtual std::string GetName() const = 0;                // 调试

    virtual bool ManagesOwnFBO() const { return false; }

    FramebufferID GetOutputFBO() const { return m_OutputFBO; }
    void SetOutputFBO(FramebufferID fbo) { m_OutputFBO = fbo; }

    // 渲染目标 FBO
    void SetTargetFBO(FramebufferID fbo) { m_TargetFBO = fbo; }
    FramebufferID GetTargetFBO() const { return m_TargetFBO; }

    virtual void SetInputTexture(TextureSemantic semantic, unsigned int textureID) {}

    // ---- 声明式接口 ----
    void DeclareInput(TextureSemantic semantic, int level = 0) { m_DeclaredInputs.push_back({ semantic, level }); }
    const std::vector<DeclaredInput>& GetDeclaredInputs() const { return m_DeclaredInputs; }

    void DeclareOutput(TextureSemantic semantic, int colorIndex = 0, int level = 0) { m_DeclaredOutputs.push_back({ semantic, colorIndex, level }); }
    const std::vector<DeclaredOutput>& GetDeclaredOutputs() const { return m_DeclaredOutputs; }

private:
    FramebufferID m_TargetFBO = {INVALID_ID};
    FramebufferID m_OutputFBO = {INVALID_ID};
    std::vector<DeclaredInput> m_DeclaredInputs;
    std::vector<DeclaredOutput> m_DeclaredOutputs;
};