#include "mesh.h"

Mesh::Mesh(const void* vertexData, unsigned int vertexSize, const unsigned int* indexData, VertexType vertextype,
    unsigned int indexCount)
{
    m_VAO = std::make_unique<VertexArray>();
    m_VBO = std::make_unique<VertexBuffer>(vertexData, vertexSize);

    VertexBufferLayout layout;
    switch (vertextype)
    {
    case VertexType::Vertex2D:
        layout.Push<float>(3);  // Position
        layout.Push<float>(2);  // UV
		m_DrawMode = GL_TRIANGLES;
        break;
    case VertexType::Vertex3D:
        layout.Push<float>(3);  // Position
        layout.Push<float>(3);  // Normal
        layout.Push<float>(2);  // UV
		layout.Push<float>(4);  // Tangent
		m_DrawMode = GL_TRIANGLES;
        break;
    case VertexType::VertexLine:
        layout.Push<float>(3);  // Position
        layout.Push<float>(4);  // Color
        m_DrawMode = GL_LINES;
		break;
    default:
        break;
    }

    m_VAO->AddBuffer(*m_VBO, layout);
    m_IBO = std::make_unique<IndexBuffer>(indexData, indexCount);

    m_VAO->UnBind();
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
