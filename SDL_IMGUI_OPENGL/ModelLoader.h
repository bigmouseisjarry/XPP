#pragma once

#include <vector>
#include <string>
#include <glm.hpp>
#include <GL/glew.h>
#include "Vertex.h"
#include <gtc/quaternion.hpp>
#include "TextureSemantic.h"

struct SamplerInfo
{
    int minFilter = GL_LINEAR;
    int magFilter = GL_LINEAR;
    int wrapS = GL_CLAMP_TO_EDGE;
    int wrapT = GL_CLAMP_TO_EDGE;
    bool sRGB = false;              // Albedo/Emissive 需要线性化
    bool valid = false;             // false = Texture 构造函数用默认值
};

struct RawTextureData {
    std::string name;
    // 外部文件：stbi_load 解码后的像素，unique_ptr 自动 stbi_image_free
    // 嵌入纹理：直接引用 model.images 的数据，这里不需要持有多余内存
    // 统一用 vector<uint8_t> 拷贝持有，因为 tinygltf::Model 在函数返回后就销毁了
    std::vector<unsigned char> pixels;
    int width, height, channels;
    TextureSemantic semantic;
    SamplerInfo sampler;
};

struct SubMeshData {
    std::string name;
    std::vector<Vertex3D> vertices;
    std::vector<unsigned int> indices;
    std::vector<RawTextureData> textures;   // 已解码的纹理像素（替代 TextureInfo 的路径）
    glm::vec4 diffuseColor = { 1,1,1,1 };
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    glm::vec3 emissiveFactor = { 0,0,0 };
    int alphaMode = 0;
    float alphaCutoff = 0.5f;
    bool doubleSided = false;
};

struct ModelData {
    std::vector<SubMeshData> subMeshes;
};

// NodeData 不变，但子网格用 CPUSubMeshData
struct NodeData {
    std::string name;
    int meshIndex = -1;
    std::vector<int> children;
    bool useMatrix = false;
    glm::mat4 matrix = glm::mat4(1.0f);
    glm::vec3 translation = { 0, 0, 0 };
    glm::quat rotation = glm::quat(1, 0, 0, 0);
    glm::vec3 scale = { 1, 1, 1 };
    std::vector<SubMeshData> subMeshes;
};

struct GLTFScene {
    std::vector<NodeData> nodes;
    std::vector<int> rootNodes;
};

namespace ModelLoader
{
    ModelData LoadGLTF(const std::string& filepath);
    GLTFScene LoadGLTFScene(const std::string& filepath);
}