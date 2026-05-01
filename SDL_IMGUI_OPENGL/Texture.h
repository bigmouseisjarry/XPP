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
	unsigned char* m_LocalBuffer;
	int m_Width, m_Height, m_BPP;
	int m_cols;               // 多少列
	int m_rows;               // 多少行

public:
	// 这是解决move语义的关键，解决双方资源竞争问题，避免重复释放同一资源
	//Texture(Texture&& other) noexcept;
	//Texture& operator=(Texture&& other) noexcept;
	Texture(const std::string& path, int cols = 1, int rows = 1); 
	Texture(const std::string& path, bool isHDR);
	~Texture();

	void Bind(unsigned int slot = 0)const;
	void UnBind(unsigned int slot) const;

	inline int GetWidth() const { return m_Width; }
	inline int GetHeight() const { return m_Height; }
	inline int GetCols() const { return m_cols; }
	inline int GetRows() const { return m_rows; }
	inline unsigned int GetRendererID() const { return m_RendererID; }


	inline const std::string& GetFilePath() const { return m_FilePath; }   // 调试用
	// static Texture CreateDefault(float r, float g, float b, float a = 1.0f);

private:
	Texture(float r, float g, float b, float a);
	Texture(const float* data, int width, int height);
	Texture(const std::string& path, int cols, int rows ,int minFilter, int magFilter, int wrapS, int wrapT, bool sRGB);
	// 从内存中加载
	Texture(const unsigned char* data, int len, int minFilter, int magFilter, int wrapS, int wrapT, bool sRGB);
	Texture() : m_RendererID(0), m_LocalBuffer(nullptr), m_Width(0), m_Height(0), m_BPP(0), m_cols(1), m_rows(1) {}
	friend class ResourceManager; 
};

