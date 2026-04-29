#pragma once
#include <GL/glew.h>
#include <vector>

enum class AttachmentFormat {
    None = 0,
    RGBA8,          // GL_RGBA8
    RGBA16F,        // GL_RGBA16F
    Depth24,        // GL_DEPTH_COMPONENT24
    Depth24Stencil8, // GL_DEPTH24_STENCIL8
    DepthArray24    // GL_DEPTH_COMPONENT24 纹理数组
};

struct FramebufferAttachment {
    AttachmentFormat format = AttachmentFormat::None;
};

struct FramebufferSpec {
    int width = 0;
    int height = 0;
    std::vector<FramebufferAttachment> colorAttachments;
    FramebufferAttachment depthAttachment;
    bool depthClampToBorder = false;  // 深度纹理使用 CLAMP_TO_BORDER + 白色 border（阴影贴图需要）
    int depthLayers = 1;              // 纹理数组层数，1 = 普通纹理
    bool depthShadowCompare = false;  // 启用 GL_LINEAR + 深度比较模式

    bool hasColor() const { return !colorAttachments.empty(); }
    bool hasDepth() const { return depthAttachment.format != AttachmentFormat::None; }
};

class Framebuffer
{
private:
    unsigned int m_RendererID = 0;
    FramebufferSpec m_Spec;
    std::vector<unsigned int> m_ColorGLTextureIDs;
    unsigned int m_DepthGLTextureID = 0;     // 纹理形式的 depth（Depth24），可采样
    unsigned int m_DepthStencilRBO = 0;    // RBO 形式的 depth/stencil（Depth24Stencil8），不可采样

public:
    Framebuffer(const FramebufferSpec& spec);
    ~Framebuffer();

    void Bind() const;
    void Unbind() const;

    // Color attachment 访问
    unsigned int GetColorGLTextureID(int index = 0) const;
    void BindColorTexture(int index, unsigned int slot) const;

    // Depth attachment 访问
    unsigned int GetDepthGLTextureID() const;
    void BindDepthTexture(unsigned int slot) const;
    void UnbindDepthTexture(unsigned int slot) const;

    // 纹理数组：绑定到特定 layer
    void BindLayer(int layer) const;

    // 通用纹理解绑
    static void UnbindTexture(unsigned int slot);

    int GetWidth() const { return m_Spec.width; }
    int GetHeight() const { return m_Spec.height; }
    const FramebufferSpec& GetSpec() const { return m_Spec; }

    void Resize(int width, int height);

private:
    void Invalidate();
    // AttachmentFormat → GL 枚举映射
    static GLenum ToInternalFormat(AttachmentFormat fmt);   // GPU 内部存储格式
    static GLenum ToFormat(AttachmentFormat fmt);           // 纹理的通道格式
    static GLenum ToType(AttachmentFormat fmt);             // 每个通道的数据类型
    static bool IsDepthFormat(AttachmentFormat fmt);        // 判断是不是深度格式
};