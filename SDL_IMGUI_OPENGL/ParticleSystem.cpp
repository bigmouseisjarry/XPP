#include "ParticleSystem.h"
#include "Renderer.h"

ParticleSystem* ParticleSystem::Get()
{
	static ParticleSystem instance;
	return &instance;
}

void ParticleSystem::Update(entt::registry& registry, float deltaTime)
{

    InitResources();

    auto view = registry.view<ParticleEmitterComponent>();
    for (auto [entity, emitter] : view.each())
    {
        if (!emitter.isPlaying) continue;
        UpdateEmitter(emitter, deltaTime);
    }
}

void ParticleSystem::SubmitRender(entt::registry& registry)
{
    auto view = registry.view<ParticleEmitterComponent>();
    for (auto [entity, emitter] : view.each())
    {
        int count = (int)emitter.m_InstanceData.size();
        if (count == 0) continue;

        Renderer::Get()->SubmitInstancedUnits(
            emitter.m_MeshID, emitter.m_Material.get(),
            emitter.renderLayer, count, emitter.additiveBlend);
    }
}

void ParticleSystem::InitResources()
{
    if (m_Initialized) return;

    // 加载粒子着色器
    m_ParticleShaderID = ResourceManager::Get()->GetShaderID("ShaderParticle.glsl");

    // 创建粒子 quad mesh（和普通 quad 一样，但用 VertexParticle 类型）
    std::vector<Vertex> quadVerts = {
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, 0.0f}, {0.0f, 1.0f}}
    };
    std::vector<unsigned int> quadIndices = { 0, 1, 2, 2, 3, 0 };
    m_QuadMeshID = ResourceManager::Get()->CreateMesh<Vertex>(
        "particleQuad", quadVerts, quadIndices);

    m_Initialized = true;
}


// TODO:这里的随机数可以将其抽出来做一个随机系统
void ParticleSystem::Emit(ParticleEmitterComponent& emitter, int count)
{
    for (int i = 0; i < count; i++)
    {
        if ((int)emitter.m_Particles.size() >= emitter.maxParticles)
            break;

        ParticleEmitterComponent::Particle p;
        p.alive = true;
        p.age = 0.0f;
        p.lifetime = emitter.lifetimeMin + (emitter.lifetimeMax - emitter.lifetimeMin) * ((float)rand() / RAND_MAX);
        p.position = SamplePosition(emitter.shape, emitter.shapeSize, emitter.shapeInnerRadius);
        p.size = 1.0f;
        p.rotation = 0.0f;

        float speed = emitter.speedMin + (emitter.speedMax - emitter.speedMin) * ((float)rand() / RAND_MAX);
        p.velocity = SampleDirection(emitter.direction, emitter.directionAngle) * speed;

        if (emitter.randomSpriteFrame && emitter.spriteCols > 1 && emitter.spriteRows > 1)
            p.spriteFrame = rand() % (emitter.spriteCols * emitter.spriteRows);
        else
            p.spriteFrame = 0;

        emitter.m_Particles.push_back(p);
    }
}

void ParticleSystem::UpdateEmitter(ParticleEmitterComponent& emitter, float deltaTime)
{

    // 1. 发射新粒子
    emitter.m_EmitAccumulator += emitter.emitRate * deltaTime;
    while (emitter.m_EmitAccumulator >= 1.0f)
    {
        Emit(emitter, 1);
        emitter.m_EmitAccumulator -= 1.0f;
    }

    // 2. 更新粒子状态
    for (auto& p : emitter.m_Particles)
    {
        if (!p.alive) continue;

        p.age += deltaTime;
        if (p.age >= p.lifetime)
        {
            p.alive = false;
            continue;
        }

        p.velocity += emitter.gravity * deltaTime;
        p.velocity *= (1.0f - emitter.drag * deltaTime);
        p.position += p.velocity * deltaTime;
        p.rotation += emitter.rotationSpeed * deltaTime;
    }

    // 3. swap-and-pop 移除死亡粒子
    for (int i = (int)emitter.m_Particles.size() - 1; i >= 0; i--)
    {
        if (!emitter.m_Particles[i].alive)
        {
            // 和最后一个活着的粒子交换
            int last = (int)emitter.m_Particles.size() - 1;
            if (i != last)
                std::swap(emitter.m_Particles[i], emitter.m_Particles[last]);
            emitter.m_Particles.pop_back();
        }
    }

    // 4. 构建实例数据
    emitter.m_InstanceData.clear();
    float uPerFrame = 1.0f / emitter.spriteCols;
    float vPerFrame = 1.0f / emitter.spriteRows;
    for (auto& p : emitter.m_Particles)
    {
        if (!p.alive) continue;

        float t = p.age / p.lifetime;  // 归一化时间

        // 精灵表 UV
        int col = p.spriteFrame % emitter.spriteCols;
        int row = p.spriteFrame / emitter.spriteCols;

        VertexParticleInstance inst;
        inst.position = p.position;
        inst.color = SampleColor(emitter.colorOverLifetime, t);
        inst.size = SampleSize(emitter.sizeOverLifetime, t) * p.size;
        inst.rotation = p.rotation;
        inst.uvOffset = { col * uPerFrame, row * vPerFrame };
        inst.uvScale = { uPerFrame, vPerFrame };
        emitter.m_InstanceData.push_back(inst);
    }

    // 5. 上传 GPU
    int aliveCount = (int)emitter.m_InstanceData.size();
    if (aliveCount > 0)
    {
        // 懒初始化：首次有粒子时创建 instanced mesh
        if (emitter.m_MeshID.value == INVALID_ID)
        {
            auto instanceVBO = std::make_unique<VertexBuffer>(
                nullptr,
                emitter.maxParticles * sizeof(VertexParticleInstance),
                false  // GL_DYNAMIC_DRAW
            );

            VertexBufferLayout instanceLayout;
            instanceLayout.Push<float>(3);  // position
            instanceLayout.Push<float>(4);  // color
            instanceLayout.Push<float>(1);  // size
            instanceLayout.Push<float>(1);  // rotation
            instanceLayout.Push<float>(2);  // uvOffset
            instanceLayout.Push<float>(2);  // uvScale

            static int meshCounter = 0;
            const Mesh* sharedMesh = ResourceManager::Get()->GetMesh(m_QuadMeshID);
            emitter.m_MeshID = ResourceManager::Get()->CreateInstancedMesh( "particleMesh_" + std::to_string(meshCounter++), 
                *sharedMesh,std::move(instanceVBO), instanceLayout, emitter.maxParticles);

            if (emitter.particleTexture.value != INVALID_ID)
                emitter.m_Material->SetTexture(TextureSemantic::Albedo, emitter.particleTexture);
        }

        Mesh* mesh = ResourceManager::Get()->GetMeshMut(emitter.m_MeshID);
        if (mesh)
            mesh->UpdateInstanceData(emitter.m_InstanceData.data(), aliveCount, sizeof(VertexParticleInstance));
    }
}

glm::vec4 ParticleSystem::SampleColor(const std::vector<ParticleColorKey>& keys, float t)
{
    if (keys.empty()) return { 1,1,1,1 };
    if (keys.size() == 1 || t <= keys[0].time) return keys[0].color;
    if (t >= keys.back().time) return keys.back().color;

    for (size_t i = 0; i < keys.size() - 1; i++)
    {
        if (t >= keys[i].time && t <= keys[i + 1].time)
        {
            float f = (t - keys[i].time) / (keys[i + 1].time - keys[i].time);
            return glm::mix(keys[i].color, keys[i + 1].color, f);
        }
    }
    return keys.back().color;
}

float ParticleSystem::SampleSize(const std::vector<ParticleSizeKey>& keys, float t)
{
    if (keys.empty()) return 1.0f;
    if (keys.size() == 1 || t <= keys[0].time) return keys[0].size;
    if (t >= keys.back().time) return keys.back().size;

    for (size_t i = 0; i < keys.size() - 1; i++)
    {
        if (t >= keys[i].time && t <= keys[i + 1].time)
        {
            float f = (t - keys[i].time) / (keys[i + 1].time - keys[i].time);
            return glm::mix(keys[i].size, keys[i + 1].size, f);
        }
    }
    return keys.back().size;
}

glm::vec3 ParticleSystem::SamplePosition(EmitterShape shape, const glm::vec3& shapeSize, float innerRadius)
{
    float rx = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    float ry = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    float rz = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

    switch (shape)
    {
    case EmitterShape::Point:
        return { 0, 0, 0 };
    case EmitterShape::Box:
        return { rx * shapeSize.x, ry * shapeSize.y, rz * shapeSize.z };
    case EmitterShape::Sphere: {
        glm::vec3 dir = glm::normalize(glm::vec3(rx, ry, rz));
        float r = innerRadius + (1.0f - innerRadius) * ((float)rand() / RAND_MAX);
        return dir * shapeSize.x * r;
    }
    case EmitterShape::Cone: {
        // 沿 direction 方向的锥体，简化为在 XZ 平面扇形
        float angle = ((float)rand() / RAND_MAX) * shapeSize.x;  // shapeSize.x = 半角(弧度)
        float dist = innerRadius + (1.0f - innerRadius) * ((float)rand() / RAND_MAX);
        float a = ((float)rand() / RAND_MAX) * 6.283185f;
        return glm::vec3(cos(a) * sin(angle), sin(a) * sin(angle), cos(angle)) * dist;
    }
    }
    return { 0, 0, 0 };
}

glm::vec3 ParticleSystem::SampleDirection(const glm::vec3& dir, float angle)
{
    if (angle <= 0.0f) return glm::normalize(dir);

    // 在 dir 周围锥形范围内随机
    glm::vec3 d = glm::normalize(dir);
    float rad = angle * 3.14159265f / 180.0f;
    float theta = ((float)rand() / RAND_MAX) * rad;
    float phi = ((float)rand() / RAND_MAX) * 6.283185f;

    // 构建正交基
    glm::vec3 up = abs(d.z) < 0.99f ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
    glm::vec3 right = glm::normalize(glm::cross(up, d));
    up = glm::cross(d, right);

    return glm::normalize(d + right * sin(theta) * cos(phi) + up * sin(theta) * sin(phi));
}
