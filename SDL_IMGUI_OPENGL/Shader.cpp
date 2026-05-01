#include "Shader.h"
#include <ranges>
#include "UniformBuffer.h"

Shader::Shader(const std::string& filepath)
	:m_filePath(filepath), m_RendererID(0)
{
    ShaderProgramSource source = ParseShader(filepath);
    m_RendererID = CreateShader(source.VertexSource, source.FragmentSource);

    GLint numUniforms;
    glGetProgramiv(m_RendererID, GL_ACTIVE_UNIFORMS, &numUniforms);

    std::vector<GLuint> uniformIndices(numUniforms);
    std::vector<GLint> blockIndices(numUniforms);

    // 填充索引数组：0, 1, 2, ..., numUniforms-1
    std::ranges::copy(std::views::iota(0, numUniforms), uniformIndices.begin());

    // 批量查询所有 Uniform 所属的 Block 索引
    glGetActiveUniformsiv(m_RendererID,
        numUniforms,
		uniformIndices.data(),       // 要查询的 Uniform 索引数组
		GL_UNIFORM_BLOCK_INDEX,      // 查询每个 Uniform 所属的 Block 索引，如果是 -1 则表示不属于任何 Block（即独立 Uniform）
		blockIndices.data());        // 查询结果存储在 blockIndices 中

    for (GLint i = 0; i < numUniforms; i++)
    {
        if(blockIndices[i] != -1) continue; // 如果属于 UBO，则跳过

        char nameBuf[256];     // 用于存储 Uniform 名字的缓冲区
        GLsizei nameLen;        // 实际返回的名字长度
        GLint sz;               // Uniform 的大小（如果是数组，>1）
        GLenum type;            // Uniform 的类型（如 GL_FLOAT, GL_FLOAT_VEC3 等）
        glGetActiveUniform(m_RendererID, i, sizeof(nameBuf), &nameLen, &sz, &type, nameBuf);
        std::string name(nameBuf, nameLen);
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        if (location == -1) continue;
        m_UniformIndex[name] = m_Uniforms.size();
        m_Uniforms.push_back({ name, type, location, sz });
    }

    GLint numBlocks;
    glGetProgramiv(m_RendererID, GL_ACTIVE_UNIFORM_BLOCKS, &numBlocks);
    for (GLint i = 0; i < numBlocks; i++)
    {
        char nameBuf[256];
        GLsizei nameLen;
        glGetActiveUniformBlockName(m_RendererID, i, sizeof(nameBuf), &nameLen, nameBuf);
        std::string blockName(nameBuf, nameLen);

        GLuint blockIndex = glGetUniformBlockIndex(m_RendererID, blockName.c_str());
        GLuint binding = UniformBuffer::GetBindingPoint(blockName);
		// std::cout << "Uniform Block: " << blockName << ", Block Index: " << blockIndex << ", Binding Point: " << binding << std::endl;
        glUniformBlockBinding(m_RendererID, blockIndex, binding);
    }

}

Shader::~Shader()
{
    glDeleteProgram(m_RendererID);
}

void Shader::Bind()const
{
    glUseProgram(m_RendererID);
}

void Shader::UnBind()const
{
    glUseProgram(0);
}

void Shader::Set1f(const std::string& name, float value) const
{
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::Set4f(const std::string& name, float v0, float v1, float v2, float v3) const
{
    
    glUniform4f(GetUniformLocation(name), v0, v1, v2, v3);
}

void Shader::Set1iv(const std::string& name,GLsizei count, GLint* value) const
{
    glUniform1iv(GetUniformLocation(name), count, value);
}

void Shader::Set1i(const std::string& name, int value) const
{
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetVec4f(const std::string& name, const glm::vec4& value) const
{
    glUniform4fv(GetUniformLocation(name), 1, &value[0]);
}

void Shader::SetVec3f(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(GetUniformLocation(name), 1, &value[0]);
}

void Shader::SetVec2f(const std::string& name, const glm::vec2& value) const
{
    glUniform2fv(GetUniformLocation(name), 1, &value[0]);
}

void Shader::SetMat4f(const std::string& name, const glm::mat4& matrix) const
{
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]);
}

bool Shader::HasUniform(const std::string& name) const
{
    return m_UniformIndex.find(name) != m_UniformIndex.end();
}

ShaderProgramSource Shader::ParseShader(const std::string& filepath)
{
    std::fstream stream(filepath);
    std::string line;
    std::stringstream ss[2];

    enum class ShaderType
    {
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    ShaderType type = ShaderType::NONE;
    while (getline(stream, line))
    {
        if (line.find("#shader") != std::string::npos)
        {
            if (line.find("vertex") != std::string::npos)
                type = ShaderType::VERTEX;
            else if (line.find("fragment") != std::string::npos)
                type = ShaderType::FRAGMENT;
        }
        else
        {
            ss[(int)type] << line << '\n';
        }
    }
    return { ss[0].str(),ss[1].str() };
}

unsigned int Shader::CompileShader(unsigned int type, const std::string& source)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(sizeof(char) * length);
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << m_filePath <<" 着色器编译失败" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(id);
        return 0;
    }

    return id;
}

unsigned int Shader::CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

unsigned int Shader::GetUniformLocation(const std::string& name) const
{
    if (m_UniformIndex.find(name) != m_UniformIndex.end())
        return m_Uniforms[m_UniformIndex.at(name)].location;

    //int location = glGetUniformLocation(m_RendererID, name.c_str());
    //if (location == -1)
    //    std::cout << name << "不存在" << std::endl;

    //m_UniformLocationCache[name] = location;
    //return location;

    std::cerr << "[Shader Warning]"<< m_filePath<< ": " << " Uniform '" << name << "' not found!" << std::endl;
    return -1;
}
