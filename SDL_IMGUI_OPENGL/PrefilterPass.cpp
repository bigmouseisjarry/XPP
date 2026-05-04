#include "PrefilterPass.h"
#include "FullscreenQuad.h"
#include "ResourceManager.h"
#include <ext/matrix_clip_space.hpp>
#include <ext/matrix_transform.hpp>

PipelineState PrefilterPass::GetPipelineState() const
{
    PipelineState ps;
    ps.blend.enabled = false;
    ps.depth.enabled = false;
    ps.cull.enabled = false;
    return ps;
}

void PrefilterPass::Setup(RenderContext& ctx)
{
}

void PrefilterPass::Execute(RenderContext& ctx)
{

    if (m_Completed) return;

    auto* rm = ResourceManager::Get();
    const Texture* envCubemap = rm->GetTexture(m_InputCubemapID);
    const Texture* prefilterMap = rm->GetTexture(m_OutputCubemapID);
    const Shader* shader = rm->GetShader(rm->GetShaderID("ShaderPrefilter.glsl"));

    if (!envCubemap || !prefilterMap || !shader) {
        std::cout << "PrefilterPass 的 envCubemap prefilterMap shader 有为空" << std::endl;
        return;
    }
    shader->Bind();

    envCubemap->Bind(TextureSlot::GetSlot(TextureSemantic::EnvironmentCubemap));
    shader->Set1i("u_EnvironmentCubemap", TextureSlot::GetSlot(TextureSemantic::EnvironmentCubemap));

    glm::mat4 captureProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(0, 0,-1), glm::vec3(0,-1, 0)),
    };

    // 创建 FBO，大小先用 128，后面 AttachCubemapFace 时按 mip 调整 viewport
    FramebufferSpec spec;
    spec.width = 128;
    spec.height = 128;
    Framebuffer fbo(spec);

    int mipLevels = 5;
    for (int mip = 0; mip < mipLevels; mip++)
    {
        int mipSize = 128 >> mip;
        float roughness = (float)mip / (float)(mipLevels - 1);  // 0.0, 0.25, 0.5, 0.75, 1.0
        shader->Set1f("u_Roughness", roughness);

        for (int i = 0; i < 6; i++)
        {
            fbo.Bind();
            fbo.AttachCubemapFace(m_OutputCubemapID, i, mip);
            // glViewport(0, 0, mipSize, mipSize);
            glClear(GL_COLOR_BUFFER_BIT);

            glm::mat4 invProjView = glm::inverse(captureProj * captureViews[i]);
            shader->SetMat4f("u_InvProjView", invProjView);

            FullscreenQuad::Draw(shader);
        }
    }

    fbo.Unbind();
    m_Completed = true;
}

void PrefilterPass::Teardown(RenderContext& ctx)
{
}
