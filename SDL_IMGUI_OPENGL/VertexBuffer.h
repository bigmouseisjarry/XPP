#pragma once

#include <vector>

// 数据
class VertexBuffer
{
private:
	unsigned int m_RendererID;

public:
	VertexBuffer(const void* data, unsigned int size,bool UseStatic = true);
	~VertexBuffer();

	void Bind() const;
	void UnBind() const;

};