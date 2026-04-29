#include "Texture.h"

#include "stb_image.h"

//Texture::Texture(Texture&& other) noexcept
//    : m_RendererID(other.m_RendererID)
//    , m_FilePath(std::move(other.m_FilePath))
//    , m_LocalBuffer(other.m_LocalBuffer)
//    , m_Width(other.m_Width)
//    , m_Height(other.m_Height)
//    , m_BPP(other.m_BPP)
//    , m_cols(other.m_cols)
//    , m_rows(other.m_rows)
//{
//    other.m_RendererID = 0;
//    other.m_LocalBuffer = nullptr;
//}
//
//Texture& Texture::operator=(Texture&& other) noexcept
//{
//    if (this != &other)
//    {
//        glDeleteTextures(1, &m_RendererID);
//        if (m_LocalBuffer) stbi_image_free(m_LocalBuffer);
//
//        m_RendererID = other.m_RendererID;
//        m_FilePath = std::move(other.m_FilePath);
//        m_LocalBuffer = other.m_LocalBuffer;
//        m_Width = other.m_Width;
//        m_Height = other.m_Height;
//        m_BPP = other.m_BPP;
//        m_cols = other.m_cols;
//        m_rows = other.m_rows;
//
//        other.m_RendererID = 0;
//        other.m_LocalBuffer = nullptr;
//    }
//    return *this;
//}

Texture::Texture(const std::string& path, int cols, int rows)
	:m_RendererID(0), m_FilePath(path), m_LocalBuffer(nullptr), m_Width(0), m_Height(0), m_BPP(0), m_cols(cols), m_rows(rows)
{
	stbi_set_flip_vertically_on_load(1); // 纹理坐标原点在左下角，和OpenGL一致，加载时翻转图片

	m_LocalBuffer = stbi_load(path.c_str(), &m_Width, &m_Height, &m_BPP, 4);

	glGenTextures(1, &m_RendererID);
	glBindTexture(GL_TEXTURE_2D, m_RendererID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_LocalBuffer);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (m_LocalBuffer)
		stbi_image_free(m_LocalBuffer);
}

Texture::Texture(const std::string& path, bool isHDR)
	: m_RendererID(0), m_FilePath(path), m_LocalBuffer(nullptr), m_Width(0), m_Height(0), m_BPP(0), m_cols(1),
	m_rows(1)
{
	stbi_set_flip_vertically_on_load(1);
	m_LocalBuffer = (unsigned char*)stbi_loadf(path.c_str(), &m_Width, &m_Height, &m_BPP, 4);

	glGenTextures(1, &m_RendererID);
	glBindTexture(GL_TEXTURE_2D, m_RendererID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, m_LocalBuffer);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (m_LocalBuffer)
		stbi_image_free(m_LocalBuffer);
}

Texture::~Texture()
{
	glDeleteTextures(1, &m_RendererID);
}

void Texture::Bind(unsigned int slot) const
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_RendererID);
}

// TODO: 可以删除这个函数，带solt会直接覆盖绑定
void Texture::UnBind(unsigned int slot)const
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::Texture(float r, float g, float b, float a)
	: m_RendererID(0), m_FilePath("默认采样纹理"), m_LocalBuffer(nullptr), m_Width(1), m_Height(1), m_BPP(4), m_cols(1), m_rows(1)
{

	unsigned char pixels[4] = {
		(unsigned char)(r * 255),
		(unsigned char)(g * 255),
		(unsigned char)(b * 255),
		(unsigned char)(a * 255)
	};
	glGenTextures(1, &m_RendererID);
	glBindTexture(GL_TEXTURE_2D, m_RendererID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::Texture(const float* data, int width, int height)
	: m_RendererID(0), m_FilePath("噪声纹理"), m_LocalBuffer(nullptr),
	m_Width(width), m_Height(height), m_BPP(16), m_cols(1), m_rows(1)
{
	glGenTextures(1, &m_RendererID);
	glBindTexture(GL_TEXTURE_2D, m_RendererID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0,
		GL_RGBA, GL_FLOAT, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, 0);
}