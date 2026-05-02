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
#include <mutex>
#include "WorkerPool.h"
#include "GLCommandQueue.h"
#include "SceneManager.h"

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
    ModelData modelData = ModelLoader::LoadGLTF(filepath);

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
        std::vector<uint8_t> blob(subData.vertices.size() * sizeof(Vertex3D));
        memcpy(blob.data(), subData.vertices.data(), blob.size());
        MeshID meshID = ResourceManager::Get()->CreateMeshFromBlob( meshName, blob, sizeof(Vertex3D), subData.indices, VertexType::Vertex3D);

        auto mat = std::make_unique<Material>();
        mat->m_Shader = ResourceManager::Get()->GetShaderID("Shader3D.glsl");

        for(const  auto& tex : subData.textures)
        {
            TextureID texID = ResourceManager::Get()->CreateTextureFromPixels(
                tex.name, (unsigned char*)tex.pixels.data(), tex.width, tex.height, tex.channels,
                tex.sampler.minFilter, tex.sampler.magFilter,
                tex.sampler.wrapS, tex.sampler.wrapT, tex.sampler.sRGB);
            mat->SetTexture(tex.semantic, texID);
        }

        if (mat->GetTexture(TextureSemantic::Albedo).value == INVALID_ID)
            mat->SetTexture(TextureSemantic::Albedo, ResourceManager::Get()->GetTextureID("resources/white.png"));

        mat->Set("u_Color", subData.diffuseColor);
        mat->Set("u_MetallicFactor", subData.metallicFactor);
        mat->Set("u_RoughnessFactor", subData.roughnessFactor);
        mat->Set("u_EmissiveFactor", subData.emissiveFactor);

        if (subData.alphaMode == 2)mat->m_Transparent = true;                           // BLEND
        if (subData.alphaMode == 1)mat->Set("u_AlphaCutoff", subData.alphaCutoff);       // MASK
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
    glm::vec3 boundsMin(FLT_MAX), boundsMax(-FLT_MAX);
    for (size_t i = 0; i < scene.nodes.size(); i++)
    {
        const auto& node = scene.nodes[i];
        if (node.subMeshes.empty()) continue;


        Model3DComponent modelComp;
        for (size_t si = 0; si < node.subMeshes.size(); si++)
        {
            const auto& subData = node.subMeshes[si];

            for (const auto& v : subData.vertices)
            {
                boundsMin = glm::min(boundsMin, v.Position);
                boundsMax = glm::max(boundsMax, v.Position);
            }

            std::string meshName = filepath + "_node" + std::to_string(i) + "_sub" + std::to_string(si);
            std::vector<uint8_t> blob(subData.vertices.size() * sizeof(Vertex3D));
            memcpy(blob.data(), subData.vertices.data(), blob.size());
            MeshID meshID = ResourceManager::Get()->CreateMeshFromBlob(meshName, blob, sizeof(Vertex3D), subData.indices, VertexType::Vertex3D);

            auto mat = std::make_unique<Material>();
            mat->m_Shader = ResourceManager::Get()->GetShaderID("Shader3D.glsl");

            for (const auto& tex : subData.textures)
            {
                TextureID texID = ResourceManager::Get()->CreateTextureFromPixels(
                    tex.name, (unsigned char*)tex.pixels.data(), tex.width, tex.height, tex.channels,
                    tex.sampler.minFilter, tex.sampler.magFilter,
                    tex.sampler.wrapS, tex.sampler.wrapT, tex.sampler.sRGB);
                mat->SetTexture(tex.semantic, texID);
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
    y2zTransform.SetRotation({ 90.0f, 0.0f, 0.0f });

    auto& y2zPhysics = registry.emplace<Physics3DComponent>(y2zEntity);
    y2zPhysics.colliderSize = boundsMax - boundsMin;
    y2zPhysics.colliderOffset = (boundsMin + boundsMax) * 0.5f;

    // 预烘焙 Y→Z: (x, y, z) → (x, z, -y)
    std::swap(y2zPhysics.colliderSize.y, y2zPhysics.colliderSize.z);
    float oldOffsetY = y2zPhysics.colliderOffset.y;
    y2zPhysics.colliderOffset.y = y2zPhysics.colliderOffset.z;
    y2zPhysics.colliderOffset.z = -oldOffsetY;

    // 将场景根节点挂在 Y2Z 下
    for (int rootIdx : scene.rootNodes)
    {
        auto& rootTransform = registry.get<Transform3DComponent>(entities[rootIdx]);
        rootTransform.parent = y2zEntity;
        y2zTransform.children.push_back(entities[rootIdx]);
    }

    return y2zEntity;
}

struct AsyncLoadTask {
    std::string filepath;
    bool isHierarchy = false;       
    std::atomic<bool> ready{ false };   // GL 命令已全部 Drain 完成

    // CPU 加载结果（工作线程写，主线程读，cpuDone 做同步）
    ModelData cpuModelData;
    GLTFScene cpuSceneData;

    // GPU 命令已推送的标记（避免重复推送）
    bool glCommandsPushed = false;
};

static std::vector<std::shared_ptr<AsyncLoadTask>> s_PendingTasks;
static std::mutex s_TaskMutex;

void EntityFactory::CreateModel3DAsync(const std::string& filepath)
{
    auto task = std::make_shared<AsyncLoadTask>();
    task->filepath = filepath;
    task->isHierarchy = false;

    {
        std::lock_guard lock(s_TaskMutex);
        s_PendingTasks.push_back(task);
    }

    WorkerPool::Get()->Enqueue([task] {
        // ---- 工作线程 ----
        task->cpuModelData = ModelLoader::LoadGLTF(task->filepath);

        int totalCommands = 0;
        for (auto& sub : task->cpuModelData.subMeshes) {
            totalCommands++;  // 1 个 mesh
            totalCommands += (int)sub.textures.size();
        }

        auto commandsLeft = std::make_shared<std::atomic<int>>(totalCommands);

        // 推送 GL 命令
        for (size_t i = 0; i < task->cpuModelData.subMeshes.size(); i++) {
            auto& sub = task->cpuModelData.subMeshes[i];

            // 推送 Mesh
            std::string meshName = task->filepath + "_sub" + std::to_string(i);
            std::vector<uint8_t> blob(sub.vertices.size() * sizeof(Vertex3D));
            memcpy(blob.data(), sub.vertices.data(), blob.size());

            GLCommandQueue::Get()->PushMesh({
                meshName, std::move(blob), sizeof(Vertex3D),
                std::move(sub.indices), VertexType::Vertex3D,
                [task, commandsLeft](MeshID) {
                    if (commandsLeft->fetch_sub(1) == 1)
                          task->ready.store(true);
                }
                });

            // 推送 Texture
            for (auto& tex : sub.textures) {
                // 像素数据从 vector move 进 PendingTexture
                // PendingTexture 的 pixels 字段需要改成 vector<uint8_t>
                GLCommandQueue::Get()->PushTexture({
                    tex.name, tex.width, tex.height, tex.channels,
                    std::move(tex.pixels),
                    tex.sampler.minFilter, tex.sampler.magFilter,
                    tex.sampler.wrapS, tex.sampler.wrapT, tex.sampler.sRGB,
                    [task, commandsLeft](TextureID) {
                           if (commandsLeft->fetch_sub(1) == 1)
                          task->ready.store(true);
                    }
                    });
            }
        }
        });
}

void EntityFactory::CreateModelHierarchyAsync(const std::string& filepath)
{
    auto task = std::make_shared<AsyncLoadTask>();
    task->filepath = filepath;
    task->isHierarchy = true;

    {
        std::lock_guard lock(s_TaskMutex);
        s_PendingTasks.push_back(task);
    }

    WorkerPool::Get()->Enqueue([task] {
        task->cpuSceneData = ModelLoader::LoadGLTFScene(task->filepath);

        int totalCommands = 0;
        for (auto& node : task->cpuSceneData.nodes)
            for (auto& sub : node.subMeshes) {
                totalCommands++;
                totalCommands += (int)sub.textures.size();
            }

        auto commandsLeft = std::make_shared<std::atomic<int>>(totalCommands);

        // 推送 GL 命令
        for (size_t ni = 0; ni < task->cpuSceneData.nodes.size(); ni++) {
            auto& node = task->cpuSceneData.nodes[ni];
            for (size_t si = 0; si < node.subMeshes.size(); si++) {
                auto& sub = node.subMeshes[si];
                std::string meshName = task->filepath + "_node" + std::to_string(ni) + "_sub" + std::to_string(si);
                std::vector<uint8_t> blob(sub.vertices.size() * sizeof(Vertex3D));
                memcpy(blob.data(), sub.vertices.data(), blob.size());
                GLCommandQueue::Get()->PushMesh({
                    meshName, std::move(blob), sizeof(Vertex3D),
                    std::move(sub.indices), VertexType::Vertex3D,
                    [task, commandsLeft](MeshID) {
                        if (commandsLeft->fetch_sub(1) == 1)
                            task->ready.store(true);
                    }
                    });
                for (auto& tex : sub.textures) {
                    GLCommandQueue::Get()->PushTexture({
                        tex.name, tex.width, tex.height, tex.channels,
                        std::move(tex.pixels),
                        tex.sampler.minFilter, tex.sampler.magFilter,
                        tex.sampler.wrapS, tex.sampler.wrapT, tex.sampler.sRGB,
                        [task, commandsLeft](TextureID) {
                            if (commandsLeft->fetch_sub(1) == 1)
                                task->ready.store(true);
                        }
                        });
                }
            }
        }
        });
}

static void BuildModel3DEntity(entt::registry& registry, const ModelData& data, const std::string& filepath) {
    auto entity = registry.create();
    registry.emplace<TagComponent>(entity, TagComponent{ .name = "Model3D" });

    glm::vec3 boundsMin(FLT_MAX), boundsMax(-FLT_MAX);
    Model3DComponent modelComp;

    for (size_t i = 0; i < data.subMeshes.size(); i++) {
        const auto& sub = data.subMeshes[i];
        for (const auto& v : sub.vertices) {
            boundsMin = glm::min(boundsMin, v.Position);
            boundsMax = glm::max(boundsMax, v.Position);
        }

        // Mesh 和 Texture 已经由 GLCommandQueue::Drain 创建好了，按名字查找
        std::string meshName = filepath + "_sub" + std::to_string(i);
        MeshID meshID = ResourceManager::Get()->GetMeshID(meshName);

        auto mat = std::make_unique<Material>();
        mat->m_Shader = ResourceManager::Get()->GetShaderID("Shader3D.glsl");

        for (const auto& tex : sub.textures) {
            TextureID texID = ResourceManager::Get()->GetTextureID(tex.name);
            mat->SetTexture(tex.semantic, texID);
        }
        if (mat->GetTexture(TextureSemantic::Albedo).value == INVALID_ID)
            mat->SetTexture(TextureSemantic::Albedo,
                ResourceManager::Get()->GetTextureID("resources/white.png"));

        mat->Set("u_Color", sub.diffuseColor);
        mat->Set("u_MetallicFactor", sub.metallicFactor);
        mat->Set("u_RoughnessFactor", sub.roughnessFactor);
        mat->Set("u_EmissiveFactor", sub.emissiveFactor);
        if (sub.alphaMode == 2) mat->m_Transparent = true;
        if (sub.alphaMode == 1) mat->Set("u_AlphaCutoff", sub.alphaCutoff);
        mat->m_DoubleSided = sub.doubleSided;

        modelComp.subMeshes.emplace_back(meshID, std::move(mat));
    }

    auto& physics = registry.emplace<Physics3DComponent>(entity);
    physics.isGrounded = false;
    physics.colliderSize = boundsMax - boundsMin;
    physics.colliderOffset = (boundsMin + boundsMax) * 0.5f;

    registry.emplace<Transform3DComponent>(entity);
    registry.emplace<Model3DComponent>(entity, std::move(modelComp));
    registry.emplace<DestroyComponent>(entity);
}

static void BuildHierarchyEntity(entt::registry& registry, const GLTFScene& scene, const std::string& filepath)
{
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
    glm::vec3 boundsMin(FLT_MAX), boundsMax(-FLT_MAX);
    for (size_t i = 0; i < scene.nodes.size(); i++)
    {
        const auto& node = scene.nodes[i];
        if (node.subMeshes.empty()) continue;

        Model3DComponent modelComp;
        for (size_t si = 0; si < node.subMeshes.size(); si++)
        {
            const auto& subData = node.subMeshes[si];

            for (const auto& v : subData.vertices)
            {
                boundsMin = glm::min(boundsMin, v.Position);
                boundsMax = glm::max(boundsMax, v.Position);
            }

            std::string meshName = filepath + "_node" + std::to_string(i) + "_sub" + std::to_string(si);
            MeshID meshID = ResourceManager::Get()->GetMeshID(meshName);

            auto mat = std::make_unique<Material>();
            mat->m_Shader = ResourceManager::Get()->GetShaderID("Shader3D.glsl");

            for (const auto& ti : subData.textures)
            {
                TextureID texID = ResourceManager::Get()->GetTextureID(ti.name);
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

    auto& y2zPhysics = registry.emplace<Physics3DComponent>(y2zEntity);
    y2zPhysics.colliderSize = boundsMax - boundsMin;
    y2zPhysics.colliderOffset = (boundsMin + boundsMax) * 0.5f;

    // 预烘焙 Y→Z: (x, y, z) → (x, z, -y)
    std::swap(y2zPhysics.colliderSize.y, y2zPhysics.colliderSize.z);
    float oldOffsetY = y2zPhysics.colliderOffset.y;
    y2zPhysics.colliderOffset.y = y2zPhysics.colliderOffset.z;
    y2zPhysics.colliderOffset.z = -oldOffsetY;

    // 将场景根节点挂在 Y2Z 下
    for (int rootIdx : scene.rootNodes)
    {
        auto& rootTransform = registry.get<Transform3DComponent>(entities[rootIdx]);
        rootTransform.parent = y2zEntity;
        y2zTransform.children.push_back(entities[rootIdx]);
    }
}

void EntityFactory::ProcessAsyncLoadResults()
{
    GLCommandQueue::Get()->Drain();

    std::lock_guard lock(s_TaskMutex);
    auto& registry = SceneManager::Get()->GetCurrentRegistry(); // 需要新增这个接口

    for (auto it = s_PendingTasks.begin(); it != s_PendingTasks.end(); ) {
        auto& task = *it;
        if (!task->ready.load()) {
            ++it;
            continue;
        }

        // CPU 数据已就绪 + GL 资源已 Drain → 组装 Entity
        if (task->isHierarchy) {
            BuildHierarchyEntity(registry, task->cpuSceneData, task->filepath);
        }
        else {
            BuildModel3DEntity(registry, task->cpuModelData, task->filepath);
        }

        it = s_PendingTasks.erase(it);
    }
}

void EntityFactory::ShutdownAsync()
{
    std::lock_guard lock(s_TaskMutex);
    s_PendingTasks.clear();
}
