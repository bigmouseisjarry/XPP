#pragma once
#include <glm.hpp>

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
    bool isStatic = false;
    bool isGrounded = false;
    bool isCollisionEnabled = true;
};