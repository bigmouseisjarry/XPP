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
    glGenBuffers(BUFFER_COUNT, m_Buffers);
    for (int i = 0;i < BUFFER_COUNT;i++)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, m_Buffers[i]);
        glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_Buffers[0]);
    m_CurrentBuffer = m_Buffers[0];
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

UniformBuffer::UniformBuffer(const void* data, size_t size, unsigned int bindingPoint)
    :m_BindingPoint(bindingPoint)
{
    glGenBuffers(BUFFER_COUNT, m_Buffers);
    glBindBuffer(GL_UNIFORM_BUFFER, m_Buffers[0]);
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
    for (int i = 1; i < BUFFER_COUNT; i++)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, m_Buffers[i]);
        glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_Buffers[0]);
    m_CurrentBuffer = m_Buffers[0];
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

UniformBuffer::~UniformBuffer()
{
    glDeleteBuffers(BUFFER_COUNT, m_Buffers);
}

void UniformBuffer::SetData(const void* data, unsigned int size) const
{
    m_CurrentBuffer = (m_CurrentBuffer + 1) % BUFFER_COUNT;
    unsigned int buf = m_Buffers[m_CurrentBuffer];
    glBindBuffer(GL_UNIFORM_BUFFER, buf);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
    glBindBufferBase(GL_UNIFORM_BUFFER, m_BindingPoint, buf);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer::SetSubData(const void* data, unsigned int offset, unsigned int size) const
{
    m_CurrentBuffer = (m_CurrentBuffer + 1) % BUFFER_COUNT;
    unsigned int buf = m_Buffers[m_CurrentBuffer];
    glBindBuffer(GL_UNIFORM_BUFFER, buf);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
    glBindBufferBase(GL_UNIFORM_BUFFER, m_BindingPoint, buf);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

unsigned int UniformBuffer::GetBindingPoint(const std::string& blockName)
{
    auto it = s_BlockBindings.find(blockName);
    if (it != s_BlockBindings.end()) return it->second;
    return 0;
}
