#pragma once
#include "Vertex.h"
#include <vector>
#include <gtc/matrix_transform.hpp>
#include "tiny_gltf.h"
#include "ResourceManager.h"

namespace GeometryUtils
{
    // 模型文件中可能没有法线数据，或者我们想要重新计算平滑法线，这个函数可以根据顶点和索引计算出每个顶点的平均法线
    static void ComputeSmoothNormals(std::vector<Vertex3D>& vertices, const std::vector<unsigned int>& indices)
    {
        // 累加面法线到每个顶点
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            glm::vec3& v0 = vertices[indices[i]].Position;
            glm::vec3& v1 = vertices[indices[i + 1]].Position;
            glm::vec3& v2 = vertices[indices[i + 2]].Position;

            glm::vec3 faceNormal = glm::cross(v1 - v0, v2 - v0);

            vertices[indices[i]].Normal += faceNormal;
            vertices[indices[i + 1]].Normal += faceNormal;
            vertices[indices[i + 2]].Normal += faceNormal;
        }

        // 归一化
        for (auto& v : vertices)
            v.Normal = glm::normalize(v.Normal);
    }

    // Y-up → Z-up 坐标转换矩阵（绕 X 轴旋转 -90°）
    static glm::mat4 GetYToZMatrix()
    {
        return glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));
    }
	// 法线变换矩阵是坐标变换矩阵的逆转置
    static glm::mat3 GetYToZNormalMatrix()
    {
        return glm::transpose(glm::inverse(glm::mat3(GetYToZMatrix())));
    }

    // 对顶点数据应用坐标转换
    static void ApplyCoordinateTransform(std::vector<Vertex3D>& vertices)
    {
        glm::mat4 yToZ = GetYToZMatrix();
        glm::mat3 normalMat = GetYToZNormalMatrix();
        for (auto& v : vertices)
        {
            v.Position = glm::vec3(yToZ * glm::vec4(v.Position, 1.0f));
            v.Normal = glm::normalize(normalMat * v.Normal);
        }
    }

    // 三角形带 → 三角形列表
    static void ExpandStripToTriangles(std::vector<uint32_t>& indices)
    {
        if (indices.size() < 3) return;
        std::vector<uint32_t> triangles;
        triangles.reserve((indices.size() - 2) * 3);
        for (size_t i = 2; i < indices.size(); i++)
        {
            if (i % 2 == 0)
                // 偶数位置：保持顺序 [i-2, i-1, i]
                triangles.insert(triangles.end(), { indices[i - 2], indices[i - 1], indices[i] });
            else
                // 奇数位置：交换前两个顶点 [i-1, i-2, i]
                triangles.insert(triangles.end(), { indices[i - 1], indices[i - 2], indices[i] });
        }
        indices = std::move(triangles);
    }

    // 三角形扇 → 三角形列表
    static void ExpandFanToTriangles(std::vector<uint32_t>& indices)
    {
        if (indices.size() < 3) return;
        std::vector<uint32_t> triangles;
        triangles.reserve((indices.size() - 2) * 3);
        for (size_t i = 2; i < indices.size(); i++)
            triangles.insert(triangles.end(), { indices[0], indices[i - 1], indices[i] });
        indices = std::move(triangles);
    }

    // 获取 accessor 中每个元素的字节大小
    static int GetComponentSize(int componentType)
    {
        switch (componentType)
        {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return 1;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return 2;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: return 4;
        case TINYGLTF_COMPONENT_TYPE_FLOAT: return 4;
        default: return 0;
        }
    }

    static int GetTypeComponents(int type)
    {
        switch (type)
        {
        case TINYGLTF_TYPE_SCALAR: return 1;
        case TINYGLTF_TYPE_VEC2: return 2;
        case TINYGLTF_TYPE_VEC3: return 3;
        case TINYGLTF_TYPE_VEC4: return 4;
        case TINYGLTF_TYPE_MAT2: return 4;
        case TINYGLTF_TYPE_MAT3: return 9;
        case TINYGLTF_TYPE_MAT4: return 16;
        default: return 0;
        }
    }

    // 从 accessor 获取数据指针和步长
    static const unsigned char* GetAccessorPtr(const tinygltf::Model& model, int accessorIdx, int& stride, size_t& count)
    {
        const auto& acc = model.accessors[accessorIdx];
        const auto& bv = model.bufferViews[acc.bufferView];
        const auto& buf = model.buffers[bv.buffer];
        count = acc.count;
        int accStride = acc.ByteStride(bv);
        // ByteStride 返回 0 表示紧密打包，用元素大小作为步长
        stride = (accStride > 0) ? accStride
            : GetComponentSize(acc.componentType) * GetTypeComponents(acc.type);
        return buf.data.data() + bv.byteOffset + acc.byteOffset;
    }

    // TUDO: 可以先检查 acc.sparse.isSparse，如果为 false 就直接返回指向原始 buffer 的 span
    static std::vector<unsigned char> GetAccessorData( const tinygltf::Model& model, int accessorIdx, int& stride, size_t& count)
    {
        const auto& acc = model.accessors[accessorIdx];
        const auto& bv = model.bufferViews[acc.bufferView];
        const auto& buf = model.buffers[bv.buffer];
        count = acc.count;

        int accStride = acc.ByteStride(bv);
        stride = (accStride > 0) ? accStride
            : GetComponentSize(acc.componentType) * GetTypeComponents(acc.type);

        size_t totalBytes = count * stride;
        std::vector<unsigned char> result(totalBytes);

        // 复制 dense 数据
        const unsigned char* src = buf.data.data() + bv.byteOffset + acc.byteOffset;
        memcpy(result.data(), src, totalBytes);

        // 应用 sparse overlay
        if (acc.sparse.isSparse)
        {
            const auto& sparse = acc.sparse;

            // sparse indices
            const auto& idxBV = model.bufferViews[sparse.indices.bufferView];
            const auto& idxBuf = model.buffers[idxBV.buffer];
            const unsigned char* idxData = idxBuf.data.data()
                + idxBV.byteOffset + sparse.indices.byteOffset;

            // sparse values
            const auto& valBV = model.bufferViews[sparse.values.bufferView];
            const auto& valBuf = model.buffers[valBV.buffer];
            const unsigned char* valData = valBuf.data.data()
                + valBV.byteOffset + sparse.values.byteOffset;

            for (size_t i = 0; i < sparse.count; i++)
            {
                size_t targetIdx = 0;
                switch (sparse.indices.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    targetIdx = idxData[i]; break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    targetIdx = reinterpret_cast<const uint16_t*>(idxData)[i]; break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    targetIdx = reinterpret_cast<const uint32_t*>(idxData)[i]; break;
                }
                memcpy(result.data() + targetIdx * stride,
                    valData + i * stride, stride);
            }
        }
        return result;
    }

    // 根据位置、UV 和索引计算切线和副切线（Lengyel 算法）
    static void ComputeTangents(std::vector<Vertex3D>& vertices, const std::vector<unsigned int>& indices)
    {
        std::vector<glm::vec3> tempBitangent(vertices.size(), glm::vec3(0.0f));

        for (size_t i = 0; i < indices.size(); i += 3)
        {
            unsigned int i0 = indices[i];
            unsigned int i1 = indices[i + 1];
            unsigned int i2 = indices[i + 2];

            glm::vec3& p0 = vertices[i0].Position;
            glm::vec3& p1 = vertices[i1].Position;
            glm::vec3& p2 = vertices[i2].Position;

            glm::vec2& uv0 = vertices[i0].UV;
            glm::vec2& uv1 = vertices[i1].UV;
            glm::vec2& uv2 = vertices[i2].UV;

            glm::vec3 edge1 = p1 - p0;
            glm::vec3 edge2 = p2 - p0;
            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;

            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

            glm::vec3 tangent(
                (edge1.x * deltaUV2.y - edge2.x * deltaUV1.y) * r,
                (edge1.y * deltaUV2.y - edge2.y * deltaUV1.y) * r,
                (edge1.z * deltaUV2.y - edge2.z * deltaUV1.y) * r
            );
            glm::vec3 bitangent(
                (edge2.x * deltaUV1.x - edge1.x * deltaUV2.x) * r,
                (edge2.y * deltaUV1.x - edge1.y * deltaUV2.x) * r,
                (edge2.z * deltaUV1.x - edge1.z * deltaUV2.x) * r
            );

            vertices[i0].Tangent += glm::vec4(tangent, 0.0f);
            vertices[i1].Tangent += glm::vec4(tangent, 0.0f);
            vertices[i2].Tangent += glm::vec4(tangent, 0.0f);
			
			tempBitangent[i0] += bitangent;
			tempBitangent[i1] += bitangent;
            tempBitangent[i2] += bitangent;
        }

        for (size_t i = 0;i < vertices.size(); i++)
        {
            glm::vec3 T = glm::vec3(vertices[i].Tangent);
            if (T != glm::vec3(0.0f))
            {
				T = glm::normalize(T - vertices[i].Normal * glm::dot(T, vertices[i].Normal));
                glm::vec3 B = glm::cross(vertices[i].Normal, T);
                // handedness: 累加的 bitangent 和 cross(N,T) 同向 → 1，反向 → -1
                float handedness = (glm::dot(B, tempBitangent[i]) >= 0.0f) ? 1.0f : -1.0f;
				vertices[i].Tangent = glm::vec4(T, handedness);
            }
        }
    }

    static SamplerInfo GetSamplerInfo(const tinygltf::Model& model,int texIndex, bool sRGB)
    {
        if (texIndex < 0) return {};
        const auto& gltfTex = model.textures[texIndex];
        if (gltfTex.sampler < 0) 
        {
            SamplerInfo info;
            info.sRGB = sRGB;
            info.valid = true;
            return info;
        }
        const auto& s = model.samplers[gltfTex.sampler];
        SamplerInfo info;
        info.valid = true;
        info.sRGB = sRGB;
        if (s.magFilter >= 0) info.magFilter = s.magFilter;
        if (s.minFilter >= 0) info.minFilter = s.minFilter;
        if (s.wrapS >= 0)     info.wrapS = s.wrapS;
        if (s.wrapT >= 0)     info.wrapT = s.wrapT;
        return info;
    }

    static std::vector<unsigned char> Base64Decode(const std::string& input)
    {
        static const int table[256] = { /* -1 填充 */ };
        // 建表
        static bool init = false;
        static int lookup[256];
        if (!init)
        {
            memset(lookup, -1, sizeof(lookup));
            const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            for (int i = 0; chars[i]; i++)
                lookup[(unsigned char)chars[i]] = i;
            init = true;
        }

        std::vector<unsigned char> result;
        result.reserve(input.size() * 3 / 4);

        int val = 0, valb = -8;
        for (unsigned char c : input)
        {
            if (lookup[c] == -1) break;
            val = (val << 6) + lookup[c];
            valb += 6;
            if (valb >= 0)
            {
                result.push_back(static_cast<unsigned char>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return result;
    }

    static std::string RegisterTexture(const tinygltf::Model& model,const std::string& modelDir,int texIndex, bool sRGB)
    {
        if (texIndex < 0) return "";
        const auto& gltfTex = model.textures[texIndex];
        int imgIdx = gltfTex.source;
        if (imgIdx < 0) return "";
        const auto& img = model.images[imgIdx];

        // Case 1: 外部文件 URI
        if (!img.uri.empty() && img.uri.substr(0, 5) != "data:")
            return modelDir + img.uri;

        // Case 2: bufferView 嵌入
        if (img.bufferView >= 0)
        {
            const auto& bv = model.bufferViews[img.bufferView];
            const auto& buf = model.buffers[bv.buffer];
            const unsigned char* data = buf.data.data() + bv.byteOffset;
            int len = (int)bv.byteLength;

            std::string name = "__embedded__" + std::to_string(imgIdx);
            auto* rm = ResourceManager::Get();
            if (rm->GetTextureID(name).value != INVALID_ID)
                return name;

            int minF = GL_LINEAR, magF = GL_LINEAR, wrapS = GL_CLAMP_TO_EDGE, wrapT = GL_CLAMP_TO_EDGE;
            if (gltfTex.sampler >= 0)
            {
                const auto& s = model.samplers[gltfTex.sampler];
                if (s.minFilter >= 0) minF = s.minFilter;
                if (s.magFilter >= 0) magF = s.magFilter;
                if (s.wrapS >= 0)     wrapS = s.wrapS;
                if (s.wrapT >= 0)     wrapT = s.wrapT;
            }

            rm->CreateTextureFromMemory(name, data, len, minF, magF, wrapS, wrapT, sRGB);
            return name;
        }

        // Case 3: data: URI（base64 编码）
        if (!img.uri.empty() && img.uri.substr(0, 5) == "data:")
        {
            std::string name = "__embedded__" + std::to_string(imgIdx);
            auto* rm = ResourceManager::Get();
            if (rm->GetTextureID(name).value != INVALID_ID)
                return name;

            size_t commaPos = img.uri.find(',');
            if (commaPos == std::string::npos) return "";
            std::string base64 = img.uri.substr(commaPos + 1);
            std::vector<unsigned char> decoded = Base64Decode(base64);

            int minF = GL_LINEAR, magF = GL_LINEAR, wrapS = GL_CLAMP_TO_EDGE, wrapT = GL_CLAMP_TO_EDGE;
            if (gltfTex.sampler >= 0)
            {
                const auto& s = model.samplers[gltfTex.sampler];
                if (s.minFilter >= 0) minF = s.minFilter;
                if (s.magFilter >= 0) magF = s.magFilter;
                if (s.wrapS >= 0)     wrapS = s.wrapS;
                if (s.wrapT >= 0)     wrapT = s.wrapT;
            }

            rm->CreateTextureFromMemory(name, decoded.data(), (int)decoded.size(), minF, magF, wrapS, wrapT, sRGB);
            return name;
        }

        return "";
    }

}