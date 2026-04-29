#include "Renderer.h"
#include "RenderPass.h"
#include <iomanip>
#include <gtc/matrix_transform.hpp>
#include <algorithm>
#include "ShadowPass.h"
#include "OpaquePass3D.h"
#include "TransparentPass3D.h"
#include "OpaquePass2D.h"
#include "TransparentPass2D.h"
#include "PostProcessPass.h"
#include "SkyboxPass.h"

Renderer* Renderer::Get()
{
    static Renderer instance;
    return &instance;
}
// TODO: 这里的 UBO 初始化其实也可以放到 ResourceManager 里,让它来管理所有 GPU 资源,Renderer 只负责使用
// 或者干脆把 UBO 包装成一个 UniformBuffer 类, 由 Renderer 来管理和使用, ResourceManager 不涉及任何 GPU 资源的细节
void Renderer::InitUBO()            
{
    m_PerFrameUBO = std::make_unique<UniformBuffer>(sizeof(PerFrameData), static_cast<unsigned int>(UBOBinding::PerFrame));
    m_PerObjectUBO = std::make_unique<UniformBuffer>(sizeof(PerObjectData), static_cast<unsigned int>(UBOBinding::PerObject));
    m_LightUBO = std::make_unique<UniformBuffer>(sizeof(LightArrayData), static_cast<unsigned int>(UBOBinding::Lights));
}

void Renderer::Clear()
{
    m_Camera2DUnits.clear();
    m_Camera3DUnits.clear();
    for (auto& [layer, units] : m_RenderUnits)
        units.clear();

}

void Renderer::Flush(const std::vector<Light3DComponent*>& lights)
{
    RenderContext ctx{ m_RenderUnits,m_Camera2DUnits, m_Camera3DUnits,lights,
        *m_LightUBO,*m_PerFrameUBO,*m_PerObjectUBO,m_Viewport };
    
	m_Pipeline->Execute(ctx);

    Clear();
}

void Renderer::InitPipeline(bool has3D)
{
    m_Pipeline = std::make_unique<RenderPipeline>();

    if (has3D)
    {
        // 创建阴影数组 FBO（depth array，8 层 = 最多 8 盏灯阴影）
        FramebufferSpec shadowSpec;
        shadowSpec.width = 2048;
        shadowSpec.height = 2048;
        shadowSpec.depthAttachment = { AttachmentFormat::DepthArray24 };
        shadowSpec.depthClampToBorder = true;
        shadowSpec.depthLayers = 8;
        shadowSpec.depthShadowCompare = true;
        m_ShadowArrayFBOID = ResourceManager::Get()->CreateFramebuffer("ShadowArray", shadowSpec);

        // 创建中间 Scene FBO（HDR 颜色 + 法线 + 可采样深度）
        FramebufferSpec sceneSpec;
        sceneSpec.width = 1024; sceneSpec.height = 768;
        sceneSpec.colorAttachments.push_back({ AttachmentFormat::RGBA16F });  // [0] HDR 场景颜色
        sceneSpec.colorAttachments.push_back({ AttachmentFormat::RGBA16F });  // [1] 世界空间法线
        sceneSpec.depthAttachment = { AttachmentFormat::Depth24 };            // 可采样深度纹理
        FramebufferID sceneFBOID = ResourceManager::Get()->CreateFramebuffer("Scene3D", sceneSpec);
        Framebuffer* sceneFBO = ResourceManager::Get()->GetFramebufferMut(sceneFBOID);
        RegisterIntermediateFBO(sceneFBOID);

        // 创建FXAA
        FramebufferSpec fxaaSpec;
        fxaaSpec.width = 1024; fxaaSpec.height = 768;
        fxaaSpec.colorAttachments.push_back({ AttachmentFormat::RGBA16F });
        FramebufferID fxaaFBOID = ResourceManager::Get()->CreateFramebuffer("FXAA", fxaaSpec);
        RegisterIntermediateFBO(fxaaFBOID, 1.0f, 1.0f);
        Framebuffer* fxaaFBO = ResourceManager::Get()->GetFramebufferMut(fxaaFBOID);

        // ============ 基础渲染 Pass ============

        // Depth 注册
        m_Pipeline->RegisterFBOTexture(TextureSemantic::Depth, sceneFBO, -1);
        // depth 是隐式输出，color 是显式输出

        // ShadowPass：自管理 FBO
        m_Pipeline->AddPass(std::make_unique<ShadowPass>());

        auto skyboxPass = std::make_unique<SkyboxPass>();
        skyboxPass->SetTargetFBO(sceneFBO);
        skyboxPass->SetOutputFBO(sceneFBO);
		skyboxPass->DeclareOutput(TextureSemantic::SceneColor, 0);
        m_SkyboxTexID = ResourceManager::Get()->GetTextureID("resources/skybox/moonless_golf_4k.hdr");
        m_Pipeline->AddPass(std::move(skyboxPass));

        auto opaque3D = std::make_unique<OpaquePass3D>();
        opaque3D->SetTargetFBO(sceneFBO);
        opaque3D->SetOutputFBO(sceneFBO);
		opaque3D->DeclareOutput(TextureSemantic::SceneColor, 0);
        m_Pipeline->AddPass(std::move(opaque3D));

        auto trans3D = std::make_unique<TransparentPass3D>();
        trans3D->SetTargetFBO(sceneFBO);
        trans3D->SetOutputFBO(sceneFBO);
        trans3D->DeclareOutput(TextureSemantic::SceneColor, 0);
        m_Pipeline->AddPass(std::move(trans3D));

        //ShaderID fxaaShaderID = ResourceManager::Get()->GetShaderID("ShaderFXAA.glsl");
        //auto fxaaPass = std::make_unique<PostProcessPass>("FXAA", fxaaShaderID);
        //fxaaPass->SetTargetFBO(fxaaFBO);
        //fxaaPass->SetOutputFBO(fxaaFBO);
        //fxaaPass->DeclareInput(TextureSemantic::SceneColor);
        //fxaaPass->DeclareOutput(TextureSemantic::SceneColor, 0);  
        //m_Pipeline->AddPass(std::move(fxaaPass));

		auto PostProcess = std::make_unique<PostProcessPass>("PostProcess", ResourceManager::Get()->GetShaderID("ShaderPostProcess.glsl"));
		PostProcess->SetTargetFBO(nullptr);
        PostProcess->DeclareInput(TextureSemantic::SceneColor, 0);
		m_Pipeline->AddPass(std::move(PostProcess));


        // ============ Bloom FBO ============

        // 辅助 lambda：创建指定缩放比例的 Bloom FBO
        auto createBloomFBO = [&](const std::string& name, float scale) -> Framebuffer* {
            FramebufferSpec spec;
            spec.width = std::max(1, (int)(1024 * scale));
            spec.height = std::max(1, (int)(768 * scale));
            spec.colorAttachments.push_back({ AttachmentFormat::RGBA16F });
            FramebufferID fboID = ResourceManager::Get()->CreateFramebuffer(name, spec);
            RegisterIntermediateFBO(fboID, scale, scale);
            return ResourceManager::Get()->GetFramebufferMut(fboID);
            };

        // 亮部提取
        Framebuffer* bloomBrightFBO = createBloomFBO("BloomBright", 0.5f);
        // Level 0 ping-pong (1/2 分辨率)
        Framebuffer* bloomPing0 = createBloomFBO("BloomPing0", 0.5f);
        Framebuffer* bloomPong0 = createBloomFBO("BloomPong0", 0.5f);
        // Level 1 ping-pong (1/4 分辨率)
        Framebuffer* bloomPing1 = createBloomFBO("BloomPing1", 0.25f);
        Framebuffer* bloomPong1 = createBloomFBO("BloomPong1", 0.25f);
        // Level 2 ping-pong (1/8 分辨率)
        Framebuffer* bloomPing2 = createBloomFBO("BloomPing2", 0.125f);
        Framebuffer* bloomPong2 = createBloomFBO("BloomPong2", 0.125f);
        // Level 3 ping-pong (1/16 分辨率)
        Framebuffer* bloomPing3 = createBloomFBO("BloomPing3", 0.0625f);
        Framebuffer* bloomPong3 = createBloomFBO("BloomPong3", 0.0625f);
        // 上采样中间结果
        Framebuffer* bloomUp3FBO = createBloomFBO("BloomUp3", 0.125f);
        Framebuffer* bloomUp2FBO = createBloomFBO("BloomUp2", 0.25f);
        Framebuffer* bloomUp1FBO = createBloomFBO("BloomUp1", 0.5f);
        // 最终 Bloom 结果（全分辨率）
        Framebuffer* bloomResultFBO = createBloomFBO("BloomResult", 1.0f);

        // Shader IDs
        ShaderID bloomBrightShader = ResourceManager::Get()->GetShaderID("ShaderBloomBright.glsl");
        ShaderID bloomBlurShader = ResourceManager::Get()->GetShaderID("ShaderBloomBlur.glsl");
        ShaderID bloomUpShader = ResourceManager::Get()->GetShaderID("ShaderBloomUp.glsl");

        // ============ Bloom Pass ============

        // 亮部提取：读取 Scene3D，提取亮度超过阈值的像素
        auto bloomBrightPass = std::make_unique<PostProcessPass>("BloomBright", bloomBrightShader);
        bloomBrightPass->SetTargetFBO(bloomBrightFBO);
        bloomBrightPass->SetOutputFBO(bloomBrightFBO);
        bloomBrightPass->DeclareInput(TextureSemantic::SceneColor);
        bloomBrightPass->DeclareOutput(TextureSemantic::BloomBright, 0);
        bloomBrightPass->SetSetupCallback([](const Shader* shader) {
            shader->Set1f("u_Threshold", 2.0f);
            });
        m_Pipeline->AddPass(std::move(bloomBrightPass));

        // 4 级高斯模糊（每级水平 + 垂直，ping-pong）
        // Ping = V-blur 输出，Pong = H-blur 输出
        struct BlurLevel { Framebuffer* ping; Framebuffer* pong; };
        BlurLevel blurLevels[4] = {
            { bloomPing0, bloomPong0 },
            { bloomPing1, bloomPong1 },
            { bloomPing2, bloomPong2 },
            { bloomPing3, bloomPong3 },
        };

        for (int i = 0; i < 4; i++)
        {
            auto hBlur = std::make_unique<PostProcessPass>(
                "BloomBlurH" + std::to_string(i), bloomBlurShader);
            hBlur->SetTargetFBO(blurLevels[i].pong);
            hBlur->SetOutputFBO(blurLevels[i].pong);
            hBlur->DeclareInput(TextureSemantic::BloomBright);
            hBlur->DeclareOutput(TextureSemantic::BloomBright, 0);
            hBlur->SetSetupCallback([](const Shader* shader) {
                shader->SetVec2f("u_Direction", glm::vec2(1.0f, 0.0f));
                });
            m_Pipeline->AddPass(std::move(hBlur));

            auto vBlur = std::make_unique<PostProcessPass>(
                "BloomBlurV" + std::to_string(i), bloomBlurShader);
            vBlur->SetTargetFBO(blurLevels[i].ping);
            vBlur->SetOutputFBO(blurLevels[i].ping);
            vBlur->DeclareInput(TextureSemantic::BloomBright);
            vBlur->DeclareOutput(TextureSemantic::BloomBright, 0, 0);
            if (i < 3)
                vBlur->DeclareOutput(TextureSemantic::BloomBright, 0, i + 1);  // 给对应 BloomUp
            else
                vBlur->DeclareOutput(TextureSemantic::BloomUp, 0, 0);          // BlurV3 → 上采样起点

            vBlur->SetSetupCallback([](const Shader* shader) {
                shader->SetVec2f("u_Direction", glm::vec2(0.0f, 1.0f));
                });
            m_Pipeline->AddPass(std::move(vBlur));
        }

        // BloomUp3：上采样 1/16→1/8，叠加 level 2 blur
        auto up3 = std::make_unique<PostProcessPass>("BloomUp3", bloomUpShader);
        up3->SetTargetFBO(bloomUp3FBO);
        up3->SetOutputFBO(bloomUp3FBO);
        up3->DeclareInput(TextureSemantic::BloomUp, 0);              // 从 BlurV3
        up3->DeclareInput(TextureSemantic::BloomBright, 3);          // BlurV2 的 level=3
        up3->DeclareOutput(TextureSemantic::BloomUp, 0);
        m_Pipeline->AddPass(std::move(up3));

        // BloomUp2：上采样 1/8→1/4，叠加 level 1 blur
        auto up2 = std::make_unique<PostProcessPass>("BloomUp2", bloomUpShader);
        up2->SetTargetFBO(bloomUp2FBO);
        up2->SetOutputFBO(bloomUp2FBO);
        up2->DeclareInput(TextureSemantic::BloomUp, 0);
        up2->DeclareInput(TextureSemantic::BloomBright, 2);          // BlurV1 的 level=2
        up2->DeclareOutput(TextureSemantic::BloomUp, 0);
        m_Pipeline->AddPass(std::move(up2));

        // BloomUp1：上采样 1/4→1/2，叠加 level 0 blur
        auto up1 = std::make_unique<PostProcessPass>("BloomUp1", bloomUpShader);
        up1->SetTargetFBO(bloomUp1FBO);
        up1->SetOutputFBO(bloomUp1FBO);
        up1->DeclareInput(TextureSemantic::BloomUp, 0);
        up1->DeclareInput(TextureSemantic::BloomBright, 1);          // BlurV0 的 level=1
        up1->DeclareOutput(TextureSemantic::BloomUp, 0);
        m_Pipeline->AddPass(std::move(up1));

        // BloomUp0：上采样 1/2→全分辨率，产出最终 Bloom
        auto up0 = std::make_unique<PostProcessPass>("BloomUp0", bloomUpShader);
        up0->SetTargetFBO(bloomResultFBO);
        up0->SetOutputFBO(bloomResultFBO);
        up0->DeclareInput(TextureSemantic::BloomUp, 0);
        up0->DeclareInput(TextureSemantic::BloomBright, 1);          // 也用 level=1（最近一级 blur）
        up0->DeclareOutput(TextureSemantic::Bloom, 0);
        m_Pipeline->AddPass(std::move(up0));

        // ============ SSAO FBO ============

        Framebuffer* ssaoFBO = createBloomFBO("SSAO", 1.0f);
        Framebuffer* ssaoBlurFBO = createBloomFBO("SSAOBlur", 1.0f);

        // 生成 SSAO 采样核（64 个半球采样点）
        std::vector<glm::vec3> ssaoKernel(64);
        for (int i = 0; i < 64; i++)
        {
            glm::vec3 sample(
                (float)rand() / (float)RAND_MAX * 2.0f - 1.0f,
                (float)rand() / (float)RAND_MAX * 2.0f - 1.0f,
                (float)rand() / (float)RAND_MAX
            );
            sample = glm::normalize(sample);
            sample *= (float)rand() / (float)RAND_MAX;

            // 让采样点集中在原点附近（更精确的近距离遮蔽）
            float scale = (float)i / 64.0f;
            scale = 0.1f + scale * scale * 0.9f;
            sample *= scale;

            ssaoKernel[i] = sample;
        }

        // 创建 SSAO 采样核 UBO（std140 布局：每个 vec3 对齐到 vec4 = 16 字节）
        std::vector<glm::vec4> ssaoKernelPadded(64);
        for (int i = 0; i < 64; i++)
            ssaoKernelPadded[i] = glm::vec4(ssaoKernel[i], 0.0f);
        m_SSAOKernelUBO = std::make_unique<UniformBuffer>(ssaoKernelPadded.data(), ssaoKernelPadded.size() * sizeof(glm::vec4),static_cast<int>(UBOBinding::SSAOKernel));

        // 获取噪声纹理
        TextureID noiseTexID = ResourceManager::Get()->GetTextureID("u_NoiseMap");
        const Texture* noiseTex = ResourceManager::Get()->GetTexture(noiseTexID);

        ShaderID ssaoShader = ResourceManager::Get()->GetShaderID("ShaderSSAO.glsl");
        ShaderID ssaoBlurShader = ResourceManager::Get()->GetShaderID("ShaderSSAOBlur.glsl");

        // ============ SSAO Pass ============

        auto ssaoPass = std::make_unique<PostProcessPass>("SSAO", ssaoShader);
        ssaoPass->SetTargetFBO(ssaoFBO);
        ssaoPass->SetOutputFBO(ssaoFBO);
        ssaoPass->DeclareInput(TextureSemantic::Depth);
        ssaoPass->DeclareOutput(TextureSemantic::SSAO, 0);
        ssaoPass->SetSetupCallback([noiseTex, this](const Shader* shader) {
            // 噪声纹理（槽位 14，避让 BloomUp=13）
            unsigned int noiseSlot = 14;
            noiseTex->Bind(noiseSlot);
            shader->Set1i("u_NoiseMap", noiseSlot);

            glm::mat4 proj = Renderer::Get()->GetProjection();
            shader->SetMat4f("u_Projection", proj);
            shader->SetMat4f("u_InvProjection", glm::inverse(proj));
            const auto& vp = Renderer::Get()->GetViewport();
            shader->SetVec2f("u_NoiseScale", glm::vec2(vp.width / 4.0f, vp.height / 4.0f));
            shader->Set1f("u_Radius", 0.5f);
            });
        m_Pipeline->AddPass(std::move(ssaoPass));

        // SSAO模糊
        auto ssaoBlurPass = std::make_unique<PostProcessPass>("SSAOBlur", ssaoBlurShader);
        ssaoBlurPass->SetTargetFBO(ssaoBlurFBO);
        ssaoBlurPass->SetOutputFBO(ssaoBlurFBO);
        ssaoBlurPass->DeclareInput(TextureSemantic::SSAO);
        ssaoBlurPass->DeclareOutput(TextureSemantic::SSAO, 0);
        m_Pipeline->AddPass(std::move(ssaoBlurPass));


        // ============ Composite Pass ============

        ShaderID compositeShaderID = ResourceManager::Get()->GetShaderID("ShaderComposite.glsl");
        auto compositePass = std::make_unique<PostProcessPass>("Composite", compositeShaderID);
        compositePass->SetTargetFBO(nullptr);
        compositePass->DeclareInput(TextureSemantic::SceneColor);
        compositePass->DeclareInput(TextureSemantic::Bloom);
        compositePass->DeclareInput(TextureSemantic::SSAO);
        m_Pipeline->AddPass(std::move(compositePass));
    }
    else
    {
        m_Pipeline->AddPass(std::make_unique<OpaquePass2D>());
        m_Pipeline->AddPass(std::make_unique<TransparentPass2D>());
    }
}

void Renderer::SetWindowSize(int w, int h)
{
    m_Viewport.width = w;
    m_Viewport.height = h;
    for (auto& [fboID, scale] : m_IntermediateFBOs)
    {
        Framebuffer* fbo = ResourceManager::Get()->GetFramebufferMut(fboID);
        if (fbo) fbo->Resize(static_cast<int>(w * scale.x), static_cast<int>(h * scale.y));
    }
}

void Renderer::RegisterIntermediateFBO(FramebufferID fboID, float scaleX, float scaleY)
{
    m_IntermediateFBOs.push_back({ fboID, glm::vec2(scaleX, scaleY) });
}


void Renderer::SubmitCameraUnits(const glm::mat4& projView, const std::vector<RenderLayer>& layers, RenderMode mode, const glm::vec3& viewPos)
{
    if(mode == RenderMode::Render2D)
        m_Camera2DUnits.emplace_back(projView, layers, viewPos);
	else
        m_Camera3DUnits.emplace_back(projView, layers, viewPos);
}

void Renderer::SubmitRenderUnits(const MeshID& mesh, const Material* material,const glm::mat4& model,RenderLayer renderlayer)
{
    m_RenderUnits[renderlayer].emplace_back(mesh, material, model);
}






