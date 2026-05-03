#include "ComParticle.h"

ParticleEmitterComponent::ParticleEmitterComponent()
{
    m_Particles.reserve(maxParticles);
    m_InstanceData.reserve(maxParticles);

    m_Material = std::make_unique<Material>();
    m_Material->m_Shader = ResourceManager::Get()->GetShaderID("ShaderParticle.glsl");
    m_Material->m_Transparent = true;
    m_Material->m_DoubleSided = true;
    m_Material->Set("u_Color", glm::vec4(1.0f));
    m_Material->Set("u_Dimension", (int)0);
}

ParticleEmitterComponent::~ParticleEmitterComponent() = default;

ParticleEmitterComponent::ParticleEmitterComponent(ParticleEmitterComponent&& o) noexcept
    : emitRate(o.emitRate), maxParticles(o.maxParticles), burstCount(o.burstCount)
    , shape(o.shape), shapeSize(o.shapeSize), shapeInnerRadius(o.shapeInnerRadius)
    , lifetimeMin(o.lifetimeMin), lifetimeMax(o.lifetimeMax)
    , direction(o.direction), directionAngle(o.directionAngle)
    , speedMin(o.speedMin), speedMax(o.speedMax)
    , gravity(o.gravity), drag(o.drag)
    , colorOverLifetime(std::move(o.colorOverLifetime))
    , sizeOverLifetime(std::move(o.sizeOverLifetime))
    , rotationSpeed(o.rotationSpeed)
    , hasSubEmitter(o.hasSubEmitter), subEmitterEvent(o.subEmitterEvent)
    , particleTexture(o.particleTexture), spriteCols(o.spriteCols), spriteRows(o.spriteRows)
    , randomSpriteFrame(o.randomSpriteFrame)
    , additiveBlend(o.additiveBlend), renderLayer(o.renderLayer), is3D(o.is3D)
    , isPlaying(o.isPlaying), m_EmitAccumulator(o.m_EmitAccumulator)
    , m_Particles(std::move(o.m_Particles))
    , m_InstanceData(std::move(o.m_InstanceData))
    , m_Material(std::move(o.m_Material)), m_MeshID(o.m_MeshID)
{
}

ParticleEmitterComponent& ParticleEmitterComponent::operator=(ParticleEmitterComponent&& o) noexcept
{
    if (this != &o) {
        emitRate = o.emitRate;
        maxParticles = o.maxParticles;
        burstCount = o.burstCount;
        shape = o.shape;
        shapeSize = o.shapeSize;
        shapeInnerRadius = o.shapeInnerRadius;
        lifetimeMin = o.lifetimeMin;
        lifetimeMax = o.lifetimeMax;
        direction = o.direction;
        directionAngle = o.directionAngle;
        speedMin = o.speedMin;
        speedMax = o.speedMax;
        gravity = o.gravity;
        drag = o.drag;
        rotationSpeed = o.rotationSpeed;
        hasSubEmitter = o.hasSubEmitter;
        subEmitterEvent = o.subEmitterEvent;
        particleTexture = o.particleTexture; 
        spriteCols = o.spriteCols;
        spriteRows = o.spriteRows;
        randomSpriteFrame = o.randomSpriteFrame;
        additiveBlend = o.additiveBlend;
        renderLayer = o.renderLayer;
        is3D = o.is3D;
        isPlaying = o.isPlaying;
        m_EmitAccumulator = o.m_EmitAccumulator;
        m_MeshID = o.m_MeshID;

        colorOverLifetime = std::move(o.colorOverLifetime);
        sizeOverLifetime = std::move(o.sizeOverLifetime);
        m_Particles = std::move(o.m_Particles);
        m_InstanceData = std::move(o.m_InstanceData);
        m_Material = std::move(o.m_Material);
    }
    return *this;
}