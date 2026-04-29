#pragma once

#include <vector>
#include <string>
#include <glm.hpp>
#include "Vertex.h"
#include "TextureSemantic.h"

struct TextureInfo
{
    TextureSemantic semantic;
    std::string path;  // 相对于模型目录的纹理路径
};

struct SubMeshData
{
    std::string name;
    std::vector<Vertex3D> vertices;
    std::vector<unsigned int> indices;
    std::vector<TextureInfo> textures;
    glm::vec4 diffuseColor = { 1,1,1,1 };   // Kd + d
};

struct ModelData
{
    std::vector<SubMeshData> subMeshes;
};

namespace ModelLoader
{
    ModelData LoadOBJ(const std::string& filepath);
    ModelData LoadGLTF(const std::string& filepath);
    ModelData LoadModel(const std::string& filepath);  // 按扩展名分发
}