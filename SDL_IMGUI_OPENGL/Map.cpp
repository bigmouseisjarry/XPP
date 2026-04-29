#include "Map.h"
#include<filesystem>
#include <gtc/matrix_transform.hpp>
#include <regex>

Map::Map(RenderLayer renderlayer)
{
	m_RenderLayer = renderlayer;
}



void Map::Init(const std::string& folderPath, entt::registry& registry)
{
    m_BackImages.clear();

    namespace fs = std::filesystem;

    if (!fs::exists(folderPath)) {
        std::cout << "Map: 文件夹不存在 - " << folderPath << std::endl;
        return;
    }

    // 收集所有图片文件
    std::vector<std::pair<std::string, int>> imageFiles;

    for (const auto& entry : fs::directory_iterator(folderPath))
    {
        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".png" || ext == ".jpg")
        {
            std::string filename = entry.path().stem().string();
            int order = __ExtractNumberFromFilename(filename);
            imageFiles.emplace_back(entry.path().string(), order);
        }
    }

    // 按数字排序
    std::sort(imageFiles.begin(), imageFiles.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

    // 使用 AddBackImage 加载
    for (int i = 0; i < imageFiles.size(); ++i)
    {
        AddBackImage(imageFiles[i].first, i);
    }

	AddCollider({ -500.0f, -400.0f }, { 1280.0f, 100.0f }, registry);    // TODO：碰撞体数据也放在文件里
}

void Map::AddBackImage(const std::string& filepath, int renderorder)
{
    TextureID texID = ResourceManager::Get()->LoadTexture(filepath);

    if (texID.value != -1)
    {
        BackImage img;
        img.m_TextureID = texID;
        img.m_RenderOrder = renderorder;

        const Texture* tex = ResourceManager::Get()->GetTexture(texID);
        if (tex) {
            img.m_Size = glm::vec2(tex->GetWidth() * 3, tex->GetHeight() * 3);
        }

        // 初始化缓存的Material
        static ShaderID defaultShader = ResourceManager::Get()->GetShaderID("Shader.glsl");
        img.m_Material = new Material;
        img.m_Material->m_Shader = defaultShader;
		img.m_Material->SetTexture(TextureSemantic::Albedo, texID);
        img.m_Material->Set("u_Color", glm::vec4(1.0f));
		img.m_Material->Set("u_UVscale", glm::vec2(1.0f));  // 默认全图
		img.m_Material->Set("u_UVoffset", glm::vec2(0.0f));  // 默认偏移
        img.m_Material->m_Transparent = true;
        img.m_Material->m_RenderOrder = renderorder;

        // 缓存模型矩阵（只有scale）
        img.m_ModelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(img.m_Size, 1.0f));

        m_BackImages.push_back(std::move(img));

        //// 重新排序
        //std::sort(m_BackImages.begin(), m_BackImages.end(),
        //    [](const BackImage& a, const BackImage& b) {
        //        return a.m_RenderOrder < b.m_RenderOrder;
        //    });
    }
}

void Map::AddCollider(const glm::vec2& pos, const glm::vec2& size, entt::registry& registry)
{
    auto entity = registry.create();
    registry.emplace<Transform2DComponent>(entity,
        Transform2DComponent{ .position = pos });
    registry.emplace<Physics2DComponent>(entity,
        Physics2DComponent{ .colliderSize = size, .isStatic = true });
    m_ColliderEntities.push_back(entity);
}


void Map::OnRender() const
{
    static MeshID defaultMesh = ResourceManager::Get()->GetMeshID("quad");
    glm::mat4 posMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(m_Position, 0.0f));

    for (const auto& img : m_BackImages)
    {
        glm::mat4 model = posMatrix * img.m_ModelMatrix;
        Renderer::Get()->SubmitRenderUnits(defaultMesh, img.m_Material, model, m_RenderLayer);
    }
}

int Map::__ExtractNumberFromFilename(const std::string& filename)
{
    // 方法1：提取所有连续数字
    std::regex pattern("(\\d+)");
    std::smatch match;

    if (std::regex_search(filename, match, pattern))
    {
        return std::stoi(match[1].str());
    }

    // 没找到数字，返回一个很大的值（排到最后）
    return INT_MAX;
}
