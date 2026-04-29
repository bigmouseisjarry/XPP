#include "Framebuffer.h"
#include <iostream>
#include "TextureSemantic.h"

Framebuffer::Framebuffer(const FramebufferSpec& spec)
    : m_Spec(spec)
{
	Invalidate();
}

Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &m_RendererID);
    
    for (auto texID : m_ColorGLTextureIDs)
        glDeleteTextures(1, &texID);

    if (m_DepthGLTextureID)
        glDeleteTextures(1, &m_DepthGLTextureID);
    if (m_DepthStencilRBO)
        glDeleteRenderbuffers(1, &m_DepthStencilRBO);

}

void Framebuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    glViewport(0, 0, m_Spec.width, m_Spec.height);
}

void Framebuffer::Unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int Framebuffer::GetColorGLTextureID(int index) const
{
	if (index >= 0 && index < (int)m_ColorGLTextureIDs.size())
        return m_ColorGLTextureIDs[index];

	return 0;
}

void Framebuffer::BindColorTexture(int index, unsigned int slot) const
{
    if (index < 0 || index >= (int)m_ColorGLTextureIDs.size()) return;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_ColorGLTextureIDs[index]); // 确保 index 有效
}

unsigned int Framebuffer::GetDepthGLTextureID() const
{
	return m_DepthGLTextureID;
}


// TUDO:应该给个辅助函数判断得到底层 GL 格式，深度纹理可能是 GL_TEXTURE_2D 或 GL_TEXTURE_2D_ARRAY
void Framebuffer::BindDepthTexture(unsigned int slot) const
{
    if (!m_DepthGLTextureID) return;
    glActiveTexture(GL_TEXTURE0 + slot);
    GLenum target = (m_Spec.depthAttachment.format == AttachmentFormat::DepthArray24)
        ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
    glBindTexture(target, m_DepthGLTextureID);
}

void Framebuffer::UnbindDepthTexture(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    GLenum target = (m_Spec.depthAttachment.format == AttachmentFormat::DepthArray24)
        ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
    glBindTexture(target, 0);
}

void Framebuffer::BindLayer(int layer) const
{
    if (!m_DepthGLTextureID) return;
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        m_DepthGLTextureID, 0, layer);
}

void Framebuffer::UnbindTexture(unsigned int slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Framebuffer::Resize(int width, int height)
{
    m_Spec.width = width;
    m_Spec.height = height;

    // 销毁旧 GL 对象
    for (auto id : m_ColorGLTextureIDs)
        glDeleteTextures(1, &id);
    m_ColorGLTextureIDs.clear();

    if (m_DepthGLTextureID)
    {
        glDeleteTextures(1, &m_DepthGLTextureID);
        m_DepthGLTextureID = 0;
    }
    if (m_DepthStencilRBO)
    {
        glDeleteRenderbuffers(1, &m_DepthStencilRBO);
        m_DepthStencilRBO = 0;
    }
    glDeleteFramebuffers(1, &m_RendererID);

    // 重新创建（复用构造函数中的创建逻辑）
    Invalidate();
}

void Framebuffer::Invalidate()
{
    glGenFramebuffers(1, &m_RendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

    // --- 颜色 Attachments ---
    m_ColorGLTextureIDs.reserve(m_Spec.colorAttachments.size());
    std::vector<GLenum> drawBuffers;


    // 多渲染目标（MRT）
    for (int i = 0; i < (int)m_Spec.colorAttachments.size(); i++)
    {
        const auto& att = m_Spec.colorAttachments[i];
        if (att.format == AttachmentFormat::None) continue;

        unsigned int texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        glTexImage2D(GL_TEXTURE_2D, 0, ToInternalFormat(att.format),
            m_Spec.width, m_Spec.height, 0,
            ToFormat(att.format), ToType(att.format), nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
            GL_TEXTURE_2D, texID, 0);

        m_ColorGLTextureIDs.push_back(texID);
        drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
    }

    // --- 深度 Attachment ---
    if (m_Spec.hasDepth())
    {
        AttachmentFormat depthFmt = m_Spec.depthAttachment.format;

        if (depthFmt == AttachmentFormat::Depth24)
        {
            // 深度纹理（可采样，阴影贴图用）
            glGenTextures(1, &m_DepthGLTextureID);
            glBindTexture(GL_TEXTURE_2D, m_DepthGLTextureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
                m_Spec.width, m_Spec.height, 0,
                GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            if (m_Spec.depthClampToBorder)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                GL_TEXTURE_2D, m_DepthGLTextureID, 0);
        }
        else if (depthFmt == AttachmentFormat::Depth24Stencil8)
        {
            // 深度+模板 RBO（不可采样，省显存）
            glGenRenderbuffers(1, &m_DepthStencilRBO);
            glBindRenderbuffer(GL_RENDERBUFFER, m_DepthStencilRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                m_Spec.width, m_Spec.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                GL_RENDERBUFFER, m_DepthStencilRBO);
        }
        else if (depthFmt == AttachmentFormat::DepthArray24)
        {
            int layers = std::max(m_Spec.depthLayers, 1);

            glGenTextures(1, &m_DepthGLTextureID);
            glBindTexture(GL_TEXTURE_2D_ARRAY, m_DepthGLTextureID);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24,
                m_Spec.width, m_Spec.height, layers, 0,
                GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

            if (m_Spec.depthShadowCompare)
            {
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            }

            if (m_Spec.depthClampToBorder)
            {
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            // 默认绑定 layer 0
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                m_DepthGLTextureID, 0, 0);
        }
    }

    // --- Draw Buffers ---
    if (m_Spec.hasColor())
    {
        glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data());
    }
    else
    {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    // --- 完整性检查 ---
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLenum Framebuffer::ToInternalFormat(AttachmentFormat fmt)
{
    switch (fmt)
    {
    case AttachmentFormat::RGBA8:          return GL_RGBA8;
    case AttachmentFormat::RGBA16F:        return GL_RGBA16F;
    case AttachmentFormat::Depth24:        return GL_DEPTH_COMPONENT24;
    case AttachmentFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
    case AttachmentFormat::DepthArray24:     return GL_DEPTH_COMPONENT24;
    default: return 0;
    }
}

GLenum Framebuffer::ToFormat(AttachmentFormat fmt)
{
    switch (fmt)
    {
    case AttachmentFormat::RGBA8:
    case AttachmentFormat::RGBA16F:
        return GL_RGBA;
    case AttachmentFormat::Depth24:        
    case AttachmentFormat::DepthArray24:     
        return GL_DEPTH_COMPONENT;
    case AttachmentFormat::Depth24Stencil8: return GL_DEPTH_STENCIL;
    default: return 0;
    }
}

GLenum Framebuffer::ToType(AttachmentFormat fmt)
{
    switch (fmt)
    {
    case AttachmentFormat::RGBA8:               return GL_UNSIGNED_BYTE;
    case AttachmentFormat::RGBA16F:             return GL_FLOAT;
    case AttachmentFormat::Depth24:             return GL_FLOAT;
    case AttachmentFormat::Depth24Stencil8:     return GL_UNSIGNED_INT_24_8;
    case AttachmentFormat::DepthArray24:        return GL_FLOAT;
    default: return 0;
    }
}

bool Framebuffer::IsDepthFormat(AttachmentFormat fmt)
{
    return fmt == AttachmentFormat::Depth24 || fmt == AttachmentFormat::Depth24Stencil8 || fmt == AttachmentFormat::DepthArray24;
}
