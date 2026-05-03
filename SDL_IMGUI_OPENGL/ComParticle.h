#pragma once
#include <glm.hpp>
#include <vector>
#include "Vertex.h"
#include "ResourceManager.h"
#include "Material.h"
#include "RenderTypes.h"

// 粒子发射器形状
enum class EmitterShape
{
    Point,      // 点
    Box,        // 盒子（范围随机）
    Sphere,     // 球面
    Cone        // 锥体
};

struct ParticleColorKey
{
    float     time;      // 归一化时间 0~1
    glm::vec4 color;
};

struct ParticleSizeKey
{
    float time;         // 归一化时间 0~1
    float size;
};

struct ParticleEmitterComponent
{
    // ── 发射参数 ──
    float     emitRate = 10.0f;                                 // 每秒发射数
    int       maxParticles = 500;                               // 最大粒子数
    int       burstCount = 0;                                   // 单次爆发数（0=持续发射）

    EmitterShape shape = EmitterShape::Point;
    glm::vec3 shapeSize = { 1,1,1 };                            // Box 半尺寸 / Sphere 半径 / Cone 角度
    float     shapeInnerRadius = 0.0f;                          // 球/锥内径

    // ── 生命周期 ──
    float     lifetimeMin = 1.0f;
    float     lifetimeMax = 2.0f;

    // ── 速度 ──
    glm::vec3 direction = { 0,0,1 };                            // 发射方向
    float     directionAngle = 0.0f;                            // 方向散布角（度）
    float     speedMin = 1.0f;
    float     speedMax = 3.0f;

    // ── 物理参数 ──
    glm::vec3 gravity = { 0,0,0 };                              // 重力加速度
    float     drag = 0.0f;                                      // 阻力系数

    // ── 视觉参数 ──
    std::vector<ParticleColorKey> colorOverLifetime = {
        {0.0f, {1,1,1,1}}, {1.0f, {1,1,1,0}}
    };
    std::vector<ParticleSizeKey> sizeOverLifetime = {
        {0.0f, 1.0f}, {1.0f, 0.0f}
    };
    float     rotationSpeed = 0.0f;                             // 旋转速度（弧度/秒）

    // ── 子发射器 ──
    bool      hasSubEmitter = false;                            // 是否有子发射器
    int       subEmitterEvent = 0;                              // 0=死亡时, 1=出生时

    // ── 纹理 ──
    TextureID particleTexture = { INVALID_ID };
    int       spriteCols = 1;                                   // 精灵表列数
    int       spriteRows = 1;                                   // 精灵表行数
    bool      randomSpriteFrame = false;                        // 随机选择序列帧纹理图中的某一帧作为起始帧

    // ── 渲染 ──
    bool      additiveBlend = false;                            // true=加法混合(火焰/光效)
    RenderLayer renderLayer = RenderLayer::RQ_WorldObjects;
    bool      is3D = false;                                     // 由场景自动设置

    // ── 内部状态 ──
    bool      isPlaying = true;
    float     m_EmitAccumulator = 0.0f;                         // 距离上一次发射的时间

    ParticleEmitterComponent();
    ~ParticleEmitterComponent();

    ParticleEmitterComponent(ParticleEmitterComponent&&) noexcept;
    ParticleEmitterComponent& operator=(ParticleEmitterComponent&&) noexcept;
    ParticleEmitterComponent(const ParticleEmitterComponent&) = delete;
    ParticleEmitterComponent& operator=(const ParticleEmitterComponent&) = delete;

    // ── 内部数据（ParticleSystem 访问）──
    struct Particle
    {
        glm::vec3 position;
        glm::vec3 velocity;
        float     lifetime;
        float     age = 0.0f;
        float     size = 1.0f;
        float     rotation = 0.0f;
        int       spriteFrame = 0;
        bool      alive = false;
    };

    std::vector<Particle> m_Particles;
    std::vector<VertexParticleInstance> m_InstanceData;
    std::unique_ptr<Material> m_Material;
    MeshID m_MeshID = { INVALID_ID };
};