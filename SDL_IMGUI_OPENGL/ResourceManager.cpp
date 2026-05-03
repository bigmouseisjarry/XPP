#include "ResourceManager.h"
#include <ext/scalar_constants.hpp>

ResourceManager* ResourceManager::Get()
{
    static ResourceManager instance;
    return &instance;
}

void ResourceManager::Init()
{

    // 注册 shader，返回 ID
    LoadShader("Shader.glsl");
    LoadShader("Shader2.glsl");
    LoadShader("Shader3.glsl");
    LoadShader("Shader3D.glsl");
    LoadShader("ShaderLine.glsl");
    LoadShader("ShaderSkybox.glsl");
    LoadShader("ShaderShadow.glsl");
    LoadShader("ShaderComposite.glsl");
    LoadShader("ShaderPostProcess.glsl");
    LoadShader("ShaderBloomBright.glsl");
    LoadShader("ShaderBloomBlur.glsl");
    LoadShader("ShaderBloomUp.glsl");
    LoadShader("ShaderComposite.glsl");
    LoadShader("ShaderSSAO.glsl");
    LoadShader("ShaderSSAOBlur.glsl");
	LoadShader("ShaderFXAA.glsl");
    LoadShader("ShaderParticle.glsl");

    // 注册 texture
    LoadTexture("resources/idle/leftidle.png", 10, 1);
    LoadTexture("resources/idle/rightidle.png", 10, 1);
    LoadTexture("resources/walk/leftwalk.png", 4, 6);
    LoadTexture("resources/walk/rightwalk.png", 4, 6);
    LoadTexture("resources/walk/leftfromidle.png", 2, 1);
    LoadTexture("resources/walk/rightfromidle.png", 2, 1);
    LoadTexture("resources/testimage.png");
    LoadTexture("resources/map1.png");
    LoadTexture("resources/white.png");
    LoadTexture("resources/map/BGBack.png");
    LoadTexture("resources/map/BGFront.png");
    LoadTexture("resources/map/CloudsBack.png");
    LoadTexture("resources/map/CloudsFront.png");
    LoadTexture("resources/map/Tileset.png");
    LoadTexture("resources/map/TilesExamples.png");
    LoadTexture("resources/map/Trees.png");

	LoadDefaultTexture("u_AlbedoMap", 1.0f, 1.0f, 1.0f, 1.0f);
	LoadDefaultTexture("u_NormalMap", 0.5f, 0.5f, 1.0f, 1.0f);
	LoadDefaultTexture("u_MetallicRoughnessMap", 0.0f, 0.0f, 0.0f, 1.0f);
	LoadDefaultTexture("u_EmissiveMap", 0.0f, 0.0f, 0.0f, 1.0f);
	LoadDefaultTexture("u_OcclusionMap", 1.0f, 1.0f, 1.0f, 1.0f);
    LoadDefaultTexture("u_DepthMap", 1.0f, 1.0f, 1.0f, 1.0f);
    LoadDefaultTexture("u_WorldNormalMap", 0.0f, 1.0f, 0.0f, 1.0f);
    LoadDefaultTexture("u_BloomBrightMap", 0.0f, 0.0f, 0.0f, 1.0f);
    LoadDefaultTexture("u_SSAOMap", 1.0f, 1.0f, 1.0f, 1.0f);
    LoadDefaultTexture("u_BloomMap", 0.0f, 0.0f, 0.0f, 1.0f);
    CreateNoiseTexture("u_NoiseMap", 4, 4);

    LoadHDRTexture("resources/skybox/grasslands_sunset_4k.hdr");
    LoadHDRTexture("resources/skybox/moonless_golf_4k.hdr");



    {
        // 注册默认 quad mesh
        std::vector<Vertex> quadVerts = {
            {glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec2(0.0f, 0.0f)},
            {glm::vec3(0.5f, -0.5f, 0.0f), glm::vec2(1.0f, 0.0f)},
            {glm::vec3(0.5f,  0.5f, 0.0f), glm::vec2(1.0f, 1.0f)},
            {glm::vec3(-0.5f,  0.5f, 0.0f), glm::vec2(0.0f, 1.0f)}
        };
        std::vector<unsigned int> quadIndices = { 0, 1, 2, 2, 3, 0 };
        CreateMesh("quad", quadVerts, quadIndices);
    }

    {
        // 立方体顶点数据（带法线）
        std::vector<Vertex3D> vertices = {
            // 前面 (Z+)  T=(1,0,0), Handedness=1.0
            {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},

            // 后面 (Z-)  T=(-1,0,0), Handedness=1.0
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f,-1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f,-1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f,-1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f,-1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},

            // 左面 (X-)  T=(0,0,-1), Handedness=1.0
            {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f,-1.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f,-1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f,-1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f,-1.0f, 1.0f}},

            // 右面 (X+)  T=(0,0,1), Handedness=1.0
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},

            // 上面 (Y+)  T=(1,0,0), Handedness=1.0
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},

            // 下面 (Y-)  T=(1,0,0), Handedness=1.0
            {{-0.5f, -0.5f, -0.5f}, {0.0f,-1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f,-1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f,-1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f}, {0.0f,-1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        };

        // 索引数据
        std::vector<unsigned int> indices = {
            0, 1, 2,  2, 3, 0,      // 前面
            4, 5, 6,  6, 7, 4,      // 后面
            8, 9, 10, 10, 11, 8,    // 左面
            12, 13, 14, 14, 15, 12, // 右面
            16, 17, 18, 18, 19, 16, // 上面
            20, 21, 22, 22, 23, 20  // 下面
        };

        CreateMesh("cube3d", vertices, indices);
    }

    {
        std::vector<VertexLine> vertices = {
        { glm::vec3(-100.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)},
        { glm::vec3(100.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) },
        { glm::vec3(0.0f,-100.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(0.0f, 100.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(0.0f, 0.0f,-100.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) },
        { glm::vec3(0.0f, 0.0f, 100.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) }
        };
        std::vector<unsigned int> indices = {
            0, 1, // X 轴
            2, 3, // Y 轴
            4, 5  // Z 轴
        };
        CreateMesh("Axis", vertices, indices);
    }

    {
        std::vector<Vertex> fsQuadVerts = {
            {glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
            {glm::vec3(1.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
            {glm::vec3(1.0f,  1.0f, 0.0f), glm::vec2(1.0f, 1.0f)},
            {glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec2(0.0f, 1.0f)}
        };
        std::vector<unsigned int> fsQuadIndices = { 0, 1, 2, 2, 3, 0 };
        CreateMesh("fullscreenQuad", fsQuadVerts, fsQuadIndices);
    }

    {
        glm::vec4 debugColor = glm::vec4(1.0f, 0.12f, 0.12f, 1.0f);
        std::vector<VertexLine> vertices = {
            { glm::vec3(-0.5f, -0.5f, -0.5f), debugColor }, // 0
            { glm::vec3(0.5f, -0.5f, -0.5f), debugColor }, // 1
            { glm::vec3(0.5f,  0.5f, -0.5f), debugColor }, // 2
            { glm::vec3(-0.5f,  0.5f, -0.5f), debugColor }, // 3
            { glm::vec3(-0.5f, -0.5f,  0.5f), debugColor }, // 4
            { glm::vec3(0.5f, -0.5f,  0.5f), debugColor }, // 5
            { glm::vec3(0.5f,  0.5f,  0.5f), debugColor }, // 6
            { glm::vec3(-0.5f,  0.5f,  0.5f), debugColor }, // 7
        };
        std::vector<unsigned int> indices = {
            0,1, 1,2, 2,3, 3,0,  // back
            4,5, 5,6, 6,7, 7,4,  // front
            0,4, 1,5, 2,6, 3,7   // connecting
        };
        CreateMesh("DebugColliderBox", vertices, indices);
    }

    {
        std::vector<VertexLine> vertices;
        std::vector<unsigned int> indices;
        glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 0.3f, 0.6f);

        const int rings = 3;
        const int segsPerRing = 32;
        const float r = 1.0f;

        // 纬度环（水平）
        for (int i = 1; i < rings; i++) {
            float phi = glm::pi<float>() * float(i) / float(rings);
            unsigned int base = (unsigned int)vertices.size();
            for (int j = 0; j < segsPerRing; j++) {
                float theta = 2.0f * glm::pi<float>() * float(j) / float(segsPerRing);
                vertices.push_back({
                    glm::vec3(r * sin(phi) * cos(theta),
                              r * cos(phi),
                              r * sin(phi) * sin(theta)),
                    lightColor
                    });
            }
            for (int j = 0; j < segsPerRing; j++) {
                indices.push_back(base + j);
                indices.push_back(base + (j + 1) % segsPerRing);
            }
        }

        // 经度环（垂直）
        for (int i = 0; i < rings; i++) {
            float theta = 2.0f * glm::pi<float>() * float(i) / float(rings);
            unsigned int base = (unsigned int)vertices.size();
            for (int j = 0; j <= segsPerRing / 2; j++) {
                float phi = glm::pi<float>() * float(j) / (segsPerRing / 2);
                vertices.push_back({
                    glm::vec3(r * sin(phi) * cos(theta),
                              r * cos(phi),
                              r * sin(phi) * sin(theta)),
                    lightColor
                    });
            }
            for (int j = 0; j < segsPerRing / 2; j++) {
                indices.push_back(base + j);
                indices.push_back(base + j + 1);
            }
        }

        {
            unsigned char pixels[32 * 32 * 4];
            for (int y = 0; y < 32; y++)
            {
                for (int x = 0; x < 32; x++)
                {
                    float dx = (x - 15.5f) / 15.5f;
                    float dy = (y - 15.5f) / 15.5f;
                    float dist = sqrtf(dx * dx + dy * dy);
                    float alpha = std::max(0.0f, 1.0f - dist);
                    int i = (y * 32 + x) * 4;
                    pixels[i] = 255;
                    pixels[i + 1] = 255;
                    pixels[i + 2] = 255;
                    pixels[i + 3] = (unsigned char)(alpha * 255.0f);
                }
            }
            CreateTextureFromPixels("defaultParticle", pixels, 32, 32, 4,
                GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, false);
        }


        CreateMesh("DebugWireSphere", vertices, indices);
    }

    {
        std::vector<VertexLine> vertices;
        std::vector<unsigned int> indices;
        glm::vec4 c = glm::vec4(1.0f, 1.0f, 0.2f, 0.7f); // 黄色半透明

        // 近平面四角（Z=0，光源平面）
        vertices.push_back({ glm::vec3(-1, -1,  0), c }); // 0
        vertices.push_back({ glm::vec3(1, -1,  0), c }); // 1
        vertices.push_back({ glm::vec3(1,  1,  0), c }); // 2
        vertices.push_back({ glm::vec3(-1,  1,  0), c }); // 3

        // 箭头：中心 → 尖端
        vertices.push_back({ glm::vec3(0,  0,  0), c }); // 4  中心
        vertices.push_back({ glm::vec3(0,  0, -3), c }); // 5  箭头尖
        vertices.push_back({ glm::vec3(-.3, 0, -2.6), c }); // 6 箭尾左
        vertices.push_back({ glm::vec3(.3, 0, -2.6), c }); // 7 箭尾右
        vertices.push_back({ glm::vec3(0,-.3, -2.6), c }); // 8 箭尾下
        vertices.push_back({ glm::vec3(0, .3, -2.6), c }); // 9 箭尾上

        // 近平面四条边
        indices.insert(indices.end(), { 0,1, 1,2, 2,3, 3,0 });
        // 箭头主干
        indices.insert(indices.end(), { 4,5 });
        // 箭头头部
        indices.insert(indices.end(), { 5,6, 5,7, 5,8, 5,9 });

        CreateMesh("DebugLightFrustum", vertices, indices);
    }

}

ShaderID ResourceManager::LoadShader(const std::string& filePath)
{
    ShaderID id{ static_cast<uint32_t>(m_Shaders.size()) };
    m_Shaders.push_back(std::make_unique<Shader>(filePath));
    m_ShaderNameToID[filePath] = id;
    return id;
}

TextureID ResourceManager::LoadTexture(const std::string& path, int cols, int rows)
{
    auto it = m_TextureNameToID.find(path);
    if (it != m_TextureNameToID.end()) return it->second;

    TextureID id{ static_cast<uint32_t>(m_Textures.size()) };
    m_Textures.push_back(std::make_unique<Texture>(path, cols, rows));
    m_TextureNameToID[path] = id;
    return id;
}

TextureID ResourceManager::LoadDefaultTexture(const std::string& name, float r, float g, float b, float a)
{
    TextureID id{ static_cast<uint32_t>(m_Textures.size()) };
    m_Textures.emplace_back(std::unique_ptr<Texture>(new Texture(r, g, b, a)));
    m_TextureNameToID[name] = id;
    return id;
}

TextureID ResourceManager::LoadHDRTexture(const std::string& name)
{
    TextureID id{ static_cast<uint32_t>(m_Textures.size()) };
    m_Textures.push_back(std::make_unique<Texture>(name, true));
    m_TextureNameToID[name] = id;
    return id;
}

TextureID ResourceManager::CreateNoiseTexture(const std::string& name, int width, int height)
{
    // 生成随机半球采样向量
    std::vector<float> data(width * height * 4);
    for (int i = 0; i < width * height; i++)
    {
        // 在切线空间单位半球内随机采样
        float x = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        float y = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        float z = (float)rand() / (float)RAND_MAX;  // z > 0，保证在半球内
        glm::vec3 v = glm::normalize(glm::vec3(x, y, z));

        data[i * 4 + 0] = v.x;
        data[i * 4 + 1] = v.y;
        data[i * 4 + 2] = v.z;
        data[i * 4 + 3] = 0.0f;
    }

    TextureID id{ static_cast<uint32_t>(m_Textures.size()) };
    m_Textures.emplace_back(std::unique_ptr<Texture>(new Texture(data.data(), width, height)));
    m_TextureNameToID[name] = id;
    return id;
}

TextureID ResourceManager::CreateTextureFromPixels(const std::string& name, unsigned char* pixels, int width, int height, int channels, int minFilter, int magFilter, int wrapS, int wrapT, bool sRGB)
{
    auto it = m_TextureNameToID.find(name);
    if (it != m_TextureNameToID.end()) return it->second;

    TextureID id{ static_cast<uint32_t>(m_Textures.size()) };
    m_Textures.emplace_back(std::make_unique<Texture>(pixels, width, height, channels, minFilter, magFilter, wrapS, wrapT, sRGB));
    m_TextureNameToID[name] = id;
    return id;
}

MeshID ResourceManager::CreateInstancedMesh(const std::string& name, const Mesh& sharedMesh, std::unique_ptr<VertexBuffer> instanceVBO, const VertexBufferLayout& instanceLayout, unsigned int maxInstances)
{
    auto it = m_MeshNameToID.find(name);
    if (it != m_MeshNameToID.end())
        return it->second;

    auto mesh = std::make_unique<Mesh>(
        sharedMesh, std::move(instanceVBO), instanceLayout, maxInstances);

    MeshID id{ static_cast<uint32_t>(m_Meshes.size()) };
    m_Meshes.push_back(std::move(mesh));
    m_MeshNameToID[name] = id;
    return id;
}

FramebufferID ResourceManager::CreateFramebuffer(const std::string& name, int width, int height)
{
    FramebufferSpec spec;
    spec.width = width;
    spec.height = height;
    spec.depthAttachment = { AttachmentFormat::Depth24 };
    spec.depthClampToBorder = true;
    return CreateFramebuffer(name, spec);
}

FramebufferID ResourceManager::CreateFramebuffer(const std::string& name, const FramebufferSpec& spec)
{
    auto it = m_FramebufferNameToID.find(name);
    if (it != m_FramebufferNameToID.end()) return it->second;

    FramebufferID id{ static_cast<uint32_t>(m_Framebuffers.size()) };
    m_Framebuffers.emplace_back(std::make_unique<Framebuffer>(spec));
    m_FramebufferNameToID[name] = id;
    return id;

}
MeshID ResourceManager::CreateMeshFromBlob(const std::string& name, const std::vector<uint8_t>& vertexBlob, size_t vertexSize, const std::vector<unsigned int>& indices, VertexType vertexType)
{
    auto it = m_MeshNameToID.find(name);
    if (it != m_MeshNameToID.end()) return it->second;
 
    auto mesh = std::make_unique<Mesh>(vertexBlob.data(), vertexBlob.size(), indices.data(), vertexType, indices.size());
    MeshID id{ static_cast<uint32_t>(m_Meshes.size()) };
    m_Meshes.push_back(std::move(mesh));
    m_MeshNameToID[name] = id;
    return id;
}
ShaderID ResourceManager::GetShaderID(const std::string& name) const
{
    auto it = m_ShaderNameToID.find(name);
    return (it != m_ShaderNameToID.end()) ? it->second : ShaderID{ INVALID_ID };
}

TextureID ResourceManager::GetTextureID(const std::string& name) const
{
    auto it = m_TextureNameToID.find(name);
    return (it != m_TextureNameToID.end()) ? it->second : TextureID{ INVALID_ID };
}

MeshID ResourceManager::GetMeshID(const std::string& name) const
{
    auto it = m_MeshNameToID.find(name);
    return (it != m_MeshNameToID.end()) ? it->second : MeshID{ INVALID_ID };
}

FramebufferID ResourceManager::GetFramebufferID(const std::string& name) const
{
    auto it = m_FramebufferNameToID.find(name);
	return (it != m_FramebufferNameToID.end()) ? it->second : FramebufferID{ INVALID_ID };
}

const Shader* ResourceManager::GetShader(ShaderID id) const
{
    if (id.value >= m_Shaders.size()) return nullptr;
    return m_Shaders[id.value].get();
}

const Texture* ResourceManager::GetTexture(TextureID id) const
{
    if (id.value >= m_Textures.size()) return nullptr;
    return m_Textures[id.value].get();
}

const Framebuffer* ResourceManager::GetFramebuffer(FramebufferID id) const
{
    if (id.value >= m_Framebuffers.size())return nullptr;
    return m_Framebuffers[id.value].get();
}

Framebuffer* ResourceManager::GetFramebufferMut(FramebufferID id)
{
    if (id.value >= m_Framebuffers.size())return nullptr;
    return m_Framebuffers[id.value].get();
}

const Mesh* ResourceManager::GetMesh(MeshID id) const
{
    if (id.value >= m_Meshes.size())return nullptr;
    return m_Meshes[id.value].get();
}

Mesh* ResourceManager::GetMeshMut(MeshID id)
{
    if (id.value >= m_Meshes.size())return nullptr;
    return m_Meshes[id.value].get();
}
