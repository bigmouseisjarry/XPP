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
	VertexBufferLayout m_Layout;

	unsigned int m_IndexCount = 0;
	unsigned int m_InstanceCount = 0;					// 当前活着的粒子数
	unsigned int m_MaxInstances = 0;					// 缓冲区最大容量
public:
	Mesh(const void* vertexData, unsigned int vertexSize, const unsigned int* indexData, VertexType vertextype, unsigned int indexCount);
	Mesh(const Mesh& sharedMesh, std::unique_ptr<VertexBuffer> instanceVBO, const VertexBufferLayout& instanceLayout, unsigned int maxInstances);
	const VertexArray* GetVAO() const;
	const IndexBuffer* GetIBO() const;
	VertexBuffer* GetVBO();

	GLenum GetDrawMode() const { return m_DrawMode; }
	const VertexBufferLayout& GetLayout() const { return m_Layout; }
	unsigned int GetIndexCount() const { return m_IndexCount; }
	unsigned int GetMaxInstances() const { return m_MaxInstances; }
	unsigned int GetInstanceCount() const { return m_InstanceCount; }

	void UpdateInstanceData(const void* data, unsigned int instanceCount, unsigned int instanceSize);
};
