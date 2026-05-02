#include "Texture.h"

#include "stb_image.h"

Texture::Texture(const std::string& path, int cols, int rows)
	:m_RendererID(0), m_FilePath(path), m_Width(0), m_Height(0), m_BPP(0), m_cols(cols), m_rows(rows)
{
	stbi_set_flip_vertically_on_load(1); // 纹理坐标原点在左下角，和OpenGL一致，加载时翻转图片

	unsigned char* m_LocalBuffer = stbi_load(path.c_str(), &m_Width, &m_Height, &m_BPP, 4);

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
	: m_RendererID(0), m_FilePath(path), m_Width(0), m_Height(0), m_BPP(0), m_cols(1),
	m_rows(1)
{
	stbi_set_flip_vertically_on_load(1);
	float* m_LocalBuffer = stbi_loadf(path.c_str(), &m_Width, &m_Height, &m_BPP, 4);

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
	: m_RendererID(0), m_FilePath("默认采样纹理"), m_Width(1), m_Height(1), m_BPP(4), m_cols(1), m_rows(1)
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
	: m_RendererID(0), m_FilePath("数据生成纹理"), m_Width(width), m_Height(height), m_BPP(16), m_cols(1), m_rows(1)
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

Texture::Texture(unsigned char* pixels, int width, int height, int channels,int minFilter, int magFilter, int wrapS, int wrapT, bool sRGB)
	: m_RendererID(0), m_FilePath("原始像素加载"), m_Width(width), m_Height(height), m_BPP(channels * 8), m_cols(1), m_rows(1)
{
	glGenTextures(1, &m_RendererID);
	glBindTexture(GL_TEXTURE_2D, m_RendererID);

	GLenum internalFormat, format;
	if (channels == 4) {
		internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
		format = GL_RGBA;
	}
	else if (channels == 3) {
		internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
		format = GL_RGB;
	}
	else {
		internalFormat = GL_R8;
		format = GL_RED;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
		format, GL_UNSIGNED_BYTE, pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

	if (minFilter != GL_NEAREST && minFilter != GL_LINEAR)
		glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);
}