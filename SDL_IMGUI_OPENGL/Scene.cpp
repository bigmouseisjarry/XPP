#include "Scene.h"
#include "ResourceManager.h"
#include "OpaquePass2D.h"
#include "TransparentPass2D.h"
#include "Renderer.h"
#include "ShadowPass.h"
#include "SkyboxPass.h"
#include "OpaquePass3D.h"
#include "TransparentPass3D.h"
#include "PostProcessPass.h"
#include "EquirectToCubemapPass.h"
#include "IrradiancePass.h"
#include "PrefilterPass.h"
#include "BRDFLUTPass.h"

void Scene::BuildDefaultPipeline2D(RenderPipeline& pipeline)
{
	pipeline.AddPass(std::make_unique<OpaquePass2D>());
    pipeline.AddPass(std::make_unique<TransparentPass2D>());
}

void Scene::BuildDefaultPipeline3D(RenderPipeline& pipeline)
{
    auto* rm = ResourceManager::Get();  // 提前到这里，后面全用 rm
    TextureID shadowDepthTexID = rm->CreateDepthTextureArray("ShadowDepthArray", 2048, 2048, 8, true, true);
    FramebufferSpec shadowSpec;
    shadowSpec.width = 2048; shadowSpec.height = 2048;
    shadowSpec.depthAttachment = shadowDepthTexID;
    Renderer::Get()->SetShadowArrayFBOID(rm->CreateFramebuffer("ShadowArray", shadowSpec));

        // 创建中间 Scene FBO（HDR 颜色 + 法线 + 可采样深度）
    TextureID sceneColorTex = rm->CreateTexture2D("SceneColorTex", 1024, 768, GL_RGBA16F);
    TextureID sceneNormalTex = rm->CreateTexture2D("SceneNormalTex", 1024, 768, GL_RGBA16F);
    TextureID sceneDepthTex = rm->CreateDepthTexture2D("SceneDepthTex", 1024, 768, false);
    FramebufferSpec sceneSpec;
    sceneSpec.width = 1024; sceneSpec.height = 768;
    sceneSpec.colorAttachments = { sceneColorTex, sceneNormalTex };
    sceneSpec.depthAttachment = sceneDepthTex;
    FramebufferID sceneFBOID = rm->CreateFramebuffer("Scene3D", sceneSpec);
    Renderer::Get()->RegisterIntermediateFBO(sceneFBOID);

    // ============ 基础渲染 Pass ============

    // Depth 注册
    pipeline.RegisterTextureID(TextureSemantic::Depth, sceneDepthTex);
    // depth 是隐式输出，color 是显式输出

    // ShadowPass：自管理 FBO
    pipeline.AddPass(std::make_unique<ShadowPass>());

    // ============ IBL 预计算 ============

    // 创建 EnvironmentCubemap（512×512, RGBA16F, 1 mip level → 渲染后 GenerateMipmap 生成完整链）
    TextureID envCubemapID = rm->CreateCubemapTexture(
        TextureSlot::GetSamplerName(TextureSemantic::EnvironmentCubemap), 512, GL_RGBA16F, 1);

    // 创建 IrradianceMap（32×32, RGBA16F）
    TextureID irradianceID = rm->CreateCubemapTexture(
        TextureSlot::GetSamplerName(TextureSemantic::IrradianceMap), 32, GL_RGBA16F, 1);

    // 创建 PrefilterMap（128×128, RGBA16F, 5 mip levels）
    TextureID prefilterID = rm->CreateCubemapTexture(
        TextureSlot::GetSamplerName(TextureSemantic::PrefilterMap), 128, GL_RGBA16F, 5);

    // 创建 BRDF LUT（512×512, RG16F, 2D 纹理）
    TextureID brdfLutID = rm->CreateTexture2D(
        TextureSlot::GetSamplerName(TextureSemantic::BRDFLUT), 512, 512, GL_RG16F);
    // 注意：BRDFLUT 是 2D 不是 cubemap，但 Texture 构造函数可以根据
    // internalFormat 判断——或者需要单独的 CreateTexture2D 方法

    // 天空盒 HDR 纹理 ID（SkyboxPass 已有这行，这里提前获取）
    TextureID skyboxTexID = rm->GetTextureID("resources/skybox/moonless_golf_4k.hdr");

    // EquirectToCubemap
    auto equirectPass = std::make_unique<EquirectToCubemapPass>();
    equirectPass->SetSkyboxTextureID(skyboxTexID);
    equirectPass->SetOutputCubemapID(envCubemapID);
    pipeline.AddPass(std::move(equirectPass));

    // Irradiance
    auto irradiancePass = std::make_unique<IrradiancePass>();
    irradiancePass->SetInputCubemapID(envCubemapID);
    irradiancePass->SetOutputCubemapID(irradianceID);
    pipeline.AddPass(std::move(irradiancePass));

    // Prefilter
    auto prefilterPass = std::make_unique<PrefilterPass>();
    prefilterPass->SetInputCubemapID(envCubemapID);
    prefilterPass->SetOutputCubemapID(prefilterID);
    pipeline.AddPass(std::move(prefilterPass));

    // BRDF LUT
    auto brdfLutPass = std::make_unique<BRDFLUTPass>();
    brdfLutPass->SetOutputTextureID(brdfLutID);
    pipeline.AddPass(std::move(brdfLutPass));


    auto skyboxPass = std::make_unique<SkyboxPass>();
    skyboxPass->SetTargetFBO(sceneFBOID);
    skyboxPass->SetOutputFBO(sceneFBOID);
	skyboxPass->DeclareOutput(TextureSemantic::SceneColor, 0);
    Renderer::Get()->SetSkyboxTexID(skyboxTexID);
    pipeline.AddPass(std::move(skyboxPass));

    auto opaque3D = std::make_unique<OpaquePass3D>();
    opaque3D->SetTargetFBO(sceneFBOID);
    opaque3D->SetOutputFBO(sceneFBOID);
	opaque3D->DeclareOutput(TextureSemantic::SceneColor, 0);
    opaque3D->DeclareInput(TextureSemantic::ShadowMapArray);
    pipeline.AddPass(std::move(opaque3D));

    auto trans3D = std::make_unique<TransparentPass3D>();
    trans3D->SetTargetFBO(sceneFBOID);
    trans3D->SetOutputFBO(sceneFBOID);
    trans3D->DeclareOutput(TextureSemantic::SceneColor, 0);
    trans3D->DeclareInput(TextureSemantic::ShadowMapArray);
    pipeline.AddPass(std::move(trans3D));

    // 创建FXAA
    TextureID fxaaColorTex = rm->CreateTexture2D("FXAAColorTex", 1024, 768, GL_RGBA16F);
    FramebufferSpec fxaaSpec;
    fxaaSpec.width = 1024; fxaaSpec.height = 768;
    fxaaSpec.colorAttachments = { fxaaColorTex };
    FramebufferID fxaaFBOID = rm->CreateFramebuffer("FXAA", fxaaSpec);
    Renderer::Get()->RegisterIntermediateFBO(fxaaFBOID, 1.0f, 1.0f);
    Framebuffer* fxaaFBO = ResourceManager::Get()->GetFramebufferMut(fxaaFBOID);

    //ShaderID fxaaShaderID = ResourceManager::Get()->GetShaderID("ShaderFXAA.glsl");
    //auto fxaaPass = std::make_unique<PostProcessPass>("FXAA", fxaaShaderID);
    //fxaaPass->SetTargetFBO(fxaaFBOID);
    //fxaaPass->SetOutputFBO(fxaaFBOID);
    //fxaaPass->DeclareInput(TextureSemantic::SceneColor);
    //fxaaPass->DeclareOutput(TextureSemantic::SceneColor, 0);  
    //pipeline.AddPass(std::move(fxaaPass));

	//auto PostProcess = std::make_unique<PostProcessPass>("PostProcess", ResourceManager::Get()->GetShaderID("ShaderPostProcess.glsl"));
	//PostProcess->SetTargetFBO(nullptr);
    //PostProcess->DeclareInput(TextureSemantic::SceneColor, 0);
	//pipeline.AddPass(std::move(PostProcess));


    // ============ Bloom FBO ============

    // 辅助 lambda：创建指定缩放比例的 Bloom FBO
    auto createBloomFBOID = [&](const std::string& name, float scale) -> FramebufferID {
        int w = std::max(1, (int)(1024 * scale));
        int h = std::max(1, (int)(768 * scale));
        TextureID tex = rm->CreateTexture2D(name + "Tex", w, h, GL_RGBA16F);
        FramebufferSpec spec;
        spec.width = w; spec.height = h;
        spec.colorAttachments = { tex };
        FramebufferID fboID = rm->CreateFramebuffer(name, spec);
        Renderer::Get()->RegisterIntermediateFBO(fboID, scale, scale);
        return fboID;
        };

    // 亮部提取
    FramebufferID bloomBrightFBOID = createBloomFBOID("BloomBright", 0.5f);
    // Level 0 ping-pong (1/2 分辨率)
    FramebufferID bloomPing0ID = createBloomFBOID("BloomPing0", 0.5f);
    FramebufferID bloomPong0ID = createBloomFBOID("BloomPong0", 0.5f);
    // Level 1 ping-pong (1/4 分辨率)
    FramebufferID bloomPing1ID = createBloomFBOID("BloomPing1", 0.25f);
    FramebufferID bloomPong1ID = createBloomFBOID("BloomPong1", 0.25f);
    // Level 2 ping-pong (1/8 分辨率)
    FramebufferID bloomPing2ID = createBloomFBOID("BloomPing2", 0.125f);
    FramebufferID bloomPong2ID = createBloomFBOID("BloomPong2", 0.125f);
    // Level 3 ping-pong (1/16 分辨率)
    FramebufferID bloomPing3ID = createBloomFBOID("BloomPing3", 0.0625f);
    FramebufferID bloomPong3ID = createBloomFBOID("BloomPong3", 0.0625f);
    // 上采样中间结果
    FramebufferID bloomUp3FBOID = createBloomFBOID("BloomUp3", 0.125f);
    FramebufferID bloomUp2FBOID = createBloomFBOID("BloomUp2", 0.25f);
    FramebufferID bloomUp1FBOID = createBloomFBOID("BloomUp1", 0.5f);
    // 最终 Bloom 结果（全分辨率）
    FramebufferID bloomResultFBOID = createBloomFBOID("BloomResult", 1.0f);

    // Shader IDs
    ShaderID bloomBrightShader = ResourceManager::Get()->GetShaderID("ShaderBloomBright.glsl");
    ShaderID bloomBlurShader = ResourceManager::Get()->GetShaderID("ShaderBloomBlur.glsl");
    ShaderID bloomUpShader = ResourceManager::Get()->GetShaderID("ShaderBloomUp.glsl");

    // ============ Bloom Pass ============

    // 亮部提取：读取 Scene3D，提取亮度超过阈值的像素
    auto bloomBrightPass = std::make_unique<PostProcessPass>("BloomBright", bloomBrightShader);
    bloomBrightPass->SetTargetFBO(bloomBrightFBOID);
    bloomBrightPass->SetOutputFBO(bloomBrightFBOID);
    bloomBrightPass->DeclareInput(TextureSemantic::SceneColor);
    bloomBrightPass->DeclareOutput(TextureSemantic::BloomBright, 0);
    bloomBrightPass->SetSetupCallback([](const Shader* shader) {
        shader->Set1f("u_Threshold", 2.0f);
        });
    pipeline.AddPass(std::move(bloomBrightPass));

    // 4 级高斯模糊（每级水平 + 垂直，ping-pong）
    // Ping = V-blur 输出，Pong = H-blur 输出
    struct BlurLevel { FramebufferID ping; FramebufferID pong; };
    BlurLevel blurLevels[4] = {
        { bloomPing0ID, bloomPong0ID },
        { bloomPing1ID, bloomPong1ID },
        { bloomPing2ID, bloomPong2ID },
        { bloomPing3ID, bloomPong3ID },
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
        pipeline.AddPass(std::move(hBlur));

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
        pipeline.AddPass(std::move(vBlur));
    }

    // BloomUp3：上采样 1/16→1/8，叠加 level 2 blur
    auto up3 = std::make_unique<PostProcessPass>("BloomUp3", bloomUpShader);
    up3->SetTargetFBO(bloomUp3FBOID);
    up3->SetOutputFBO(bloomUp3FBOID);
    up3->DeclareInput(TextureSemantic::BloomUp, 0);              // 从 BlurV3
    up3->DeclareInput(TextureSemantic::BloomBright, 3);          // BlurV2 的 level=3
    up3->DeclareOutput(TextureSemantic::BloomUp, 0);
    pipeline.AddPass(std::move(up3));

    // BloomUp2：上采样 1/8→1/4，叠加 level 1 blur
    auto up2 = std::make_unique<PostProcessPass>("BloomUp2", bloomUpShader);
    up2->SetTargetFBO(bloomUp2FBOID);
    up2->SetOutputFBO(bloomUp2FBOID);
    up2->DeclareInput(TextureSemantic::BloomUp, 0);
    up2->DeclareInput(TextureSemantic::BloomBright, 2);          // BlurV1 的 level=2
    up2->DeclareOutput(TextureSemantic::BloomUp, 0);
    pipeline.AddPass(std::move(up2));

    // BloomUp1：上采样 1/4→1/2，叠加 level 0 blur
    auto up1 = std::make_unique<PostProcessPass>("BloomUp1", bloomUpShader);
    up1->SetTargetFBO(bloomUp1FBOID);
    up1->SetOutputFBO(bloomUp1FBOID);
    up1->DeclareInput(TextureSemantic::BloomUp, 0);
    up1->DeclareInput(TextureSemantic::BloomBright, 1);          // BlurV0 的 level=1
    up1->DeclareOutput(TextureSemantic::BloomUp, 0);
    pipeline.AddPass(std::move(up1));

    // BloomUp0：上采样 1/2→全分辨率，产出最终 Bloom
    auto up0 = std::make_unique<PostProcessPass>("BloomUp0", bloomUpShader);
    up0->SetTargetFBO(bloomResultFBOID);
    up0->SetOutputFBO(bloomResultFBOID);
    up0->DeclareInput(TextureSemantic::BloomUp, 0);
    up0->DeclareInput(TextureSemantic::BloomBright, 1);          // 也用 level=1（最近一级 blur）
    up0->DeclareOutput(TextureSemantic::Bloom, 0);
    pipeline.AddPass(std::move(up0));

    // ============ SSAO FBO ============

    FramebufferID ssaoFBOID = createBloomFBOID("SSAO", 1.0f);
    FramebufferID ssaoBlurFBOID = createBloomFBOID("SSAOBlur", 1.0f);

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
    Renderer::Get()->SetSSAOKernelUBO(
        std::make_unique<UniformBuffer>(ssaoKernelPadded.data(), ssaoKernelPadded.size() * sizeof(glm::vec4),static_cast<int>(UBOBinding::SSAOKernel)));

    // 获取噪声纹理
    TextureID noiseTexID = ResourceManager::Get()->GetTextureID("u_NoiseMap");
    const Texture* noiseTex = ResourceManager::Get()->GetTexture(noiseTexID);

    ShaderID ssaoShader = ResourceManager::Get()->GetShaderID("ShaderSSAO.glsl");
    ShaderID ssaoBlurShader = ResourceManager::Get()->GetShaderID("ShaderSSAOBlur.glsl");

    // ============ SSAO Pass ============

    auto ssaoPass = std::make_unique<PostProcessPass>("SSAO", ssaoShader);
    ssaoPass->SetTargetFBO(ssaoFBOID);
    ssaoPass->SetOutputFBO(ssaoFBOID);
    ssaoPass->DeclareInput(TextureSemantic::Depth);
    ssaoPass->DeclareOutput(TextureSemantic::SSAO, 0);
    ssaoPass->SetSetupCallback([noiseTex](const Shader* shader) {
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
    pipeline.AddPass(std::move(ssaoPass));

    // SSAO模糊
    auto ssaoBlurPass = std::make_unique<PostProcessPass>("SSAOBlur", ssaoBlurShader);
    ssaoBlurPass->SetTargetFBO(ssaoBlurFBOID);
    ssaoBlurPass->SetOutputFBO(ssaoBlurFBOID);
    ssaoBlurPass->DeclareInput(TextureSemantic::SSAO);
    ssaoBlurPass->DeclareOutput(TextureSemantic::SSAO, 0);
    pipeline.AddPass(std::move(ssaoBlurPass));


    // ============ Composite Pass ============

    ShaderID compositeShaderID = ResourceManager::Get()->GetShaderID("ShaderComposite.glsl");
    auto compositePass = std::make_unique<PostProcessPass>("Composite", compositeShaderID);
    compositePass->SetTargetFBO({INVALID_ID});
    compositePass->DeclareInput(TextureSemantic::SceneColor);
    compositePass->DeclareInput(TextureSemantic::Bloom);
    compositePass->DeclareInput(TextureSemantic::SSAO);
    pipeline.AddPass(std::move(compositePass));
}
