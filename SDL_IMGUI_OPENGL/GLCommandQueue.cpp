#include "GLCommandQueue.h"
#include "ResourceManager.h"

GLCommandQueue* GLCommandQueue::Get()
{
    static GLCommandQueue instance;
    return &instance;
}

void GLCommandQueue::PushTexture(PendingTexture&& tex)
{
    std::lock_guard lock(m_Mutex);
    m_PendingTextures.push_back(std::move(tex));
}

void GLCommandQueue::PushMesh(PendingMesh&& mesh)
{
    std::lock_guard lock(m_Mutex);
    m_PendingMeshes.push_back(std::move(mesh));
}

void GLCommandQueue::Drain()
{
    // 快速路径：无待处理命令
    if (!HasPending()) return;

    // 一次性取出所有命令（最小化锁持有时间）
    std::vector<PendingTexture> textures;
    std::vector<PendingMesh> meshes;
    {
        std::lock_guard lock(m_Mutex);
        textures.swap(m_PendingTextures);
        meshes.swap(m_PendingMeshes);
    }

    // 不持锁执行 GL 操作（主线程独占，安全）
    auto* rm = ResourceManager::Get();
    for (auto& tex : textures) {
        TextureID id = rm->CreateTextureFromPixels(
            tex.name, tex.pixels.data(), tex.width, tex.height, tex.channels,
            tex.minFilter, tex.magFilter, tex.wrapS, tex.wrapT, tex.sRGB);
        if (tex.onComplete) tex.onComplete(id);
    }
    for (auto& mesh : meshes) {
        MeshID id = rm->CreateMeshFromBlob(
            mesh.name, mesh.vertexBlob, mesh.vertexSize,
            mesh.indices, mesh.vertexType);
        if (mesh.onComplete) mesh.onComplete(id);
    }
}

bool GLCommandQueue::HasPending() const
{
    std::lock_guard lock(m_Mutex);
    return !m_PendingTextures.empty() || !m_PendingMeshes.empty();
}
