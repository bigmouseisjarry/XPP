#include "UniformBuffer.h"

const std::unordered_map<std::string, unsigned int> UniformBuffer::s_BlockBindings = {
    { "PerFrameData",   0 },
    { "PerObjectData",  1 },
    { "LightArrayData", 2 },
    { "SSAOKernelData", 3 },
};

UniformBuffer::UniformBuffer(unsigned int size, unsigned int bindingPoint)
	:m_BindingPoint(bindingPoint)
{
    glGenBuffers(1, &m_RendererID);
    glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
    glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, m_BindingPoint, m_RendererID);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

UniformBuffer::UniformBuffer(const void* data, size_t size, unsigned int binding)
{
    glGenBuffers(1, &m_RendererID);
    glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, m_BindingPoint, m_RendererID);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

UniformBuffer::~UniformBuffer()
{
    glDeleteBuffers(1, &m_RendererID);
}

void UniformBuffer::Bind() const
{
	glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
}

void UniformBuffer::Unbind() const
{
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer::SetData(const void* data, unsigned int size) const
{
    glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer::SetSubData(const void* data, unsigned int offset, unsigned int size) const
{
    glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

unsigned int UniformBuffer::GetBindingPoint(const std::string& blockName)
{
    auto it = s_BlockBindings.find(blockName);
    if (it != s_BlockBindings.end()) return it->second;
    return 0;
}
