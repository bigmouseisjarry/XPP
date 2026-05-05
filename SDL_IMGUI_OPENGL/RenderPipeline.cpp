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
                TextureID tid = (outputs.colorIndex == -1)
                    ? outputFBO->GetDepthTextureID()
                    : outputFBO->GetColorTextureID(outputs.colorIndex);
                m_TexturePool[PoolKey{ outputs.semantic, outputs.level }] = tid;

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

void RenderPipeline::RegisterTextureID(TextureSemantic semantic, TextureID texID, int level)
{
    m_TexturePool[PoolKey{ semantic, level }] = texID;
}

unsigned int RenderPipeline::ResolveTextureID(TextureSemantic semantic, int level)
{
    auto it = m_TexturePool.find(PoolKey{ semantic, level });
    if (it == m_TexturePool.end()) return 0;

    const auto& src = it->second;
    if (src.value != INVALID_ID) {
        const Texture* tex = ResourceManager::Get()->GetTexture(src);
        return tex ? tex->GetRendererID() : 0;
    }
    return 0;
}

