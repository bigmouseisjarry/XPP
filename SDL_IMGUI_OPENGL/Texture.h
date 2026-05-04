#pragma once
#include <GL/glew.h>
#include<glm.hpp>
#include<iostream>
#include <string>

class Texture
{
private:
	unsigned int m_RendererID;
	std::string m_FilePath;
	int m_Width, m_Height, m_BPP;
	int m_cols;               // 多少列
	int m_rows;               // 多少行
	GLenum m_Target;		  // GL_TEXTURE_2D 或 GL_TEXTURE_CUBE_MAP

	GLenum m_InternalFormat = 0;      // 记住创建时的格式
	bool m_ClampToBorder = false;     // 深度纹理用
	bool m_ShadowCompare = false;     // 深度数组用
	int m_DepthLayers = 1;            // 深度数组层数

public:
	Texture(const std::string& path, int cols = 1, int rows = 1); 
	Texture(const std::string& path, bool isHDR);
	// 接受原始像素 + 宽高 + 通道数 + 采样参数
	Texture(unsigned char* pixels, int width, int height, int channels, int minFilter, int magFilter, int wrapS, int wrapT, bool sRGB);
	// 创建空 cubemap
	// size: 每面边长，internalFormat: 如 GL_RGBA16F，mipLevels: mip 层数
	Texture(int size, GLenum internalFormat, int mipLevels = 1);
	Texture(int width, int height, GLenum internalFormat);    // 空白 2D 纹理
	~Texture();

	void Bind(unsigned int slot = 0)const;
	void UnBind(unsigned int slot) const;

	inline int GetWidth() const { return m_Width; }
	inline int GetHeight() const { return m_Height; }
	inline int GetCols() const { return m_cols; }
	inline int GetRows() const { return m_rows; }
	inline unsigned int GetRendererID() const { return m_RendererID; }

	GLenum GetTarget() const { return m_Target; }
	inline const std::string& GetFilePath() const { return m_FilePath; }   // 调试用
	// static Texture CreateDefault(float r, float g, float b, float a = 1.0f);

	bool IsCubemap() const { return m_Target == GL_TEXTURE_CUBE_MAP; }
	void GenerateMipmap() const;

	void Resize(int newWidth, int newHeight);

private:
	Texture(float r, float g, float b, float a);
	Texture(const float* data, int width, int height);
	Texture(int width, int height, bool clampToBorder);
	Texture(int width, int height, int layers, bool clampToBorder, bool shadowCompare);
	Texture() : m_RendererID(0), m_Width(0), m_Height(0), m_BPP(0), m_cols(1), m_rows(1),m_Target(GL_TEXTURE_2D) {}

	friend class ResourceManager; 
};

