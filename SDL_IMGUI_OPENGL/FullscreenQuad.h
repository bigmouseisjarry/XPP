#pragma once
#include "ResourceManager.h"

namespace FullscreenQuad
{
    inline void Draw(const Shader* shader)
    {
        if (!shader) return;
        shader->Bind();
        const Mesh* quad = ResourceManager::Get()->GetMesh(
            ResourceManager::Get()->GetMeshID("fullscreenQuad"));
        if (!quad) return;
        quad->GetVAO()->Bind();
        glDrawElements(quad->GetDrawMode(), quad->GetIBO()->GetCount(), GL_UNSIGNED_INT, nullptr);
        quad->GetVAO()->UnBind();
        shader->UnBind();
    }
}