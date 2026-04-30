#include "RenderPipeline.h"
#include "PipelineUtils.h"

void RenderPipeline::AddPass(std::unique_ptr<RenderPass> pass)
{
	m_Passes.push_back(std::move(pass));
}

void RenderPipeline::Execute(RenderContext& ctx)
{
    PipelineState saved = PipelineUtils::CaptureGLState();
    FramebufferID prevOutputFBO = {INVALID_ID};

    for (size_t i = 0; i < m_Passes.size(); ++i)
    {
        auto& pass = m_Passes[i];

        for (auto& inputs : pass->GetDeclaredInputs())
        {
            unsigned int texID = ResolveTextureID(inputs.semantic, inputs.level);
			//std::cout << pass->GetName() << " declares input: " << static_cast<int>(inputs.semantic) << " level " << inputs.level << " resolved texID: " << texID << std::endl;
			if (texID != 0)
			    pass->SetInputTexture(inputs.semantic, texID);
        }

		PipelineUtils::ApplyPipelineState(pass->GetPipelineState());

        if (!pass->ManagesOwnFBO())
        {
            
            Framebuffer* targetFBO = ResourceManager::Get()->GetFramebufferMut(pass->GetTargetFBO());
            if (targetFBO)
            {
                targetFBO->Bind();
                if (pass->GetTargetFBO().value != prevOutputFBO.value)
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }
            else
            {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, ctx.viewport.width, ctx.viewport.height);
            }
        }

        pass->Setup(ctx);
        pass->Execute(ctx);
        pass->Teardown(ctx);

		Framebuffer* outputFBO = ResourceManager::Get()->GetFramebufferMut(pass->GetOutputFBO());
        if (pass->GetOutputFBO().value != INVALID_ID)
        {
			for (auto& outputs : pass->GetDeclaredOutputs())
            {
                //std::cout << pass->GetName() << " declares output: " << static_cast<int>(outputs.semantic) << " level " << outputs.level << " resolved texID: " << ResolveTextureID(outputs.semantic, outputs.level) << std::endl;
				m_TexturePool[PoolKey{ outputs.semantic, outputs.level }] = TextureSource{ pass->GetOutputFBO(), outputs.colorIndex, 0 };
            }
        }


        prevOutputFBO = pass->GetOutputFBO();
    }

    PipelineUtils::RestoreGLState(saved);
}

const std::vector<std::unique_ptr<RenderPass>>& RenderPipeline::GetPasses() const
{
	return m_Passes;
}

// 注册非 FBO 纹理
void RenderPipeline::RegisterTexture(TextureSemantic semantic, unsigned int textureID, int level)
{
    m_TexturePool[PoolKey{ semantic,level }] = TextureSource{ {INVALID_ID}, 0, textureID };
}
// 注册 FBO 纹理（colorIndex: -1 = depth attachment）
void RenderPipeline::RegisterFBOTexture(TextureSemantic semantic, FramebufferID fbo, int colorIndex, int level)
{
    m_TexturePool[PoolKey{ semantic,level }] = TextureSource{ fbo, colorIndex,0 };
}

unsigned int RenderPipeline::ResolveTextureID(TextureSemantic semantic, int level)
{
    auto it = m_TexturePool.find(PoolKey{ semantic, level });
    if (it == m_TexturePool.end()) return 0;

    const auto& src = it->second;
    if (src.directTexID != 0) return src.directTexID;
    if (src.fbo.value == INVALID_ID) return 0;

    if (src.colorIndex == -1)
        return ResourceManager::Get()->GetFramebuffer(src.fbo)->GetDepthGLTextureID();
    else
        return ResourceManager::Get()->GetFramebuffer(src.fbo)->GetColorGLTextureID(src.colorIndex);
}

