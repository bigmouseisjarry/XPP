#pragma once
#include <memory>
#include <vector>
#include "RenderPass.h"

struct TextureSource {
    Framebuffer* fbo = nullptr;
    int colorIndex = 0;           // -1 = depth attachment
    unsigned int directTexID = 0; // 不走 FBO 的外部纹理（噪声等）
};

struct PoolKey {
    TextureSemantic semantic;
    int level = 0;
    bool operator==(const PoolKey& other) const {
        return semantic == other.semantic && level == other.level;
    }
};

struct PoolKeyHash {
    size_t operator()(const PoolKey& k) const noexcept {
        return static_cast<size_t>(k.semantic) * 1000 + k.level;
    }
};

class RenderPipeline
{
public:
    void AddPass(std::unique_ptr<RenderPass> pass);
    void Execute(RenderContext& ctx);
    const std::vector<std::unique_ptr<RenderPass>>& GetPasses() const;

    void RegisterTexture(TextureSemantic semantic, unsigned int textureID, int level = 0);
    void RegisterFBOTexture(TextureSemantic semantic, Framebuffer* fbo, int colorIndex, int level = 0);

private:
    // 延迟解析：池中的 TextureSource → 实际的 GL texture ID
    unsigned int ResolveTextureID(TextureSemantic semantic, int level = 0);

    std::vector<std::unique_ptr<RenderPass>> m_Passes;
    std::unordered_map<PoolKey, TextureSource, PoolKeyHash> m_TexturePool;
};

