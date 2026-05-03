#include "mesh.h"
#include <iostream>

Mesh::Mesh(const void* vertexData, unsigned int vertexSize, const unsigned int* indexData, VertexType vertextype,
    unsigned int indexCount)
{
    m_VAO = std::make_unique<VertexArray>();
    m_VBO = std::make_unique<VertexBuffer>(vertexData, vertexSize);

    switch (vertextype)
    {
    case VertexType::Vertex2D:
        m_Layout.Push<float>(3);  // Position
        m_Layout.Push<float>(2);  // UV
		m_DrawMode = GL_TRIANGLES;
        break;
    case VertexType::Vertex3D:
        m_Layout.Push<float>(3);  // Position
        m_Layout.Push<float>(3);  // Normal
        m_Layout.Push<float>(2);  // UV
		m_Layout.Push<float>(4);  // Tangent
		m_DrawMode = GL_TRIANGLES;
        break;
    case VertexType::VertexLine:
        m_Layout.Push<float>(3);  // Position
        m_Layout.Push<float>(4);  // Color
        m_DrawMode = GL_LINES;
    case VertexType::VertexParticle:            // 粒子实例数据走单独的 instance VBO
        m_Layout.Push<float>(3);  // Position
        m_Layout.Push<float>(2);  // UV
        m_DrawMode = GL_TRIANGLES;
        break;
    default:
        break;
    }

    m_VAO->AddBuffer(*m_VBO, m_Layout, 0);
    m_IBO = std::make_unique<IndexBuffer>(indexData, indexCount);
    m_IndexCount = indexCount;

    m_VAO->UnBind();
}

Mesh::Mesh(const Mesh& sharedMesh, std::unique_ptr<VertexBuffer> instanceVBO, const VertexBufferLayout& instanceLayout, unsigned int maxInstances)
    : m_Layout(sharedMesh.m_Layout), m_DrawMode(sharedMesh.m_DrawMode), m_MaxInstances(maxInstances)
{
    m_VAO = std::make_unique<VertexArray>();
    m_VAO->Bind();

    // 1. 绑定共享 VBO + IBO，用共享 layout 设置 location 0~
    sharedMesh.m_VBO->Bind();
    sharedMesh.m_IBO->Bind();
    unsigned int nextLoc = m_VAO->AddBuffer(*sharedMesh.m_VBO, sharedMesh.m_Layout, 0);

    // 2. 绑定实例 VBO，从 nextLoc 开始设置实例属性
    instanceVBO->Bind();
    unsigned int afterLoc = m_VAO->AddBuffer(*instanceVBO, instanceLayout, nextLoc);

    // 3. 设置实例属性的 divisor = 1
    unsigned int instanceAttrCount = (unsigned int)instanceLayout.GetElement().size();
    for (unsigned int loc = nextLoc; loc < afterLoc; loc++)
        glVertexAttribDivisor(loc, 1);

    m_VAO->UnBind();

    // 4. 拥有实例 VBO，共享 VBO/IBO 不拥有
    m_VBO = std::move(instanceVBO);
    // m_IBO 保持 nullptr，共享 mesh 负责 IBO 生命周期

    m_IndexCount = sharedMesh.m_IndexCount;
}

const VertexArray* Mesh::GetVAO() const
{
	return m_VAO.get();
}

const IndexBuffer* Mesh::GetIBO() const
{
	return m_IBO.get();
}

VertexBuffer* Mesh::GetVBO()
{
	return m_VBO.get();
}

void Mesh::UpdateInstanceData(const void* data, unsigned int instanceCount, unsigned int instanceSize)
{
    m_VBO->Bind();
    glBufferSubData(GL_ARRAY_BUFFER, 0, instanceCount * instanceSize, data);
    m_VBO->UnBind();
    m_InstanceCount = instanceCount;
}