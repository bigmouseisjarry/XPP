#pragma once
#include <string>
#include <unordered_map>

enum class TextureSemantic : unsigned int
{
    Albedo = 0,                    // 漫反射贴图（基础颜色）
    ShadowMapArray = 1,            // 阴影贴图
    Normal = 2,                    // 法线贴图
    MetallicRoughness = 3,         // 金属度/粗糙度贴图
    Emissive = 4,                  // 自发光贴图
    Occlusion = 5,                 // 环境光遮蔽贴图
	SceneColor = 6,                // 后处理场景颜色输入
    Skybox = 7,                    // 天空盒 equirectangular 纹理
    Depth = 8,                     // 场景深度纹理
    WorldNormal = 9,               // 世界空间法线纹理
    BloomBright = 10,              // Bloom 亮部提取结果，上采样叠加时读取
    SSAO = 11,                     // SSAO 遮蔽值，Blur 和 Composite 读取
    Bloom = 12,                    // 最终 Bloom 全分辨率结果，Composite 读取
    BloomUp = 13,                  // 上采样中间结果
};

struct TextureSemanticHash
{
    size_t operator()(TextureSemantic t) const noexcept
    {
        return static_cast<size_t>(t);
    }
};

namespace TextureSlot
{
    // 槽位索引
    inline unsigned int GetSlot(TextureSemantic semantic)
    {
        return static_cast<unsigned int>(semantic);
    }

    // 语义 → 着色器采样器名(resourcemanager 的默认纹理也是和着色器采样器名对应的)
    inline const std::string& GetSamplerName(TextureSemantic semantic)
    {
        static const std::unordered_map<TextureSemantic, std::string, TextureSemanticHash> s_SamplerNames = {
              { TextureSemantic::Albedo,            "u_AlbedoMap" },
              { TextureSemantic::ShadowMapArray,    "u_ShadowMaps" },
              { TextureSemantic::Normal,            "u_NormalMap" },
              { TextureSemantic::MetallicRoughness, "u_MetallicRoughnessMap" },
              { TextureSemantic::Emissive,          "u_EmissiveMap" },
              { TextureSemantic::Occlusion,         "u_OcclusionMap" },
              { TextureSemantic::SceneColor,        "u_SceneMap" },
              { TextureSemantic::Skybox,            "u_SkyboxMap" },
              { TextureSemantic::Depth,             "u_DepthMap" },
              { TextureSemantic::WorldNormal,       "u_WorldNormalMap" },
              { TextureSemantic::BloomBright,       "u_BloomBrightMap" },
              { TextureSemantic::SSAO,              "u_SSAOMap" },
              { TextureSemantic::Bloom,             "u_BloomMap" },
              { TextureSemantic::BloomUp,           "u_BloomUpMap" },
        };
        static const std::string empty;
        auto it = s_SamplerNames.find(semantic);
        return it != s_SamplerNames.end() ? it->second : empty;
    }

    // 着色器采样器名 → 语义
    inline TextureSemantic GetSemantic(const std::string& samplerName)
    {
        static const std::unordered_map<std::string, TextureSemantic> s_NameToSemantic = {
              { "u_AlbedoMap",            TextureSemantic::Albedo },
              { "u_ShadowMaps",           TextureSemantic::ShadowMapArray },
              { "u_NormalMap",            TextureSemantic::Normal },
              { "u_MetallicRoughnessMap", TextureSemantic::MetallicRoughness },
              { "u_EmissiveMap",          TextureSemantic::Emissive },
              { "u_OcclusionMap",         TextureSemantic::Occlusion },
              { "u_SceneMap",             TextureSemantic::SceneColor },
              { "u_SkyboxMap",            TextureSemantic::Skybox },
              { "u_DepthMap",             TextureSemantic::Depth },
              { "u_WorldNormalMap",       TextureSemantic::WorldNormal },
              { "u_BloomBrightMap",       TextureSemantic::BloomBright },
              { "u_SSAOMap",              TextureSemantic::SSAO },
              { "u_BloomMap",             TextureSemantic::Bloom },
              { "u_BloomUpMap",           TextureSemantic::BloomUp },
        };
        auto it = s_NameToSemantic.find(samplerName);
        return it != s_NameToSemantic.end() ? it->second : TextureSemantic::Albedo;
    }
}