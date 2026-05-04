#pragma once
#include <GL/glew.h>
#include <vector>
#include "ResourceIDs.h"

struct FramebufferSpec {
    int width = 0;
    int height = 0;
    std::vector<TextureID> colorAttachments;
    TextureID depthAttachment = { INVALID_ID };
    bool useDepthStencilRBO = false;
};

class Framebuffer
{
private:
    unsigned int m_RendererID = 0;
    unsigned int m_DepthStencilRBO = 0;    // RBO 形式的 depth/stencil（Depth24Stencil8），不可采样

    FramebufferSpec m_Spec;
    std::vector<TextureID> m_ColorTextureIDs;  // V2: TextureID 引用
    TextureID m_DepthTextureID = { INVALID_ID };  // V2: TextureID 引用

public:
    Framebuffer(const FramebufferSpec& spec);
    ~Framebuffer();

    void Bind() const;
    void Unbind() const;

    // 纹理数组：绑定到特定 layer
    void BindLayer(int layer) const;

    // 通用纹理解绑
    static void UnbindTexture(unsigned int slot);

    int GetWidth() const { return m_Spec.width; }
    int GetHeight() const { return m_Spec.height; }
    const FramebufferSpec& GetSpec() const { return m_Spec; }

    void Resize(int width, int height);

    void AttachColor(TextureID texid, int index);
    void AttachDepth(TextureID texid);
    void AttachCubemapFace(TextureID texid, int face, int mipLevel);  // 改签名，TextureID 替代 GLuint

    TextureID GetColorTextureID(int index = 0) const;
    TextureID GetDepthTextureID() const;

private:
    void Invalidate();
};