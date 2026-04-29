#pragma once
#include "RenderPass.h"
#include "FullscreenQuad.h"
#include <functional>

using SetupCallback = std::function<void(const Shader*)>;

class PostProcessPass : public RenderPass
{
public:
    PostProcessPass(const std::string& name, ShaderID shaderID)
        : m_Name(name), m_ShaderID(shaderID) {
    }

    PipelineState GetPipelineState() const override
    {
        PipelineState ps;
        ps.blend.enabled = false;
        ps.depth.enabled = false;
        ps.cull.enabled = false;
        return ps;
    }

    void Setup(RenderContext& ctx) override
    {

        const Shader* shader = ResourceManager::Get()->GetShader(m_ShaderID);
        if (shader) shader->Bind(); 
        for (auto& [semantic, texID] : m_InputTextures)
        {
            unsigned int slot = TextureSlot::GetSlot(semantic);
			glActiveTexture(GL_TEXTURE0 + slot);
			glBindTexture(GL_TEXTURE_2D, texID);

            if (shader) shader->Set1i(TextureSlot::GetSamplerName(semantic), slot);
        }

        // 非语义输入（噪声纹理、矩阵等）
        if (m_SetupCallback && shader)
            m_SetupCallback(shader);
    }

    void Execute(RenderContext& ctx) override
    {
        const Shader* shader = ResourceManager::Get()->GetShader(m_ShaderID);
        FullscreenQuad::Draw(shader);
    }

    void Teardown(RenderContext& ctx) override
    {
        for (auto& [semantic, texID] : m_InputTextures)
        {
            unsigned int slot = TextureSlot::GetSlot(semantic);
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, 0);

        }
    }

    std::string GetName() const override { return m_Name; }

    void SetInputTexture(TextureSemantic semantic, unsigned int textureID) override
    {
        m_InputTextures[semantic] = textureID;
    }

    void SetSetupCallback(SetupCallback cb) { m_SetupCallback = cb; }

protected:
    std::string m_Name;
    ShaderID m_ShaderID;
    std::unordered_map<TextureSemantic, unsigned int, TextureSemanticHash> m_InputTextures;
    SetupCallback m_SetupCallback;
};