#pragma once
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

struct Transform2DComponent {
    glm::vec2 position = { 0.0f, 0.0f };
    glm::vec2 scale = { 1.0f, 1.0f };
    float rotation = 0.0f; // 度

    mutable bool isDirty = true;
    mutable glm::mat4 modelMatrix = glm::mat4(1.0f);

    // 层级字段
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
    mutable bool isWorldDirty = true;
    mutable glm::mat4 worldMatrix = glm::mat4(1.0f);

    // 计算模型矩阵时, 因为矩阵乘法右结合，先进行平移，再进行旋转，最后进行缩放
    const glm::mat4& GetMatrix() const {
        if (isDirty) {
            modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, glm::vec3(position.x, position.y, 0.0f));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            modelMatrix = glm::scale(modelMatrix, glm::vec3(scale.x, scale.y, 1.0f));
            isDirty = false;
        }
        return modelMatrix;
    }

    const glm::mat4& GetWorldMatrix(entt::registry& registry) const {
        if (isWorldDirty) {
            if (parent == entt::null) {
                worldMatrix = GetMatrix();
            }
            else {
                auto& parentT = registry.get<Transform2DComponent>(parent);
                worldMatrix = parentT.GetWorldMatrix(registry) * GetMatrix();
            }
            isWorldDirty = false;
        }
        return worldMatrix;
    }

    void SetPosition(const glm::vec2& v) { position = v; isDirty = true; }
    void SetScale(const glm::vec2& v) { scale = v;    isDirty = true; }
    void SetRotation(float r) { rotation = r;  isDirty = true; }
    void Move(const glm::vec2& delta) { position += delta; isDirty = true; }
};

struct Transform3DComponent {
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
    glm::vec3 rotation = { 0.0f, 0.0f, 0.0f }; // 欧拉角 (pitch, yaw, roll) 度

    mutable bool isDirty = true;
    mutable glm::mat4 modelMatrix = glm::mat4(1.0f);

    // 层级字段
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
    mutable bool isWorldDirty = true;
    mutable glm::mat4 worldMatrix = glm::mat4(1.0f);

	// 旋转顺序：先绕 Y 轴（偏航），再绕 X 轴（俯仰），最后绕 Z 轴（滚转）
	// 计算模型矩阵时, 因为矩阵乘法右结合，先进行平移，再进行旋转，最后进行缩放
    const glm::mat4& GetMatrix() const {
        if (isDirty) {
            modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, position);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
            modelMatrix = glm::scale(modelMatrix, scale);
            isDirty = false;
        }
        return modelMatrix;
    }

    const glm::mat4& GetWorldMatrix(entt::registry& registry) const {
        if (isWorldDirty) {
            if (parent == entt::null) {
                worldMatrix = GetMatrix();
            }
            else {
                auto& parentT = registry.get<Transform3DComponent>(parent);
                worldMatrix = parentT.GetWorldMatrix(registry) * GetMatrix();
            }
            isWorldDirty = false;
        }
        return worldMatrix;
    }

    void SetPosition(const glm::vec3& v) { position = v; isDirty = true; }
    void SetScale(const glm::vec3& v) { scale = v;    isDirty = true; }
    void SetRotation(const glm::vec3& v) { rotation = v; isDirty = true; }
    void Move(const glm::vec3& delta) { position += delta; isDirty = true; }
    void Rotate(const glm::vec3& delta) { rotation += delta; isDirty = true; }
};