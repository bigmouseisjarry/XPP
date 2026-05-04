#include "Framebuffer.h"
#include <iostream>
#include "TextureSemantic.h"
#include "ResourceManager.h"

Framebuffer::Framebuffer(const FramebufferSpec& spec)
    : m_Spec(spec)
{
	Invalidate();
}

Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &m_RendererID);   

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

void Framebuffer::BindLayer(int layer) const
{
    const Texture* tex = ResourceManager::Get()->GetTexture(m_DepthTextureID);
    if (!tex) return;
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        tex->GetRendererID(), 0, layer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

    // Resize 引用的纹理（GL ID 会在 Texture 内部更新）
    auto* rm = ResourceManager::Get();
    for (TextureID tid : m_ColorTextureIDs) {
        Texture* tex = rm->GetTextureMut(tid);
        if (tex) tex->Resize(width, height);
    }
    if (m_DepthTextureID.value != INVALID_ID) {
        Texture* tex = rm->GetTextureMut(m_DepthTextureID);
        if (tex) tex->Resize(width, height);
    }

    // RBO 也需要重建
    if (m_DepthStencilRBO) {
        glDeleteRenderbuffers(1, &m_DepthStencilRBO);
        m_DepthStencilRBO = 0;
    }

    // 重建 FBO 容器（不拥有纹理，重建代价很低）
    m_ColorTextureIDs.clear(); 
    m_DepthTextureID = { INVALID_ID };
    glDeleteFramebuffers(1, &m_RendererID);
    Invalidate();

}

void Framebuffer::AttachColor(TextureID texid, int index)
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    const Texture* tex = ResourceManager::Get()->GetTexture(texid);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0 + index, tex->GetTarget(), tex->GetRendererID(), 0);
    if (index >= (int)m_ColorTextureIDs.size())
        m_ColorTextureIDs.resize(index + 1, { INVALID_ID });
    m_ColorTextureIDs[index] = texid;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::AttachDepth(TextureID texid)
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    const Texture* tex = ResourceManager::Get()->GetTexture(texid);
    if (tex->GetTarget() == GL_TEXTURE_2D)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex->GetRendererID(), 0);
    else if (tex->GetTarget() == GL_TEXTURE_2D_ARRAY)
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex->GetRendererID(), 0, 0);

    m_DepthTextureID = texid;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::AttachCubemapFace(TextureID texid, int face, int mipLevel)
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    const Texture* tex = ResourceManager::Get()->GetTexture(texid);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, tex->GetRendererID(), mipLevel);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);  

    int mipSize = m_Spec.width >> mipLevel;
    glViewport(0, 0, mipSize, mipSize);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

TextureID Framebuffer::GetColorTextureID(int index) const
{
    if (index >= (int)m_ColorTextureIDs.size())
        return { INVALID_ID };
    else
        return m_ColorTextureIDs[index];
}

TextureID Framebuffer::GetDepthTextureID() const
{
    return m_DepthTextureID;
}



void Framebuffer::Invalidate()
{
    glGenFramebuffers(1, &m_RendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

    // --- 颜色 Attachments ---
    m_ColorTextureIDs.reserve(m_Spec.colorAttachments.size());
    std::vector<GLenum> drawBuffers;

    for (int i = 0; i < (int)m_Spec.colorAttachments.size(); i++)
    {
        TextureID texid = m_Spec.colorAttachments[i];
        const Texture* tex = ResourceManager::Get()->GetTexture(texid);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, tex->GetTarget(), tex->GetRendererID(), 0);
        m_ColorTextureIDs.push_back(texid);
        drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
    }

    if (m_Spec.depthAttachment.value != INVALID_ID)
    {

        const Texture* tex = ResourceManager::Get()->GetTexture(m_Spec.depthAttachment);
        if (tex->GetTarget() == GL_TEXTURE_2D)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex->GetRendererID(), 0);
        else if (tex->GetTarget() == GL_TEXTURE_2D_ARRAY)
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex->GetRendererID(), 0, 0);

        m_DepthTextureID = m_Spec.depthAttachment;
    }

    if (m_Spec.useDepthStencilRBO)
    {
        glGenRenderbuffers(1, &m_DepthStencilRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_DepthStencilRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
            m_Spec.width, m_Spec.height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER, m_DepthStencilRBO);
    }

    // 在 depth/RBO 处理之后、完整性检查之前
    if (!m_Spec.colorAttachments.empty())
        glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data());
    else if (m_Spec.depthAttachment.value != INVALID_ID || m_Spec.useDepthStencilRBO)
    {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }
    else
    {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    // --- 完整性检查 ---
    if (!m_Spec.colorAttachments.empty() || m_Spec.depthAttachment.value != INVALID_ID || m_Spec.useDepthStencilRBO)
    {
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer is not complete!" << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
