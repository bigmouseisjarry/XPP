#include "VertexArray.h"

VertexArray::VertexArray()
{
	glGenVertexArrays(1, &m_RendererID);
	glBindVertexArray(m_RendererID);
}

VertexArray::~VertexArray()
{
	glDeleteVertexArrays(1, &m_RendererID);
}

unsigned int VertexArray::AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout, unsigned int start)
{
	Bind();
	vb.Bind();
	const auto& elements = layout.GetElement();
	unsigned int offset = 0;
	for (unsigned int i = 0; i < elements.size(); i++)
	{
		const auto& element = elements[i];
		unsigned int loc = start + i;
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(loc , element.count, element.type,
			element.normalized, layout.GetStride(), (const void*)offset);
		offset += element.count * element.GetSizeOfType(element.type);
	}
	return start + (unsigned int)elements.size();
}

void VertexArray::Bind() const
{
	glBindVertexArray(m_RendererID);
}

void VertexArray::UnBind() const
{
	glBindVertexArray(0);
}
