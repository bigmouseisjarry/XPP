#pragma once
#include <glm.hpp>
#include "OBB.h"

enum class CollisionLayer : uint32_t {
    Default = 1 << 0,
    Player = 1 << 1,
    Enemy = 1 << 2,
    Projectile = 1 << 3,
    Trigger = 1 << 4,
    Environment = 1 << 5,
};

struct Physics2DComponent {
    glm::vec2 colliderSize = { 0.0f, 0.0f };
    glm::vec2 velocity = { 0.0f, 0.0f };
    bool isStatic = false;
    bool isGrounded = false;
    bool isCollisionEnabled = true;
};


struct Physics3DComponent {
    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };
	glm::vec3 colliderSize = { 0.0f, 0.0f, 0.0f };  // 盒碰撞体尺寸（宽、高、深）
    glm::vec3 colliderOffset = { 0.0f, 0.0f, 0.0f };  // 模型空间中心偏移
    uint32_t collisionLayer = (uint32_t)CollisionLayer::Default;
    uint32_t collisionMask = 0xFFFFFFFF;  // 默认和所有层碰撞
    float friction = 0.5f;      // 0=无摩擦, 1=完全停止
    float restitution = 0.0f;   // 0=无弹, 1=完美弹
    bool isTrigger = false;
    bool isStatic = false;
    bool isGrounded = false;
    bool isCollisionEnabled = true;
};