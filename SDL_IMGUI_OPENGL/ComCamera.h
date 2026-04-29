#pragma once
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <span>
#include "Renderer.h"

struct Camera2DComponent {
    glm::mat4 projection = glm::mat4(1.0f);

    mutable bool isDirty = true;

	// 投影矩阵和视图矩阵的乘积
    mutable glm::mat4 projView = glm::mat4(1.0f);

    bool enabled = true;
    std::string name;
    std::vector<RenderLayer> targetLayers;

    // 正交投影参数
    float left = -640.0f;
    float right = 640.0f;
    float bottom = -400.0f;
    float top = 400.0f;

    void SetOrtho(float l, float r, float b, float t) {
        left = l; right = r; bottom = b; top = t;
        projection = glm::ortho(left, right, bottom, top);
        isDirty = true;
    }
};

struct Camera3DComponent {
    // 轨道模型
    glm::vec3 orbitTarget = { 0, 0, 0 };
    float orbitDistance = 3.0f;

    // 投影
    glm::mat4 projection = glm::mat4(1.0f);
	float fov = 45.0f;                          // 视野角
	float aspectRatio = 16.0f / 9.0f;           // 宽高比
    float nearPlane = 0.1f;                     // 近平面
    float farPlane = 100.0f;                    // 远平面

    // 视向量（由 UpdateCameraVectors 计算）
    glm::vec3 front = { 0.0f, 0.0f, -1.0f };
    glm::vec3 up = { 0.0f, 1.0f, 0.0f };
    glm::vec3 right = { 1.0f, 0.0f, 0.0f };

    float pitch = 0.0f;
    float yaw = -90.0f;

    mutable bool isDirty = true;
    mutable glm::mat4 projView = glm::mat4(1.0f);

    bool enabled = true;
    std::string name;
    std::vector<RenderLayer> targetLayers;

	// 设置透视投影参数
    void SetPerspective(float fov_, float aspect, float nearP, float farP) {
        fov = fov_; aspectRatio = aspect; nearPlane = nearP; farPlane = farP;
        projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        isDirty = true;
    }

    // 环绕关注点旋转
    void Orbit(float pitchDelta, float yawDelta) {
        pitch += pitchDelta;
        yaw += yawDelta;
        pitch = glm::clamp(pitch, -89.0f, 89.0f);
        UpdateCameraVectors();
        UpdatePosition(); // 同步到 Transform3DComponent，由 CameraSystem 处理
    }

    // 沿视线推拉
    void Dolly(float delta) {
        orbitDistance -= delta;
        orbitDistance = glm::max(orbitDistance, 0.1f);
        UpdatePosition();
    }

    // 平移
    void Pan(const glm::vec2& delta) {
        orbitTarget += right * delta.x + up * delta.y;
        UpdatePosition();
    }

    void SetOrbitTarget(const glm::vec3& target, float extent = 0.0f) {
        orbitTarget = target;
        if (extent > 0.0f) {
            orbitDistance = extent * 2.5f;
            nearPlane = extent * 0.01f;
            farPlane = extent * 100.0f;
            projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        }
        UpdatePosition();
    }

    void UpdateCameraVectors() {
        glm::vec3 f;
        f.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
        f.y = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.z = sin(glm::radians(pitch));
        front = glm::normalize(f);
		right = glm::normalize(glm::cross(front, glm::vec3(0, 0, 1)));          // 世界坐标系中，Z 轴朝上
        up = glm::normalize(glm::cross(right, front));
    }

    // 返回应写入 Transform3DComponent.position 的值
    glm::vec3 CalcPosition() const {
        return orbitTarget - front * orbitDistance;
    }

    void UpdatePosition() {
        isDirty = true; // 位置变了，projView 需要重算
        // 实际写 Transform3DComponent 由 CameraSystem 统一处理
    }
};