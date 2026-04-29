#include <GL/glew.h>
#include "VertexBuffer.h"


// 静态绘制：data是顶点数组的保存头指针位置，size是这个数组的大小
// 动态绘制：data是nullptr（后续每帧绑定），size是预先分配多少大小
VertexBuffer::VertexBuffer(const void* data, unsigned int size, bool UseStatic)
{
    glGenBuffers(1, &m_RendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    if (UseStatic)
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    else
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
}

VertexBuffer::~VertexBuffer()
{
    glDeleteBuffers(1, &m_RendererID);
}

void VertexBuffer::Bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
}

void VertexBuffer::UnBind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

