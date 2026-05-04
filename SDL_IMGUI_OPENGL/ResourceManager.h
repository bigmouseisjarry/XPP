#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include "Shader.h"
#include "Texture.h"
#include "mesh.h"
#include "Framebuffer.h"
#include "ResourceIDs.h"


class ResourceManager
{
private:

    // ID → 资源 
    std::vector<std::unique_ptr<Shader>>  m_Shaders;
    std::vector<std::unique_ptr<Texture>> m_Textures;
    std::vector<std::unique_ptr<Mesh>>    m_Meshes;
    std::vector<std::unique_ptr<Framebuffer>> m_Framebuffers;


    // 名称 → ID
    std::unordered_map<std::string, ShaderID>           m_ShaderNameToID;
    std::unordered_map<std::string, TextureID>          m_TextureNameToID;
    std::unordered_map<std::string, MeshID>             m_MeshNameToID;
    std::unordered_map<std::string, FramebufferID>      m_FramebufferNameToID;

public:
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    static ResourceManager* Get();

    void Init();

    // ========== 注册接口（返回 ID）==========
    ShaderID  LoadShader(const std::string& filePath);
    TextureID LoadTexture(const std::string& path, int cols = 1, int rows = 1);
    TextureID LoadDefaultTexture(const std::string& name, float r, float g, float b, float a);     
    TextureID LoadHDRTexture(const std::string& name);
    TextureID CreateNoiseTexture(const std::string& name, int width, int height);
    TextureID CreateTextureFromPixels(const std::string& name, unsigned char* pixels, int width, int height, int channels, int minFilter, int magFilter,
        int wrapS, int wrapT, bool sRGB);
    TextureID CreateCubemapTexture(const std::string& name, int size, GLenum internalFormat, int mipLevels = 1);
    TextureID CreateTexture2D(const std::string& name, int width, int height, GLenum internalFormat);
    TextureID CreateDepthTexture2D(const std::string& name, int width, int height, bool clampToBorder = false);
    TextureID CreateDepthTextureArray(const std::string& name, int width, int height, int layers, bool clampToBorder = false, bool shadowCompare = false);

    template<typename VertexT>
    MeshID CreateMesh(const std::string& name, const std::vector<VertexT>& vertices, const std::vector<unsigned int>& indices);
    MeshID CreateInstancedMesh(const std::string& name, const Mesh& sharedMesh, std::unique_ptr<VertexBuffer> instanceVBO, const VertexBufferLayout& instanceLayout,
        unsigned int maxInstances);
    FramebufferID CreateFramebuffer(const std::string& name, const FramebufferSpec& spec);
    MeshID CreateMeshFromBlob(const std::string& name, const std::vector<uint8_t>& vertexBlob, size_t vertexSize, const std::vector<unsigned int>& indices,
        VertexType vertexType);

    // ========== 名称查找==========
    ShaderID   GetShaderID(const std::string& name) const;
    TextureID  GetTextureID(const std::string& name) const;
    MeshID     GetMeshID(const std::string& name) const;
    FramebufferID GetFramebufferID(const std::string& name) const;

    // ========== ID 查找（运行时用，O(1)）==========
    const Shader* GetShader(ShaderID id) const;
    const Texture* GetTexture(TextureID id) const;
    Texture* GetTextureMut(TextureID id);
    const Framebuffer* GetFramebuffer(FramebufferID id) const;
    Framebuffer* GetFramebufferMut(FramebufferID id);
    const Mesh* GetMesh(MeshID id) const;
    Mesh* GetMeshMut(MeshID id);

private:
    ResourceManager() = default;
    ~ResourceManager() = default;

};

template<typename VertexT>
inline MeshID ResourceManager::CreateMesh(const std::string& name, const std::vector<VertexT>& vertices, const std::vector<unsigned int>& indices)
{
     // 检查是否已存在
     auto it = m_MeshNameToID.find(name);
     if (it != m_MeshNameToID.end())
         return it->second;
 
     // 根据类型推断 VertexType 枚举
     VertexType vertexType;
     if constexpr (std::is_same_v<VertexT, Vertex>)
         vertexType = VertexType::Vertex2D;
     else if constexpr (std::is_same_v<VertexT, Vertex3D>)
         vertexType = VertexType::Vertex3D;
     else if constexpr (std::is_same_v<VertexT, VertexLine>)
		 vertexType = VertexType::VertexLine;
     else
         static_assert(false, "Unsupported vertex type");
 
     // 创建 Mesh
     auto mesh = std::make_unique<Mesh>(
         vertices.data(),
         vertices.size() * sizeof(VertexT),
         indices.data(),
         vertexType,
         indices.size()
     );
 
     // 分配 ID
     MeshID id{ static_cast<uint32_t>(m_Meshes.size()) };
     m_Meshes.push_back(std::move(mesh));
     m_MeshNameToID[name] = id;
 
     return id;
}
