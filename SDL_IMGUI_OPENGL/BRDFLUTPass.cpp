#include "BRDFLUTPass.h"
#include "ResourceManager.h"
#include "FullscreenQuad.h"


PipelineState BRDFLUTPass::GetPipelineState() const
{
    PipelineState ps;
    ps.blend.enabled = false;
    ps.depth.enabled = false;
    ps.cull.enabled = false;
    return ps;
}

void BRDFLUTPass::Setup(RenderContext& ctx)
{
}

void BRDFLUTPass::Execute(RenderContext& ctx)
{
    if (m_Completed) return;

    auto* rm = ResourceManager::Get();
    const Texture* brdfTex = rm->GetTexture(m_OutputTexID);
    const Shader* shader = rm->GetShader(rm->GetShaderID("ShaderBRDFLUT" ));

    if (!brdfTex || !shader) return;

    // 创建普通 2D FBO（不是 cubemap）
    FramebufferSpec specV2;
    specV2.width = 512; specV2.height = 512;
    specV2.colorAttachments = { m_OutputTexID };  // TextureID 引用
    Framebuffer fbo(specV2);

    shader->Bind();
    fbo.Bind();
    glViewport(0, 0, 512, 512);
    glClear(GL_COLOR_BUFFER_BIT);

    FullscreenQuad::Draw(shader);

    fbo.Unbind();

    m_Completed = true;
}

void BRDFLUTPass::Teardown(RenderContext& ctx)
{
}
