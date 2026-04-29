#pragma once

#include "VertexArray.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include <memory>
#include "Vertex.h"

class Mesh
{
private:
	GLenum m_DrawMode = GL_TRIANGLES;
	std::unique_ptr<VertexArray> m_VAO;
	std::unique_ptr<VertexBuffer> m_VBO;
	std::unique_ptr<IndexBuffer> m_IBO;
public:
	Mesh(const void* vertexData, unsigned int vertexSize, const unsigned int* indexData, VertexType vertextype, unsigned int indexCount);
	const VertexArray* GetVAO() const;
	const IndexBuffer* GetIBO() const;
	VertexBuffer* GetVBO();
	GLenum GetDrawMode() const { return m_DrawMode; }
};
