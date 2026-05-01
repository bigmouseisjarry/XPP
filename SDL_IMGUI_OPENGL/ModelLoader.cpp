#include "ModelLoader.h"
#include "tiny_obj_loader.h"
#include "GeometryUtils.h"
#include <gtc/quaternion.hpp>
#include <iostream>

namespace ModelLoader
{

    ModelData LoadOBJ(const std::string& filepath)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        // 加载OBJ，MTL路径设为模型同目录
        size_t lastSlash = filepath.find_last_of("/\\");
        std::string mtlDir = (lastSlash != std::string::npos) ? filepath.substr(0, lastSlash + 1) : "";

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
            filepath.c_str(), mtlDir.c_str());

        ModelData modelData;

        for (const auto& shape : shapes)
        {
            SubMeshData subMesh;
            subMesh.name = shape.name;

            std::unordered_map<uint64_t, uint32_t> vertexCache;

            size_t indexOffset = 0;
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
            {
                int fv = shape.mesh.num_face_vertices[f]; // 面有几个顶点(通常=3)
                for (size_t v = 0; v < fv; v++)
                {
                    tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];

                    // 构建去重key
                    uint64_t key = (uint64_t(idx.vertex_index) << 32)
                        | (uint64_t(idx.normal_index + 1) << 16)  // +1因为-1=无法线
                        | (uint64_t(idx.texcoord_index + 1));

                    auto it = vertexCache.find(key);
                    if (it != vertexCache.end())
                    {
                        subMesh.indices.push_back(it->second);
                    }
                    else
                    {
                        Vertex3D vert;
                        // position
                        if (idx.vertex_index >= 0)
                            vert.Position = {
                                attrib.vertices[3 * idx.vertex_index],
                                attrib.vertices[3 * idx.vertex_index + 1],
                                attrib.vertices[3 * idx.vertex_index + 2]
                        };
                        // normal
                        if (idx.normal_index >= 0 && !attrib.normals.empty())
                            vert.Normal = {
                                attrib.normals[3 * idx.normal_index],
                                attrib.normals[3 * idx.normal_index + 1],
                                attrib.normals[3 * idx.normal_index + 2]
                        };
                        else
                            vert.Normal = { 0,1,0 };
                        // uv
                        if (idx.texcoord_index >= 0 && !attrib.texcoords.empty())
                            vert.UV = {
                                attrib.texcoords[2 * idx.texcoord_index],
                                attrib.texcoords[2 * idx.texcoord_index + 1]
                        };
                        else
                            vert.UV = { 0,0 };

                        uint32_t newIdx = static_cast<uint32_t>(subMesh.vertices.size());
                        subMesh.indices.push_back(newIdx);
                        vertexCache[key] = newIdx;
                        subMesh.vertices.push_back(vert);
                    }
                }
                indexOffset += fv;
            }

            // 提取材质信息
            if (!shape.mesh.material_ids.empty() && shape.mesh.material_ids[0] >= 0)
            {
                const auto& mat = materials[shape.mesh.material_ids[0]];
                // 漫反射颜色 (RGB)，Alpha 强制设为 1.0
                subMesh.diffuseColor = { mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1.0f };
                // 漫反射贴图路径：拼接上之前提取的 mtlDir
                if (!mat.diffuse_texname.empty())
                    subMesh.textures.push_back({ TextureSemantic::Albedo, mtlDir + mat.diffuse_texname });
            }

            bool hasNormals = false;
            for (const auto& v : subMesh.vertices)
            {
                if (v.Normal != glm::vec3(0, 0, 1)) { hasNormals = true; break; }
            }
            if (!hasNormals && !subMesh.indices.empty())
                GeometryUtils::ComputeSmoothNormals(subMesh.vertices, subMesh.indices);

            GeometryUtils::ApplyCoordinateTransform(subMesh.vertices);

            if (!subMesh.indices.empty())
                GeometryUtils::ComputeTangents(subMesh.vertices, subMesh.indices);

            modelData.subMeshes.push_back(std::move(subMesh));
        }

        return modelData;
    }


    ModelData LoadGLTF(const std::string& filepath)
    {
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err, warn;

        bool ret = false;
        if (filepath.size() >= 5 && filepath.substr(filepath.size() - 5) == ".gltf")
            ret = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
        else if (filepath.size() >= 4 && filepath.substr(filepath.size() - 4) == ".glb")
            ret = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);

        if (!warn.empty()) { std::cout <<"WARN :  " << warn << std::endl; }
        if (!err.empty()) { std::cout << "ERR :  " << err << std::endl; }
        if (!ret) return {};

        // 提取模型目录，用于拼接纹理相对路径
        size_t lastSlash = filepath.find_last_of("/\\");
        std::string modelDir = (lastSlash != std::string::npos)
            ? filepath.substr(0, lastSlash + 1) : "";

        // ---- 步骤1：计算每个节点的世界变换矩阵 ----
        std::vector<glm::mat4> nodeTransforms(model.nodes.size(), glm::mat4(1.0f));
        glm::mat4 yToZ = GeometryUtils::GetYToZMatrix();

        std::function<void(int, const glm::mat4&)> computeTransform =
            [&](int nodeIdx, const glm::mat4& parentTransform)
            {
                const auto& node = model.nodes[nodeIdx];

                // 计算局部变换
                glm::mat4 localTransform(1.0f);
                if (node.matrix.size() == 16)
                {
                    // glTF matrix 是列主序，与 glm::mat4 内存布局一致
                    float m[16];
                    for (int i = 0; i < 16; i++) m[i] = (float)node.matrix[i];
                    memcpy(&localTransform, m, sizeof(glm::mat4));
                }
                else
                {
                    // TRS 组合
                    if (node.translation.size() == 3)
                        localTransform = glm::translate(localTransform,
                            glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
                    if (node.rotation.size() == 4)
                    {
                        // glTF 四元数 [x,y,z,w]，glm::quat(w,x,y,z)
                        glm::quat q(
                            (float)node.rotation[3],  // w
                            (float)node.rotation[0],  // x
                            (float)node.rotation[1],  // y
                            (float)node.rotation[2]); // z
                        localTransform *= glm::mat4_cast(q);
                    }
                    if (node.scale.size() == 3)
                        localTransform = glm::scale(localTransform,
                            glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
                }

                // 世界变换 = Y→Z转换 × 父变换 × 局部变换
                // yToZ 放最前面，把整个 Y-up 场景旋转到 Z-up
                glm::mat4 worldTransform = yToZ * parentTransform * localTransform;
                nodeTransforms[nodeIdx] = worldTransform;

                for (int child : node.children)
                    computeTransform(child, parentTransform * localTransform);
                // 注意：子节点递归时传入的是不含 yToZ 的世界变换，
                // 因为 yToZ 会在子节点的 worldTransform 计算中再次应用
            };

		// 只加载默认场景（如果有），或者第一个场景
        if (!model.scenes.empty())
        {
            int sceneIdx = (model.defaultScene >= 0) ? model.defaultScene : 0;
            for (int rootNode : model.scenes[sceneIdx].nodes)
                computeTransform(rootNode, glm::mat4(1.0f));
        }

        // ---- 步骤2：遍历引用 mesh 的节点，提取图元数据 ----
        ModelData modelData;
        // TUDO: 子mesh的位置可能会要根据父mesh做变换
        for (size_t ni = 0; ni < model.nodes.size(); ni++)
        {
            const auto& node = model.nodes[ni];
            if (node.mesh < 0) continue;

            const auto& gltfMesh = model.meshes[node.mesh];
            glm::mat4 worldTransform = nodeTransforms[ni];
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldTransform)));

            for (size_t p = 0; p < gltfMesh.primitives.size(); p++)
            {
                const auto& prim = gltfMesh.primitives[p];
                SubMeshData subMesh;
                subMesh.name = gltfMesh.name.empty()
                    ? ("mesh" + std::to_string(node.mesh))
                    : gltfMesh.name;
                if (gltfMesh.primitives.size() > 1)
                    subMesh.name += "_prim" + std::to_string(p);

                // --- 位置 ---
                // 做整个后续的指导
                auto posIt = prim.attributes.find("POSITION");
                if (posIt == prim.attributes.end()) continue;

                int stride; size_t count;
                auto posBuffer = GeometryUtils::GetAccessorData(model, posIt->second, stride, count);
                const unsigned char* posData = posBuffer.data();
                subMesh.vertices.resize(count);

                for (size_t v = 0; v < count; v++)
                {
					const float* f = reinterpret_cast<const float*>(posData + v * stride);          // 直接用float*访问？
                    glm::vec3 localPos(f[0], f[1], f[2]);
                    subMesh.vertices[v].Position = glm::vec3(worldTransform * glm::vec4(localPos, 1.0f));
                }

                // --- 法线 ---
                auto normIt = prim.attributes.find("NORMAL");
                bool hasNormals = (normIt != prim.attributes.end());

                if (hasNormals)
                {
                    int normStride; size_t normCount;
                    auto normBuffer = GeometryUtils::GetAccessorData(model, normIt->second, normStride, normCount);
                    const unsigned char* normData = normBuffer.data();

                    for (size_t v = 0; v < normCount && v < count; v++)
                    {
                        const float* f = reinterpret_cast<const float*>(normData + v * normStride);     // 直接用float*访问？
                        glm::vec3 localNorm(f[0], f[1], f[2]);
                        subMesh.vertices[v].Normal = glm::normalize(normalMatrix * localNorm);
                    }
                }
                else
                {
                    for (size_t v = 0; v < count; v++)
                        subMesh.vertices[v].Normal = { 0, 0, 1 };
                }

                // --- UV ---
                auto uvIt = prim.attributes.find("TEXCOORD_0");
				bool hasUVs = (uvIt != prim.attributes.end());

                if (hasUVs)
                {
                    int uvStride; size_t uvCount;
                    auto uvBuffer = GeometryUtils::GetAccessorData(model, uvIt->second, uvStride, uvCount);
                    const unsigned char* uvData = uvBuffer.data();

                    for (size_t v = 0; v < uvCount && v < count; v++)
                    {
                        const float* f = reinterpret_cast<const float*>(uvData + v * uvStride);
                        // glTF UV (0,0) = 图片左上角，OpenGL 纹理 (0,0) = 左下角
                        // stb 已翻转图片，需翻转 V 使两者对齐
                        subMesh.vertices[v].UV = { f[0], 1.0f - f[1] };
                    }
                }
                else
                {
                    for (size_t v = 0; v < count; v++)
                        subMesh.vertices[v].UV = { 0, 0 };
                }

                // --- 切线 ---
                auto tanIt = prim.attributes.find("TANGENT");
                bool hasTangents = (tanIt != prim.attributes.end());

                if (hasTangents)
                {
                    int tanStride; size_t tanCount;
                    auto tanBuffer = GeometryUtils::GetAccessorData(model, tanIt->second, tanStride, tanCount);
                    const unsigned char* tanData = tanBuffer.data();

                    for (size_t v = 0; v < tanCount && v < count; v++)
                    {
                        const float* f = reinterpret_cast<const float*>(tanData + v * tanStride);
                        glm::vec3 localTan(f[0], f[1], f[2]);       // glTF TANGENT.xyz
                        float handedness = f[3];                    // glTF TANGENT.w (1.0 或 -1.0)

                        glm::vec3 worldTan = glm::normalize(normalMatrix * localTan);
                        // 重正交化
                        worldTan = glm::normalize(worldTan - subMesh.vertices[v].Normal * glm::dot(worldTan, subMesh.vertices[v].Normal));
                        subMesh.vertices[v].Tangent = glm::vec4(worldTan, handedness);
                    }
                }

                // --- 索引 ---
                if (prim.indices >= 0)
                {
                    int idxStride; size_t idxCount;
                    auto idxBuffer = GeometryUtils::GetAccessorData(model, prim.indices, idxStride, idxCount);
                    const unsigned char* idxData = idxBuffer.data();

                    subMesh.indices.resize(idxCount);
                    for (size_t i = 0; i < idxCount; i++)
                    {
                        const auto& idxAcc = model.accessors[prim.indices];
                        if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                            subMesh.indices[i] = *(idxData + i * idxStride);
                        else if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                            subMesh.indices[i] = *reinterpret_cast<const uint16_t*>(idxData + i * idxStride);
                        else if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                            subMesh.indices[i] = *reinterpret_cast<const uint32_t*>(idxData + i * idxStride);
                    }
                }
                else
                {
                    // 无索引，生成顺序索引
                    subMesh.indices.resize(count);
                    for (size_t i = 0; i < count; i++)
                        subMesh.indices[i] = (uint32_t)i;
                }

                // --- 图元模式展开 ---
                int mode = (prim.mode == -1) ? TINYGLTF_MODE_TRIANGLES : prim.mode;
                if (mode == TINYGLTF_MODE_TRIANGLE_STRIP)
                    GeometryUtils::ExpandStripToTriangles(subMesh.indices);
                else if (mode == TINYGLTF_MODE_TRIANGLE_FAN)
                    GeometryUtils::ExpandFanToTriangles(subMesh.indices);
                // TINYGLTF_MODE_TRIANGLES (4) 无需转换
                // POINTS / LINES / LINE_LOOP / LINE_STRIP 暂不支持

                // 无法线时自动计算
                if (!hasNormals && !subMesh.indices.empty())
                    GeometryUtils::ComputeSmoothNormals(subMesh.vertices, subMesh.indices);

                // 无切线时自动计算
                if (!hasTangents && !subMesh.indices.empty())
                    GeometryUtils::ComputeTangents(subMesh.vertices, subMesh.indices);

                // --- 材质 ---
                if (prim.material >= 0)
                {
                    const auto& mat = model.materials[prim.material];

                    // 金属 - 粗糙度 PBR 核心参数对象
                    const auto& pbr = mat.pbrMetallicRoughness;

                    // 基础颜色因子（默认 {1,1,1,1}）
                    subMesh.diffuseColor = {
                        (float)pbr.baseColorFactor[0],
                        (float)pbr.baseColorFactor[1],
                        (float)pbr.baseColorFactor[2],
                        (float)pbr.baseColorFactor[3]
                    };

                    subMesh.metallicFactor = (float)pbr.metallicFactor;
                    subMesh.roughnessFactor = (float)pbr.roughnessFactor;
                    subMesh.emissiveFactor = {
                        (float)mat.emissiveFactor[0],
                        (float)mat.emissiveFactor[1],
                        (float)mat.emissiveFactor[2]
                    };

                    if (mat.alphaMode == "MASK")       subMesh.alphaMode = 1;
                    else if (mat.alphaMode == "BLEND") subMesh.alphaMode = 2;
                    subMesh.alphaCutoff = (float)mat.alphaCutoff;
                    subMesh.doubleSided = mat.doubleSided;

                    // Albedo（漫反射/基础颜色纹理）
                    if (pbr.baseColorTexture.index >= 0)
                    {
                        std::string path = GeometryUtils::RegisterTexture(model, modelDir, pbr.baseColorTexture.index, true);
                        if (!path.empty())
                            subMesh.textures.push_back({ TextureSemantic::Albedo, path ,GeometryUtils::GetSamplerInfo(model,pbr.baseColorTexture.index, true) });
                    }

                    // Normal（法线纹理）
                    if (mat.normalTexture.index >= 0)
                    {
                        std::string path = GeometryUtils::RegisterTexture(model, modelDir, mat.normalTexture.index, false);
                        if (!path.empty())
                            subMesh.textures.push_back({ TextureSemantic::Normal, path ,GeometryUtils::GetSamplerInfo(model,mat.normalTexture.index, false) });
                    }

                    // MetallicRoughness（金属度/粗糙度纹理）
                    if (pbr.metallicRoughnessTexture.index >= 0)
                    {
                        std::string path = GeometryUtils::RegisterTexture(model, modelDir, pbr.metallicRoughnessTexture.index, false);
                        if (!path.empty())
                            subMesh.textures.push_back({ TextureSemantic::MetallicRoughness, path ,GeometryUtils::GetSamplerInfo(model,pbr.metallicRoughnessTexture.index, false) });
                    }

                    // Emissive（自发光纹理）
                    if (mat.emissiveTexture.index >= 0)
                    {
                        std::string path = GeometryUtils::RegisterTexture(model, modelDir, mat.emissiveTexture.index, true);
                        if (!path.empty())
                            subMesh.textures.push_back({ TextureSemantic::Emissive, path ,GeometryUtils::GetSamplerInfo(model,mat.emissiveTexture.index, true) });
                    }

                    // Occlusion (AO，环境光遮蔽纹理)
                    if (mat.occlusionTexture.index >= 0)
                    {
                        std::string path = GeometryUtils::RegisterTexture(model, modelDir, mat.occlusionTexture.index, false);
                        if (!path.empty())
                            subMesh.textures.push_back({ TextureSemantic::Occlusion, path ,GeometryUtils::GetSamplerInfo(model,mat.occlusionTexture.index, false) });
                    }
                }

                modelData.subMeshes.push_back(std::move(subMesh));
            }
        }

        return modelData;
    }


    ModelData LoadModel(const std::string& filepath)
    {
        if (filepath.size() >= 4 && filepath.substr(filepath.size() - 4) == ".obj")
            return LoadOBJ(filepath);
        else if ((filepath.size() >= 5 && filepath.substr(filepath.size() - 5) == ".gltf") || (filepath.size() >= 4 && filepath.substr(filepath.size() - 4) == ".glb"))
				return LoadGLTF(filepath);
        return {};
    }

    GLTFScene LoadGLTFScene(const std::string& filepath)
    {
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err, warn;

        bool ret = false;
        if (filepath.size() >= 5 && filepath.substr(filepath.size() - 5) == ".gltf")
            ret = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
        else if (filepath.size() >= 4 && filepath.substr(filepath.size() - 4) == ".glb")
            ret = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);

        if (!warn.empty()) { std::cout << "WARN :  " << warn << std::endl; }
        if (!err.empty()) { std::cout << "ERR :  " << err << std::endl; }
        if (!ret) return {};

        size_t lastSlash = filepath.find_last_of("/\\");
        std::string modelDir = (lastSlash != std::string::npos)
            ? filepath.substr(0, lastSlash + 1) : "";

        GLTFScene scene;
        scene.nodes.resize(model.nodes.size());

        // ---- 步骤1：遍历所有节点，提取局部变换 + 层级关系 ----
        for (size_t ni = 0; ni < model.nodes.size(); ni++)
        {
            const auto& node = model.nodes[ni];
            auto& nodeData = scene.nodes[ni];
            nodeData.name = node.name;
            nodeData.meshIndex = node.mesh;

            // 子节点索引
            for (int child : node.children)
                nodeData.children.push_back(child);

            // 局部变换：优先保留 TRS
            if (node.matrix.size() == 16)
            {
                nodeData.useMatrix = true;
                float m[16];
                for (int i = 0; i < 16; i++) m[i] = (float)node.matrix[i];
                memcpy(&nodeData.matrix, m, sizeof(glm::mat4));
            }
            else
            {
                if (node.translation.size() == 3)
                    nodeData.translation = {
                        (float)node.translation[0],
                        (float)node.translation[1],
                        (float)node.translation[2]
                };
                if (node.rotation.size() == 4)
                    // glm::quat 构造函数是(w, x, y, z)
                    nodeData.rotation = glm::quat(
                        (float)node.rotation[3],  // w
                        (float)node.rotation[0],  // x
                        (float)node.rotation[1],  // y
                        (float)node.rotation[2]); // z
                if (node.scale.size() == 3)
                    nodeData.scale = {
                        (float)node.scale[0],
                        (float)node.scale[1],
                        (float)node.scale[2]
                };
            }

            // ---- 步骤2：提取该节点的 mesh 数据（顶点保持局部空间）----
            if (node.mesh < 0) continue;
            const auto& gltfMesh = model.meshes[node.mesh];

            for (size_t p = 0; p < gltfMesh.primitives.size(); p++)
            {
                const auto& prim = gltfMesh.primitives[p];
                SubMeshData subMesh;
                subMesh.name = gltfMesh.name.empty()
                    ? ("mesh" + std::to_string(node.mesh))
                    : gltfMesh.name;
                if (gltfMesh.primitives.size() > 1)
                    subMesh.name += "_prim" + std::to_string(p);

                // --- 位置（局部空间，不烘焙变换）---
                auto posIt = prim.attributes.find("POSITION");
                if (posIt == prim.attributes.end()) continue;

                int stride; size_t count;
                auto posBuffer = GeometryUtils::GetAccessorData(model, posIt->second, stride, count);
                const unsigned char* posData = posBuffer.data();
                subMesh.vertices.resize(count);

                for (size_t v = 0; v < count; v++)
                {
                    const float* f = reinterpret_cast<const float*>(posData + v * stride);
                    subMesh.vertices[v].Position = { f[0], f[1], f[2] };
                }

                // --- 法线（局部空间）---
                auto normIt = prim.attributes.find("NORMAL");
                bool hasNormals = (normIt != prim.attributes.end());

                if (hasNormals)
                {
                    int normStride; size_t normCount;
                    auto normBuffer = GeometryUtils::GetAccessorData(model, normIt->second, normStride, normCount);
                    const unsigned char* normData = normBuffer.data();
                    for (size_t v = 0; v < normCount && v < count; v++)
                    {
                        const float* f = reinterpret_cast<const float*>(normData + v * normStride);
                        subMesh.vertices[v].Normal = { f[0], f[1], f[2] };
                    }
                }
                else
                {
                    for (size_t v = 0; v < count; v++)
                        subMesh.vertices[v].Normal = { 0, 0, 1 };
                }

                // --- UV ---
                auto uvIt = prim.attributes.find("TEXCOORD_0");
                bool hasUVs = (uvIt != prim.attributes.end());

                if (hasUVs)
                {
                    int uvStride; size_t uvCount;
                    auto uvBuffer = GeometryUtils::GetAccessorData(model, uvIt->second, uvStride, uvCount);
                    const unsigned char* uvData = uvBuffer.data();
                    for (size_t v = 0; v < uvCount && v < count; v++)
                    {
                        const float* f = reinterpret_cast<const float*>(uvData + v * uvStride);
                        // glTF UV 原点在左上角，OpenGL 在左下角，须翻转 Y 轴
                        subMesh.vertices[v].UV = { f[0], 1.0f - f[1] };
                    }
                }
                else
                {
                    for (size_t v = 0; v < count; v++)
                        subMesh.vertices[v].UV = { 0, 0 };
                }

                // --- 切线（局部空间）---
                auto tanIt = prim.attributes.find("TANGENT");
                bool hasTangents = (tanIt != prim.attributes.end());

                if (hasTangents)
                {
                    int tanStride; size_t tanCount;
                    auto tanBuffer = GeometryUtils::GetAccessorData(model, tanIt->second, tanStride, tanCount);
                    const unsigned char* tanData = tanBuffer.data();
                    for (size_t v = 0; v < tanCount && v < count; v++)
                    {
                        const float* f = reinterpret_cast<const float*>(tanData + v * tanStride);
                        subMesh.vertices[v].Tangent = { f[0], f[1], f[2], f[3] };
                    }
                }

                // --- 索引 ---
                if (prim.indices >= 0)
                {
                    int idxStride; size_t idxCount;
                    auto idxBuffer = GeometryUtils::GetAccessorData(model, prim.indices, idxStride, idxCount);
                    const unsigned char* idxData = idxBuffer.data();
                    subMesh.indices.resize(idxCount);
                    for (size_t i = 0; i < idxCount; i++)
                    {
                        const auto& idxAcc = model.accessors[prim.indices];
                        if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                            subMesh.indices[i] = *(idxData + i * idxStride);
                        else if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                            subMesh.indices[i] = *reinterpret_cast<const uint16_t*>(idxData + i * idxStride);
                        else if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                            subMesh.indices[i] = *reinterpret_cast<const uint32_t*>(idxData + i * idxStride);
                    }
                }
                else
                {
                    subMesh.indices.resize(count);
                    for (size_t i = 0; i < count; i++)
                        subMesh.indices[i] = (uint32_t)i;
                }

                // --- 图元模式展开 ---
                int mode = (prim.mode == -1) ? TINYGLTF_MODE_TRIANGLES : prim.mode;
                if (mode == TINYGLTF_MODE_TRIANGLE_STRIP)
                    GeometryUtils::ExpandStripToTriangles(subMesh.indices);
                else if (mode == TINYGLTF_MODE_TRIANGLE_FAN)
                    GeometryUtils::ExpandFanToTriangles(subMesh.indices);

                if (!hasNormals && !subMesh.indices.empty())
                    GeometryUtils::ComputeSmoothNormals(subMesh.vertices, subMesh.indices);
                if (!hasTangents && !subMesh.indices.empty())
                    GeometryUtils::ComputeTangents(subMesh.vertices, subMesh.indices);

                // --- 材质 ---
                if (prim.material >= 0)
                {
                    const auto& mat = model.materials[prim.material];
                    const auto& pbr = mat.pbrMetallicRoughness;

                    subMesh.diffuseColor = {
                        (float)pbr.baseColorFactor[0],
                        (float)pbr.baseColorFactor[1],
                        (float)pbr.baseColorFactor[2],
                        (float)pbr.baseColorFactor[3]
                    };
                    subMesh.metallicFactor = (float)pbr.metallicFactor;
                    subMesh.roughnessFactor = (float)pbr.roughnessFactor;
                    subMesh.emissiveFactor = {
                        (float)mat.emissiveFactor[0],
                        (float)mat.emissiveFactor[1],
                        (float)mat.emissiveFactor[2]
                    };
                    if (mat.alphaMode == "MASK")       subMesh.alphaMode = 1;
                    else if (mat.alphaMode == "BLEND") subMesh.alphaMode = 2;
                    subMesh.alphaCutoff = (float)mat.alphaCutoff;
                    subMesh.doubleSided = mat.doubleSided;

                    if (pbr.baseColorTexture.index >= 0)
                    {
                        std::string path = GeometryUtils::RegisterTexture(model, modelDir, pbr.baseColorTexture.index,
                            true);
                        if (!path.empty())
                            subMesh.textures.push_back({ TextureSemantic::Albedo, path,
                                GeometryUtils::GetSamplerInfo(model, pbr.baseColorTexture.index, true) });
                    }
                    if (mat.normalTexture.index >= 0)
                    {
                        std::string path = GeometryUtils::RegisterTexture(model, modelDir, mat.normalTexture.index,
                            false);
                        if (!path.empty())
                            subMesh.textures.push_back({ TextureSemantic::Normal, path,
                                GeometryUtils::GetSamplerInfo(model, mat.normalTexture.index, false) });
                    }
                    if (pbr.metallicRoughnessTexture.index >= 0)
                    {
                        std::string path = GeometryUtils::RegisterTexture(model, modelDir,
                            pbr.metallicRoughnessTexture.index, false);
                        if (!path.empty())
                            subMesh.textures.push_back({ TextureSemantic::MetallicRoughness, path,
                                GeometryUtils::GetSamplerInfo(model, pbr.metallicRoughnessTexture.index, false) });
                    }
                    if (mat.emissiveTexture.index >= 0)
                    {
                        std::string path = GeometryUtils::RegisterTexture(model, modelDir, mat.emissiveTexture.index,
                            true);
                        if (!path.empty())
                            subMesh.textures.push_back({ TextureSemantic::Emissive, path,
                                GeometryUtils::GetSamplerInfo(model, mat.emissiveTexture.index, true) });
                    }
                    if (mat.occlusionTexture.index >= 0)
                    {
                        std::string path = GeometryUtils::RegisterTexture(model, modelDir, mat.occlusionTexture.index,
                            false);
                        if (!path.empty())
                            subMesh.textures.push_back({ TextureSemantic::Occlusion, path,
                                GeometryUtils::GetSamplerInfo(model, mat.occlusionTexture.index, false) });
                    }
                }

                nodeData.subMeshes.push_back(std::move(subMesh));
            }
        }

        // ---- 步骤3：记录场景根节点 ----
        if (!model.scenes.empty())
        {
            int sceneIdx = (model.defaultScene >= 0) ? model.defaultScene : 0;
            for (int rootNode : model.scenes[sceneIdx].nodes)
                scene.rootNodes.push_back(rootNode);
        }

        return scene;
    }

}
