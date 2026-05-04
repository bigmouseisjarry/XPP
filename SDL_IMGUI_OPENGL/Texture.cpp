#include "Texture.h"

#include "stb_image.h"

Texture::Texture(const std::string& path, int cols, int rows)
	:m_RendererID(0), m_FilePath(path), m_Width(0), m_Height(0), m_BPP(0), m_cols(cols), m_rows(rows),m_Target(GL_TEXTURE_2D)
{
	stbi_set_flip_vertically_on_load(1); // 纹理坐标原点在左下角，和OpenGL一致，加载时翻转图片

	unsigned char* m_LocalBuffer = stbi_load(path.c_str(), &m_Width, &m_Height, &m_BPP, 4);

	glGenTextures(1, &m_RendererID);
	glBindTexture(m_Target, m_RendererID);

	glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(m_Target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(m_Target, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_LocalBuffer);
	glBindTexture(m_Target, 0);

	if (m_LocalBuffer)
		stbi_image_free(m_LocalBuffer);
}

Texture::Texture(const std::string& path, bool isHDR)
	: m_RendererID(0), m_FilePath(path), m_Width(0), m_Height(0), m_BPP(0), m_cols(1),m_rows(1), m_Target(GL_TEXTURE_2D)
{
	stbi_set_flip_vertically_on_load(1);
	float* m_LocalBuffer = stbi_loadf(path.c_str(), &m_Width, &m_Height, &m_BPP, 4);

	glGenTextures(1, &m_RendererID);
	glBindTexture(m_Target, m_RendererID);

	glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(m_Target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(m_Target, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, m_LocalBuffer);
	glBindTexture(m_Target, 0);

	if (m_LocalBuffer)
		stbi_image_free(m_LocalBuffer);
}

Texture::Texture(int size, GLenum internalFormat, int mipLevels)
	:m_Target(GL_TEXTURE_CUBE_MAP), m_InternalFormat(internalFormat)
{
	glGenTextures(1, &m_RendererID);
	glBindTexture(m_Target, m_RendererID);

	GLenum format = GL_RGBA;
	if (internalFormat == GL_RG16F) format = GL_RG;

	// 为 6 个面分配存储，每个 mip level 都要分配
	for (int level = 0; level < mipLevels; level++)
	{
		int mipSize = size >> level;
		for (int i = 0; i < 6; i++)
		{
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,  // 6个面：+X, -X, +Y, -Y, +Z, -Z
				level,                                // mip level
				internalFormat,                       // 如 GL_RGBA16F
				mipSize, mipSize,                     // 每面是正方形
				0,
				format, GL_FLOAT,                    // format + type
				nullptr                               // 空数据，后面渲染填入
			);
		}
	}

	// Cubemap 必须 clamp to edge，否则面与面之间会有接缝
	glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(m_Target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);  // R 是第三维

	// 过滤模式
	if (mipLevels > 1)
	{
		glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(m_Target, GL_TEXTURE_MAX_LEVEL, mipLevels - 1);
	}
	else
	{
		glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	glTexParameteri(m_Target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(m_Target, 0);
}

Texture::~Texture()
{
	glDeleteTextures(1, &m_RendererID);
}

void Texture::Bind(unsigned int slot) const
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(m_Target, m_RendererID);
}

void Texture::UnBind(unsigned int slot)const
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(m_Target, 0);
}

void Texture::GenerateMipmap() const
{
	glBindTexture(m_Target, m_RendererID);
	glGenerateMipmap(m_Target);
	glBindTexture(m_Target, 0);
}

void Texture::Resize(int newWidth, int newHeight)
{
	glDeleteTextures(1, &m_RendererID);
	m_Width = newWidth;
	m_Height = newHeight;

	glGenTextures(1, &m_RendererID);

	if (m_Target == GL_TEXTURE_2D)
	{
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		if (m_InternalFormat == GL_DEPTH_COMPONENT24)
		{
			// 深度 2D
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_Width, m_Height, 0,
				GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			if (m_ClampToBorder) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
				glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
			}
			else {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
		}
		else
		{
			// 颜色 2D（RGBA16F, RG16F 等）
			GLenum format = (m_InternalFormat == GL_RG16F) ? GL_RG : GL_RGBA;
			glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0,
				format, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else if (m_Target == GL_TEXTURE_2D_ARRAY)
	{
		// 深度数组
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_RendererID);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24,
			m_Width, m_Height, m_DepthLayers, 0,
			GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
			m_ShadowCompare ? GL_LINEAR : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER,
			m_ShadowCompare ? GL_LINEAR : GL_NEAREST);
		if (m_ClampToBorder) {
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
		}
		else {
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}
}

Texture::Texture(float r, float g, float b, float a)
	: m_RendererID(0), m_FilePath("默认采样纹理"), m_Width(1), m_Height(1), m_BPP(4), m_cols(1), m_rows(1), m_Target(GL_TEXTURE_2D)
{

	unsigned char pixels[4] = {
		(unsigned char)(r * 255),
		(unsigned char)(g * 255),
		(unsigned char)(b * 255),
		(unsigned char)(a * 255)
	};
	glGenTextures(1, &m_RendererID);
	glBindTexture(m_Target, m_RendererID);
	glTexImage2D(m_Target, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(m_Target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(m_Target, 0);
}

Texture::Texture(const float* data, int width, int height)
	: m_RendererID(0), m_FilePath("数据生成纹理"), m_Width(width), m_Height(height), m_BPP(16), m_cols(1), m_rows(1), m_Target(GL_TEXTURE_2D)
{
	glGenTextures(1, &m_RendererID);
	glBindTexture(m_Target, m_RendererID);
	glTexImage2D(m_Target, 0, GL_RGBA16F, m_Width, m_Height, 0,
		GL_RGBA, GL_FLOAT, data);

	glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(m_Target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(m_Target, 0);
}

Texture::Texture(unsigned char* pixels, int width, int height, int channels,int minFilter, int magFilter, int wrapS, int wrapT, bool sRGB)
	: m_RendererID(0), m_FilePath("原始像素加载"), m_Width(width), m_Height(height), m_BPP(channels * 8), m_cols(1), m_rows(1), m_Target(GL_TEXTURE_2D)
{
	glGenTextures(1, &m_RendererID);
	glBindTexture(m_Target, m_RendererID);

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

	glTexImage2D(m_Target, 0, internalFormat, width, height, 0,
		format, GL_UNSIGNED_BYTE, pixels);

	glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(m_Target, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, wrapT);

	if (minFilter != GL_NEAREST && minFilter != GL_LINEAR)
		glGenerateMipmap(m_Target);

	glBindTexture(m_Target, 0);
}

Texture::Texture(int width, int height, bool clampToBorder)
	:m_RendererID(0), m_FilePath("深度纹理"), m_Width(width), m_Height(height), m_BPP(3), m_cols(1), m_rows(1), m_Target(GL_TEXTURE_2D),
	m_InternalFormat(GL_DEPTH_COMPONENT24),m_ClampToBorder(clampToBorder)
{
	glGenTextures(1, &m_RendererID);
	glBindTexture(m_Target, m_RendererID);
	glTexImage2D(m_Target, 0, GL_DEPTH_COMPONENT24, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(m_Target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (clampToBorder)
	{
		glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(m_Target, GL_TEXTURE_BORDER_COLOR, borderColor);
	}
	else
	{
		glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	glBindTexture(m_Target, 0);
}

Texture::Texture(int width, int height, int layers, bool clampToBorder, bool shadowCompare)
	:m_RendererID(0), m_FilePath("深度纹理数组"), m_Width(width), m_Height(height), m_BPP(3), m_cols(1), m_rows(1), m_Target(GL_TEXTURE_2D_ARRAY),
	m_InternalFormat(GL_DEPTH_COMPONENT24),m_ClampToBorder(clampToBorder),m_ShadowCompare(shadowCompare),m_DepthLayers(layers)
{

	glGenTextures(1, &m_RendererID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_RendererID);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, m_Width, m_Height, layers, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	if (shadowCompare)
	{
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	if (clampToBorder)
	{
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

Texture::Texture(int width, int height, GLenum internalFormat)
	: m_RendererID(0), m_FilePath("Empty2D"), m_Width(width), m_Height(height), m_BPP(16), m_cols(1), m_rows(1), m_Target(GL_TEXTURE_2D),
	m_InternalFormat(internalFormat)
{
	GLenum format = (internalFormat == GL_RG16F) ? GL_RG : GL_RGBA;

	glGenTextures(1, &m_RendererID);
	glBindTexture(GL_TEXTURE_2D, m_RendererID);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
		format, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);
}