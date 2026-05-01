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

struct TextureInfo
{
    TextureSemantic semantic;
    std::string path;  // 相对于模型目录的纹理路径
    SamplerInfo sampler;
};

struct SubMeshData
{
    std::string name;
    std::vector<Vertex3D> vertices;
    std::vector<unsigned int> indices;
    std::vector<TextureInfo> textures;
    glm::vec4 diffuseColor = { 1,1,1,1 };       // 漫反射反照率（Kd） + 透明度（d）
    float metallicFactor = 1.0f;                // 金属度因子
    float roughnessFactor = 1.0f;               // 粗糙度因子
    glm::vec3 emissiveFactor = { 0,0,0 };       // 自发光颜色强度
    int   alphaMode = 0;                        // 0=OPAQUE, 1=MASK 镂空模式 , 2=BLEND
    float alphaCutoff = 0.5f;                   // MASK:纹理 Alpha 值低于 0.5 的像素直接被丢弃
    bool  doubleSided = false;                  // 双面渲染
};

struct ModelData
{
    std::vector<SubMeshData> subMeshes;
};

struct NodeData
{
    std::string name;
    int meshIndex = -1;         // glTF mesh index（引用用）
    std::vector<int> children;  // 子节点在 GLTFScene::nodes 中的索引

    // 变换：优先使用 TRS，仅 useMatrix=true 时使用 matrix
    bool useMatrix = false;
    glm::mat4 matrix = glm::mat4(1.0f);
    glm::vec3 translation = { 0, 0, 0 };
    glm::quat rotation = glm::quat(1, 0, 0, 0);  // w,x,y,z
    glm::vec3 scale = { 1, 1, 1 };

    // 该节点的子网格数据（顶点在局部空间，不含节点变换）
    std::vector<SubMeshData> subMeshes;
};

struct GLTFScene
{
    std::vector<NodeData> nodes;
    std::vector<int> rootNodes;  // 场景根节点索引
};

namespace ModelLoader
{
    ModelData LoadOBJ(const std::string& filepath);
    ModelData LoadGLTF(const std::string& filepath);
    ModelData LoadModel(const std::string& filepath);  // 按扩展名分发

    GLTFScene LoadGLTFScene(const std::string& filepath);
}