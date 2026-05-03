#pragma once
#include <glm.hpp>
#include <vector>
#include "Material.h"
#include "UniformBuffer.h"
#include "ComLight.h"

enum RenderLayer
{
    RQ_WorldBackground,
    RQ_WorldObjects,
    RQ_MiniMap,
    RQ_UI
};

enum class RenderMode
{
    Render2D,
    Render3D
};

enum class UBOBinding : unsigned int
{
    PerFrame = 0,
    PerObject = 1,
    Lights = 2,
    SSAOKernel = 3
};

struct RenderUnit
{
    MeshID mesh;
    const Material* material;
    glm::mat4 model;

    uint64_t SortKey() const
    {
        // [isTransparent:1][doubleSided:1][shader:16][albedo:16][mesh:30]
        uint64_t key = 0;
        key |= uint64_t(material->m_Transparent ? 1 : 0) << 63;
        key |= uint64_t(material->m_DoubleSided ? 1 : 0) << 62;
        key |= uint64_t(material->m_Shader.value & 0xFFFF) << 46;
        key |= uint64_t(material->GetTexture(TextureSemantic::Albedo).value & 0xFFFF) << 30;
        key |= uint64_t(mesh.value & 0x3FFFFFFF);
        return key;
    }
};

struct CameraUnit
{
    glm::mat4 projView;
    std::vector<RenderLayer> layers;
    glm::vec3 viewPos;
};

struct InstancedRenderUnit
{
    MeshID mesh;
    const Material* material;
    RenderLayer renderLayer;
    unsigned int instanceCount;
    bool additiveBlend;
};

struct alignas(16) PerFrameData
{
    glm::mat4 u_ProjView;
    glm::vec3 u_ViewPos;
    float     _pad0;
};

struct alignas(16) PerObjectData
{
    glm::mat4 u_Model;
    glm::mat4 u_MVP;
    glm::vec4 u_Color;
    float u_MetallicFactor;
    float u_RoughnessFactor;
    float u_AlphaCutoff;    
    float _pad2;
    glm::vec4 u_EmissiveFactor;   // xyz 生效, w=0
};

struct alignas(16) LightData
{
    glm::vec3 position;
    float     _pad0;
    glm::vec3 color;
    float     intensity;
    glm::mat4 lightSpaceMatrix;
    glm::ivec4 flags;   // x = castShadow, y = shadowLayer, zw = pad
};

struct alignas(16) LightArrayData
{
    glm::ivec4 _numLights;   // x = count, yzw = pad
    LightData  u_Lights[8];
};

struct BlendState {
    bool enabled = false;
    GLenum srcFactor = GL_SRC_ALPHA;
    GLenum dstFactor = GL_ONE_MINUS_SRC_ALPHA;
};

struct DepthState {
    bool enabled = true;
    GLenum func = GL_LESS;
    bool writeMask = true;
};

struct CullState {
    bool enabled = true;
    GLenum face = GL_BACK;
    GLenum winding = GL_CCW;
};

struct ViewportState {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct PipelineState {
    BlendState blend;
    DepthState depth;
    CullState  cull;
    bool       colorMask[4] = { true, true, true, true };
    ViewportState viewport;   
};

