#pragma once

#include "VertexBuffer.h"
#include "VertexBufferLayout.h"


// 说明书
class VertexArray
{
private:
	unsigned int m_RendererID;
public:
	VertexArray();
	~VertexArray();

	unsigned int GetRendererID() const { return m_RendererID; }

	void Bind() const;
	void UnBind() const;
	unsigned int AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout, unsigned int start = 0);
};