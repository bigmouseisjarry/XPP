#include "EquirectToCubemapPass.h"
#include "ResourceManager.h"
#include "Shader.h"
#include "Texture.h"
#include "Framebuffer.h"
#include "FullscreenQuad.h"
#include <ext/matrix_clip_space.hpp>
#include <ext/matrix_transform.hpp>

PipelineState EquirectToCubemapPass::GetPipelineState() const
{
    PipelineState ps;
    ps.blend.enabled = false;
    ps.depth.enabled = false;   // 不需要深度测试
    ps.cull.enabled = false;
    return ps;
}

void EquirectToCubemapPass::Setup(RenderContext& ctx)
{
    if (m_Completed) return;
}

void EquirectToCubemapPass::Execute(RenderContext& ctx)
{
    if (m_Completed) return;

    auto* rm = ResourceManager::Get();
    const Texture* skyboxTex = rm->GetTexture(m_SkyboxTexID);
    const Texture* cubemapTex = rm->GetTexture(m_OutputCubemapID);
    const Shader* shader = rm->GetShader(rm->GetShaderID("ShaderEquirectToCubemap.glsl"));

    if (!skyboxTex || !cubemapTex || !shader)
    {
        std::cout << "EquirectToCubemapPass 的 skyboxTex cubemapTex shader 有为空" << std::endl;
        return;
    }

    shader->Bind();

    // 天空盒 HDR 纹理绑定到它的语义 slot
    skyboxTex->Bind(TextureSlot::GetSlot(TextureSemantic::Skybox));
    shader->Set1i("u_SkyboxMap", TextureSlot::GetSlot(TextureSemantic::Skybox));

    // 6 个面的 view 矩阵
    glm::mat4 captureProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(0, 0,-1), glm::vec3(0,-1, 0)),
    };

    // 创建临时 FBO，逐面渲染
    FramebufferSpec spec;
    spec.width = 512;
    spec.height = 512;
    Framebuffer fbo(spec);  // 先创建，后面动态切换面

    for (int i = 0; i < 6; i++)
    {
        fbo.AttachCubemapFace(m_OutputCubemapID, i, 0);
        fbo.Bind();
        glViewport(0, 0, 512, 512);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 projView = captureProj * captureViews[i];
        glm::mat4 invProjView = glm::inverse(projView);
        shader->SetMat4f("u_InvProjView", invProjView);

        FullscreenQuad::Draw(shader);
    }

    fbo.Unbind();
    cubemapTex->GenerateMipmap();  // 生成 mipmap 供后续 irradiance/prefilter 使用

    m_Completed = true;
}

void EquirectToCubemapPass::Teardown(RenderContext& ctx)
{
}
