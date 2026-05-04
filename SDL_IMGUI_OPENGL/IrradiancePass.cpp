#include "IrradiancePass.h"
#include "FullscreenQuad.h"
#include "ResourceManager.h"
#include <ext/matrix_clip_space.hpp>
#include <ext/matrix_transform.hpp>

PipelineState IrradiancePass::GetPipelineState() const
{
    PipelineState ps;
    ps.blend.enabled = false;
    ps.depth.enabled = false;
    ps.cull.enabled = false;
    return ps;
}

void IrradiancePass::Setup(RenderContext& ctx)
{
}

void IrradiancePass::Execute(RenderContext& ctx)
{
    if (m_Completed) return;

    auto* rm = ResourceManager::Get();
    const Texture* envCubemap = rm->GetTexture(m_InputCubemapID);
    const Texture* irradianceMap = rm->GetTexture(m_OutputCubemapID);
    const Shader* shader = rm->GetShader(rm->GetShaderID("ShaderIrradiance.glsl"));

    if (!envCubemap || !irradianceMap || !shader) {
        std::cout << "IrradiancePass 的 envCubemap irradianceMap shader 有为空" << std::endl;
        return;
    }

    shader->Bind();

    // 绑定环境 cubemap 到 EnvironmentCubemap 语义 slot
    envCubemap->Bind(TextureSlot::GetSlot(TextureSemantic::EnvironmentCubemap));
    shader->Set1i("u_EnvironmentCubemap", TextureSlot::GetSlot(TextureSemantic::EnvironmentCubemap));

    // 同样的 90° 投影 + 6 面相机
    glm::mat4 captureProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(0, 0,-1), glm::vec3(0,-1, 0)),
    };

    FramebufferSpec spec;
    spec.width = 32;
    spec.height = 32;
    Framebuffer fbo(spec);

    for (int i = 0; i < 6; i++)
    {
        fbo.AttachCubemapFace(m_OutputCubemapID, i, 0);
        fbo.Bind();
        glViewport(0, 0, 32, 32);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 invProjView = glm::inverse(captureProj * captureViews[i]);
        shader->SetMat4f("u_InvProjView", invProjView);

        FullscreenQuad::Draw(shader);
    }

    fbo.Unbind();
    m_Completed = true;
}

void IrradiancePass::Teardown(RenderContext& ctx)
{
}
