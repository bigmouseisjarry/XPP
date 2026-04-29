#pragma once
#include <GL/glew.h>
#include <string>
#include <unordered_map>

// 可以跨shader共享
class UniformBuffer
{
private:
    unsigned int m_RendererID = 0;
    unsigned int m_BindingPoint;

    static const std::unordered_map<std::string, unsigned int> s_BlockBindings;

public:
    UniformBuffer(unsigned int size, unsigned int bindingPoint);
    UniformBuffer(const void* data, size_t size, unsigned int binding);
    ~UniformBuffer();

    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    void Bind() const;
    void Unbind() const;
    void SetData(const void* data, unsigned int size) const;
    void SetSubData(const void* data, unsigned int offset, unsigned int size) const;

    unsigned int GetBindingPoint() const { return m_BindingPoint; }

    // 根据 block 名查找对应的 binding point，供 Shader 自动绑定时调用
    static unsigned int GetBindingPoint(const std::string& blockName);
};