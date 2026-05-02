#include "DrawUtils.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "Material.h"
#include "Shader.h"
#include "mesh.h"
#include "TextureSemantic.h"
#include <algorithm>
#include "PipelineUtils.h"

namespace DrawUtils
{
    // 根据 uniform 类型调用对应的 Shader 设置函数
    static void SetUniformByType(const Shader* shader, const UniformInfo& info, const UniformValue& val)
    {
        switch (info.type)
        {
        case GL_FLOAT_MAT4:
            shader->SetMat4f(info.name, std::get<glm::mat4>(val));
            break;
        case GL_FLOAT_VEC4:
            shader->SetVec4f(info.name, std::get<glm::vec4>(val));
            break;
        case GL_FLOAT_VEC3:
            shader->SetVec3f(info.name, std::get<glm::vec3>(val));
            break;
        case GL_FLOAT_VEC2:
            shader->SetVec2f(info.name, std::get<glm::vec2>(val));
            break;
        case GL_FLOAT:
            shader->Set1f(info.name, std::get<float>(val));
            break;
        case GL_INT:
            shader->Set1i(info.name, std::get<int>(val));
            break;
        }
    }

    void DrawUnits(
        std::vector<RenderUnit>& units,
        bool transparent,
        const glm::mat4& projView,
        const glm::vec3& viewPos,
        const std::vector<Light3DComponent*>& lights,
        UniformBuffer& lightUBO,
        UniformBuffer& perFrameUBO,
        UniformBuffer& perObjectUBO)
    {
        // units 已由 Renderer::SortAll() 按复合键排好，无需重排

        const Shader* curShader = nullptr;
        const Mesh* mesh = nullptr;
        bool curDoubleSided = false;


        // 更新 per-frame UBO
        PerFrameData frameData;
        frameData.u_ProjView = projView;
        frameData.u_ViewPos = viewPos;
        perFrameUBO.SetData(&frameData, sizeof(PerFrameData));

        LightArrayData lightData{};
        int numLights = std::min((int)lights.size(), 8);
        lightData._numLights.x = numLights;
        for (int i = 0; i < numLights; i++)
        {
            lightData.u_Lights[i].position = lights[i]->position;
            lightData.u_Lights[i].color = lights[i]->color;
            lightData.u_Lights[i].intensity = lights[i]->intensity;
            lightData.u_Lights[i].lightSpaceMatrix = lights[i]->GetLightSpaceMatrix();
            lightData.u_Lights[i].flags.x = lights[i]->castShadow ? 1 : 0;
            lightData.u_Lights[i].flags.y = lights[i]->shadowLayerIndex;
        }
        lightUBO.SetData(&lightData, sizeof(LightArrayData));


        for (const auto& x : units)
        {
            if (x.material->m_Transparent != transparent) continue;

            bool doubleSided = x.material->m_DoubleSided;
            if (doubleSided != curDoubleSided)
            {
                curDoubleSided = doubleSided;
                PipelineState ps = PipelineUtils::GetCurrentState();
                ps.cull.enabled = !doubleSided;
                PipelineUtils::ApplyPipelineState(ps);
            }

            const auto& nextShader = ResourceManager::Get()->GetShader(x.material->m_Shader);
            if (curShader != nextShader)
            {
                if (curShader) curShader->UnBind();
                curShader = nextShader;
                curShader->Bind();

            }

            const auto& nextMesh = ResourceManager::Get()->GetMesh(x.mesh);

            if (!nextMesh)
            {
                std::cout << ResourceManager::Get()->GetTexture(x.material->GetTexture(TextureSemantic::Albedo))->GetFilePath() << std::endl;

            }

            if (mesh != nextMesh)
            {
                if (mesh) mesh->GetVAO()->UnBind();
                mesh = nextMesh;
                mesh->GetVAO()->Bind();
            }

            if (!curShader || !mesh) continue;

            for (const auto& info : curShader->GetUniforms())
            {
                if (info.type == GL_SAMPLER_2D || info.type == GL_SAMPLER_2D_ARRAY)
                {
                    TextureSemantic semantic = TextureSlot::GetSemantic(info.name);
                    unsigned int slot = TextureSlot::GetSlot(semantic);

                    if (semantic == TextureSemantic::ShadowMapArray)
                    {
                        curShader->Set1i(info.name, slot); 
                        continue;
                    }

                    TextureID texID = x.material->GetTexture(semantic);
                    if (texID.value == INVALID_ID)
                    {
                        texID = ResourceManager::Get()->GetTextureID(TextureSlot::GetSamplerName(semantic));
                    }

                    const Texture* tex = ResourceManager::Get()->GetTexture(texID);
					// std::cout << TextureSlot::GetSamplerName(semantic) << ": " << tex->GetFilePath() << std::endl;
                    if (tex) tex->Bind(slot);
                    curShader->Set1i(info.name, slot);
                }
            }

            // 更新 per-object UBO
            PerObjectData objData;
            objData.u_Model = x.model;
            objData.u_MVP = projView * x.model;
            objData.u_Color = std::get<glm::vec4>(*x.material->Get("u_Color"));
            const UniformValue* mf = x.material->Get("u_MetallicFactor");
            objData.u_MetallicFactor = mf ? std::get<float>(*mf) : 1.0f;
            const UniformValue* rf = x.material->Get("u_RoughnessFactor");
            objData.u_RoughnessFactor = rf ? std::get<float>(*rf) : 1.0f;
            const UniformValue* ef = x.material->Get("u_EmissiveFactor");
            objData.u_EmissiveFactor = ef ? glm::vec4(std::get<glm::vec3>(*ef), 0.0f) : glm::vec4(0.0f);
            const UniformValue* ac = x.material->Get("u_AlphaCutoff");
            objData.u_AlphaCutoff = ac ? std::get<float>(*ac) : 0.5f;
            perObjectUBO.SetData(&objData, sizeof(PerObjectData));

            // 反射循环：Material 自定义属性
            for (const auto& info : curShader->GetUniforms())
            {
                if (info.type == GL_SAMPLER_2D || info.type == GL_SAMPLER_2D_ARRAY) continue;

                const UniformValue* val = x.material->Get(info.name);
                if (!val) continue;
                SetUniformByType(curShader, info, *val);
            }

            glDrawElements(mesh->GetDrawMode(), mesh->GetIBO()->GetCount(), GL_UNSIGNED_INT, nullptr);

        }

        // 清理状态
        if (mesh) mesh->GetVAO()->UnBind();
        if (curShader) curShader->UnBind();
    }
}