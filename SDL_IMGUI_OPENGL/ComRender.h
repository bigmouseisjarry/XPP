#pragma once
#include <memory>
#include <vector>
#include <string>
#include "ResourceManager.h"
#include "Material.h"
#include "Renderer.h"

struct Visible2DComponent {
    MeshID mesh;
    std::unique_ptr<Material> material;
    RenderLayer renderLayer = RenderLayer::RQ_WorldObjects;
    bool isVisible = true;
};

struct Visible3DComponent {
    MeshID mesh;
    std::unique_ptr<Material> material;
    RenderLayer renderLayer = RenderLayer::RQ_WorldObjects;
    bool isVisible = true;
};

struct Model3DComponent {
    struct SubMesh {
        SubMesh() = default;
        SubMesh(MeshID m, std::unique_ptr<Material> mat) : mesh(m), material(std::move(mat)) {}
        SubMesh(SubMesh&&) = default;
        SubMesh& operator=(SubMesh&&) = default;
        SubMesh(const SubMesh&) = delete;
        SubMesh& operator=(const SubMesh&) = delete;

        MeshID mesh;
        std::unique_ptr<Material> material;
    };
    Model3DComponent() = default;
    Model3DComponent(Model3DComponent&&) = default;
    Model3DComponent& operator=(Model3DComponent&&) = default;
    Model3DComponent(const Model3DComponent&) = delete;
    Model3DComponent& operator=(const Model3DComponent&) = delete;

    RenderLayer renderLayer = RenderLayer::RQ_WorldObjects;
    std::vector<SubMesh> subMeshes;

};

struct AnimatedSpriteComponent
{
    float m_FrameDur = 0.1f;
    int   m_CurrentFrame = 0;
    float m_AnimTimer = 0.0f;
    int   m_LoopCount = 0;

    void ResetLoopCount() { m_LoopCount = 0; }
    int  GetLoopCount() const { return m_LoopCount; }
};