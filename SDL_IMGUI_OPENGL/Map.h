#pragma once
#include "ResourceManager.h"
#include "Renderer.h"
#include "PhysicsSystem.h"

struct BackImage
{
	TextureID m_TextureID;
	int m_RenderOrder = 0;		//数字越小，越在前面渲染，越容易被遮挡
	glm::vec2 m_Size;           // 图片尺寸（可用于视口裁剪）

	Material* m_Material;
	glm::mat4 m_ModelMatrix;
};

class Map
{
private:
	std::vector<BackImage> m_BackImages;
	RenderLayer m_RenderLayer;
	glm::vec2 m_Position = glm::vec2(0.0f);  // 地图位置
	std::vector<entt::entity> m_ColliderEntities;  // 碰撞体
public:
	Map() = default;
	Map(RenderLayer renderlayer);

	void Init(const std::string& folderPath, entt::registry& registry);
	void AddBackImage(const std::string& filepath,int renderorder);
	void AddCollider(const glm::vec2& pos, const glm::vec2& size, entt::registry& registry);
	void SetPosition(const glm::vec2& pos) { m_Position = pos; }
	void OnRender() const;
private:
	// 从文件名中提取数字（用于排序）
	static int __ExtractNumberFromFilename(const std::string& filename);
};