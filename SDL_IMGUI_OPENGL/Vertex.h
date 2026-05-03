#pragma once
#include <glm.hpp>

enum class VertexType
{
	Vertex2D,
	Vertex3D,
	VertexLine,
	VertexParticle
};

struct Vertex
{
	glm::vec3 Position;				// 绘制到屏幕坐标的哪里
	glm::vec2 UV;					// 要绘制原图像的哪一部分x,y的范围都是0到1
};

// T 和 B 是用来 “定位” 法线贴图在模型表面上方向的坐标系轴
struct Vertex3D
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 UV;
	glm::vec4 Tangent = { 0.0f, 0.0f, 0.0f,1.0f };				// xyz=方向, w=handedness
};

struct VertexLine
{
	glm::vec3 Position;
	glm::vec4 Color;
};

struct VertexParticleInstance
{
	glm::vec3 position;     // 世界/屏幕位置
	glm::vec4 color;        // RGBA
	float     size;         // 缩放大小
	float     rotation;     // 旋转角度（弧度）
	glm::vec2 uvOffset;     // 精灵表 UV 偏移
	glm::vec2 uvScale;      // 精灵表 UV 缩放
};