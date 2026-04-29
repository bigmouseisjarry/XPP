#pragma once

#include<GL/glew.h>

#include<glm.hpp>
#include<string>
#include<sstream>
#include<fstream>
#include<iostream>
#include<unordered_map>

struct ShaderProgramSource
{
	std::string VertexSource;
	std::string FragmentSource;
};

struct UniformInfo
{
	std::string name;
	GLenum type;
	int location;
	int size;         // 如果是数组，size 就是数组长度；如果不是数组，size 就是 1
};

class Shader
{
private:
	std::string m_filePath;
	unsigned int m_RendererID;

	// Uniform 信息列表和名字到索引的映射，方便后续设置 uniform 时快速查找
	std::vector<UniformInfo> m_Uniforms;
	std::unordered_map<std::string, size_t> m_UniformIndex;

public:
	Shader(const std::string& filepath);
	~Shader();

	void Bind()const;
	void UnBind()const;

	const std::vector<UniformInfo>& GetUniforms() const { return m_Uniforms; }

	void Set1f(const std::string& name, float value) const;
	void Set4f(const std::string& name, float v0, float v1, float v2, float v3) const;
	void Set1iv(const std::string& name, GLsizei count, GLint* value) const;
	void Set1i(const std::string& name, int value) const;
	void SetVec4f(const std::string& name,const glm::vec4& value)const;
	void SetVec3f(const std::string& name, const glm::vec3& value) const;
	void SetVec2f(const std::string& name,const glm::vec2& value)const;
	void SetMat4f(const std::string& name,const glm::mat4& matrix) const;

	// 检查 uniform 是否存在
	bool HasUniform(const std::string& name) const;
private:
	ShaderProgramSource ParseShader(const std::string& filepath);
	unsigned int CompileShader(unsigned int type, const std::string& source);
	unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader);
	unsigned int GetUniformLocation(const std::string& name)const;
};