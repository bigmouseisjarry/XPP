#define GLM_ENABLE_EXPERIMENTAL
#include "EntityFactory.h"
#include "ComGameplay.h"
#include "ComTransform.h"
#include "ComCamera.h"
#include "ComPhysics.h"
#include "ComRender.h"
#include "ModelLoader.h"
#include "Framebuffer.h"
#include <gtx/matrix_decompose.hpp>

entt::entity EntityFactory::CreatePlayer(entt::registry& registry)
{
    auto entity = registry.create();
    registry.emplace<TagComponent>(entity, TagComponent{ .name = "Player" });

    auto& transform = registry.emplace<Transform2DComponent>(entity,
        Transform2DComponent{ .position = {0.0f, 0.0f}, .scale = {32.0f, 32.0f} });

    registry.emplace<Physics2DComponent>(entity,
        Physics2DComponent{ .colliderSize = {32.0f, 32.0f} });

    auto& visible = registry.emplace<Visible2DComponent>(entity);
    visible.mesh = ResourceManager::Get()->GetMeshID("quad");
    visible.material = std::make_unique<Material>();
    visible.material->m_Shader = ResourceManager::Get()->GetShaderID("Shader2.glsl");
    visible.material->SetTexture(TextureSemantic::Albedo, ResourceManager::Get()->GetTextureID("resources/idle/leftidle.png"));
	auto& anim = registry.emplace<AnimatedSpriteComponent>(entity);
	anim.m_FrameDur = 0.1f;

    visible.material->Set("u_Color", glm::vec4(1.0f));
    visible.material->m_Transparent = true;
    visible.renderLayer = RenderLayer::RQ_WorldObjects;

    registry.emplace<PlayerComponent>(entity);
    registry.emplace<DestroyComponent>(entity);
	
    return entity;
}

entt::entity EntityFactory::CreateCamera2D(entt::registry& registry, float left, float right, float bottom, float top, const std::string& name, std::span<const RenderLayer> layers)
{
    auto entity = registry.create();
    registry.emplace<TagComponent>(entity, TagComponent{ .name = name });
    auto& transform = registry.emplace<Transform2DComponent>(entity);
    transform.SetPosition({ 0.0f, 0.0f });
    auto& cam = registry.emplace<Camera2DComponent>(entity);
    cam.SetOrtho(left, right, bottom, top);
    cam.name = name;
    cam.targetLayers.assign(layers.begin(), layers.end());
    return entity;
}

entt::entity EntityFactory::CreateCamera3D(entt::registry& registry, float fov, float aspect, float nearPlane, float farPlane, const std::string& name, std::span<const RenderLayer> layers)
{
    auto entity = registry.create();
    registry.emplace<TagComponent>(entity, TagComponent{ .name = name });
    registry.emplace<Transform3DComponent>(entity);
    auto& cam = registry.emplace<Camera3DComponent>(entity);
    cam.SetPerspective(fov, aspect, nearPlane, farPlane);
    cam.name = name;
    cam.targetLayers.assign(layers.begin(), layers.end());
    cam.UpdateCameraVectors();
    cam.UpdatePosition(); // 设置 isDirty，CameraSystem 会在第一次 Update 时同步 Transform3D
    return entity;
}

// TODO: 名字应该由外部传入，或者自动生成一个唯一名字
entt::entity EntityFactory::CreateLight3D(entt::registry& registry, const glm::vec3& position, const glm::vec3& color, float intensity, bool castShadow)
{
    auto entity = registry.create();
    registry.emplace<TagComponent>(entity, TagComponent{ .name = "Light3D" });
	registry.emplace<Transform3DComponent>(entity, Transform3DComponent{ .position = position }); // 这两个position字段有点冗余，后续可以考虑合并
   
    auto& light = registry.emplace<Light3DComponent>(entity, Light3DComponent{
        .position = position,
        .color = color,
        .intensity = intensity,
        .castShadow = castShadow
        });

    return entity;
}

entt::entity EntityFactory::CreateModel3D(entt::registry& registry, const std::string& filepath)
{
    auto entity = registry.create();
    registry.emplace<TagComponent>(entity, TagComponent{ .name = "Model3D" });

    // 加载模型数据
    ModelData modelData = ModelLoader::LoadModel(filepath);

    glm::vec3 boundsMin(FLT_MAX), boundsMax(-FLT_MAX);
    Model3DComponent modelComp;
    
    for (size_t i = 0; i < modelData.subMeshes.size(); i++)
    {
        const auto& subData = modelData.subMeshes[i];

        for (const auto& v : subData.vertices)
        {
            boundsMin = glm::min(boundsMin, v.Position);
            boundsMax = glm::max(boundsMax, v.Position);
        }

        std::string meshName = filepath + "_sub" + std::to_string(i);
        MeshID meshID = ResourceManager::Get()->CreateMesh<Vertex3D>(meshName, subData.vertices, subData.indices);

        auto mat = std::make_unique<Material>();
        mat->m_Shader = ResourceManager::Get()->GetShaderID("Shader3D.glsl");

        for(const  auto& ti : subData.textures)
        {
            TextureID texID;
            if (ti.sampler.valid)
                texID = ResourceManager::Get()->LoadTexture(
                    ti.path, 1, 1, ti.sampler.minFilter, ti.sampler.magFilter,
                    ti.sampler.wrapS, ti.sampler.wrapT, ti.sampler.sRGB);
            else
                texID = ResourceManager::Get()->LoadTexture(ti.path);
            mat->SetTexture(ti.semantic, texID);
        }

        if (mat->GetTexture(TextureSemantic::Albedo).value == INVALID_ID)
            mat->SetTexture(TextureSemantic::Albedo, ResourceManager::Get()->GetTextureID("resources/white.png"));

        mat->Set("u_Color", subData.diffuseColor);
        mat->Set("u_MetallicFactor", subData.metallicFactor);
        mat->Set("u_RoughnessFactor", subData.roughnessFactor);
        mat->Set("u_EmissiveFactor", subData.emissiveFactor);

        if (subData.alphaMode == 2)       // BLEND
            mat->m_Transparent = true;
        if (subData.alphaMode == 1)       // MASK
            mat->Set("u_AlphaCutoff", subData.alphaCutoff);

        mat->m_DoubleSided = subData.doubleSided;
        modelComp.subMeshes.emplace_back(meshID, std::move(mat));
    }

    auto& physics = registry.emplace<Physics3DComponent>(entity);
    physics.isGrounded = false;
    physics.colliderSize = boundsMax - boundsMin;
    physics.colliderOffset = (boundsMin + boundsMax) * 0.5f;

    registry.emplace<Transform3DComponent>(entity);
    registry.emplace<Model3DComponent>(entity, std::move(modelComp));
    registry.emplace<DestroyComponent>(entity);

    return entity;
}

entt::entity EntityFactory::CreateCube(entt::registry& registry)
{
    auto entity = registry.create();
    registry.emplace<TagComponent>(entity, TagComponent{ .name = "Cube" });

    auto& transform = registry.emplace<Transform3DComponent>(entity,
        Transform3DComponent{ .position = {0.7f, 0.0f, 0.0f}, .scale = {1.0f, 1.0f, 1.0f} });

    auto& visible = registry.emplace<Visible3DComponent>(entity);
    visible.mesh = ResourceManager::Get()->GetMeshID("cube3d");
    visible.material = std::make_unique<Material>();
    visible.material->m_Shader = ResourceManager::Get()->GetShaderID("Shader3D.glsl");
	visible.material->SetTexture(TextureSemantic::Albedo, ResourceManager::Get()->GetTextureID("resources/white.png"));
    visible.material->Set("u_Color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    visible.renderLayer = RenderLayer::RQ_WorldObjects;

    auto& physics = registry.emplace<Physics3DComponent>(entity);
	physics.colliderSize = glm::vec3(1.0f, 1.0f, 1.0f);
    physics.isStatic = true;

    // registry.emplace<CubeComponent>(entity);
    registry.emplace<DestroyComponent>(entity);

    return entity;
}

entt::entity EntityFactory::CreateAxis(entt::registry& registry)
{
	auto entity = registry.create();
	registry.emplace<TagComponent>(entity, TagComponent{ .name = "Axis" });

	auto& transform = registry.emplace<Transform3DComponent>(entity,
        Transform3DComponent{ .position = {0.0f, 0.0f, 0.0f}, .scale = {1.0f, 1.0f, 1.0f} });

	auto& visible = registry.emplace<Visible3DComponent>(entity);
    visible.mesh = ResourceManager::Get()->GetMeshID("Axis");
    visible.material = std::make_unique<Material>();
    visible.material->m_Shader = ResourceManager::Get()->GetShaderID("ShaderLine.glsl");
	visible.material->SetTexture(TextureSemantic::Albedo, ResourceManager::Get()->GetTextureID("resources/white.png"));
    visible.material->Set("u_Color", glm::vec4(1.0f, 0.5f, 0.3f, 1.0f));
    visible.material->m_Transparent = true;
    visible.renderLayer = RenderLayer::RQ_WorldBackground;

    registry.emplace<DestroyComponent>(entity);

    return entity;
}

entt::entity EntityFactory::CreateModelHierarchy(entt::registry& registry, const std::string& filepath)
{
    GLTFScene scene = ModelLoader::LoadGLTFScene(filepath);

    std::vector<entt::entity> entities(scene.nodes.size(), entt::entity{});

    // 第一轮：创建实体 + Transform（直接使用 TRS，仅 useMatrix 时分解）
    for (size_t i = 0; i < scene.nodes.size(); i++)
    {
        const auto& node = scene.nodes[i];
        auto entity = registry.create();
        entities[i] = entity;

        registry.emplace<TagComponent>(entity, TagComponent{ .name = node.name });
        auto& transform = registry.emplace<Transform3DComponent>(entity);

        if (node.useMatrix)
        {
            // 从矩阵分解 TRS
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::quat tempRotation = node.rotation;
            glm::decompose(node.matrix, transform.scale, tempRotation, transform.position, skew, perspective);
            transform.rotation = glm::eulerAngles(tempRotation) * (180.0f / glm::pi<float>());
        }
        else
        {
            transform.position = node.translation;
            transform.rotation = glm::eulerAngles(node.rotation) * (180.0f / glm::pi<float>());
            transform.scale = node.scale;
        }
        transform.isDirty = true;
    }

    // 第二轮：设置 parent/children 关系
    for (size_t i = 0; i < scene.nodes.size(); i++)
    {
        auto& transform = registry.get<Transform3DComponent>(entities[i]);
        for (int childIdx : scene.nodes[i].children)
        {
            auto& childTransform = registry.get<Transform3DComponent>(entities[childIdx]);
            childTransform.parent = entities[i];
            transform.children.push_back(entities[childIdx]);
        }
    }

    // 第三轮：为有 mesh 的节点挂载 Model3DComponent
    for (size_t i = 0; i < scene.nodes.size(); i++)
    {
        const auto& node = scene.nodes[i];
        if (node.subMeshes.empty()) continue;

        Model3DComponent modelComp;
        for (size_t si = 0; si < node.subMeshes.size(); si++)
        {
            const auto& subData = node.subMeshes[si];
            std::string meshName = filepath + "_node" + std::to_string(i) + "_sub" + std::to_string(si);
            MeshID meshID = ResourceManager::Get()->CreateMesh<Vertex3D>(meshName, subData.vertices, subData.indices);

            auto mat = std::make_unique<Material>();
            mat->m_Shader = ResourceManager::Get()->GetShaderID("Shader3D.glsl");

            for (const auto& ti : subData.textures)
            {
                TextureID texID;
                if (ti.sampler.valid)
                    texID = ResourceManager::Get()->LoadTexture(
                        ti.path, 1, 1, ti.sampler.minFilter, ti.sampler.magFilter,
                        ti.sampler.wrapS, ti.sampler.wrapT, ti.sampler.sRGB);
                else
                    texID = ResourceManager::Get()->LoadTexture(ti.path);
                mat->SetTexture(ti.semantic, texID);
            }
            if (mat->GetTexture(TextureSemantic::Albedo).value == INVALID_ID)
                mat->SetTexture(TextureSemantic::Albedo, ResourceManager::Get()->GetTextureID("resources/white.png"));

            mat->Set("u_Color", subData.diffuseColor);
            mat->Set("u_MetallicFactor", subData.metallicFactor);
            mat->Set("u_RoughnessFactor", subData.roughnessFactor);
            mat->Set("u_EmissiveFactor", subData.emissiveFactor);
            if (subData.alphaMode == 2) mat->m_Transparent = true;
            if (subData.alphaMode == 1) mat->Set("u_AlphaCutoff", subData.alphaCutoff);
            mat->m_DoubleSided = subData.doubleSided;

            modelComp.subMeshes.emplace_back(meshID, std::move(mat));
        }
        registry.emplace<Model3DComponent>(entities[i], std::move(modelComp));
        registry.emplace<DestroyComponent>(entities[i]);
    }

    // 创建 Y→Z 转换根节点
    auto y2zEntity = registry.create();
    registry.emplace<TagComponent>(y2zEntity, TagComponent{ .name = "Y2Z_Root" });
    auto& y2zTransform = registry.emplace<Transform3DComponent>(y2zEntity);
    y2zTransform.SetRotation({ -90.0f, 0.0f, 0.0f });

    // 将场景根节点挂在 Y2Z 下
    for (int rootIdx : scene.rootNodes)
    {
        auto& rootTransform = registry.get<Transform3DComponent>(entities[rootIdx]);
        rootTransform.parent = y2zEntity;
        y2zTransform.children.push_back(entities[rootIdx]);
    }

    return y2zEntity;
}
