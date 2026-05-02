#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <memory>

// 强类型 ID 前向声明（和 ResourceManager 一致）
struct TextureID;
struct MeshID;
enum class VertexType;

// GPU 纹理创建的延迟命令
struct PendingTexture {
    std::string name;
    int width, height, channels;
    std::vector<unsigned char> pixels;
    int minFilter, magFilter, wrapS, wrapT;
    bool sRGB;
    std::function<void(TextureID)> onComplete;
};

// GPU Mesh 创建的延迟命令
struct PendingMesh {
    std::string name;
    std::vector<uint8_t> vertexBlob;   // 顶点数据的字节流
    size_t vertexSize;                 // sizeof(Vertex3D) 等
    std::vector<unsigned int> indices;
    VertexType vertexType;
    std::function<void(MeshID)> onComplete;
};

class GLCommandQueue {
public:
    static GLCommandQueue* Get();

    void PushTexture(PendingTexture&& tex);
    void PushMesh(PendingMesh&& mesh);

    // 主线程每帧调用，执行所有待处理的 GL 命令
    void Drain();

    bool HasPending() const;

private:
    GLCommandQueue() = default;

    mutable std::mutex m_Mutex;
    std::vector<PendingTexture> m_PendingTextures;
    std::vector<PendingMesh> m_PendingMeshes;
};