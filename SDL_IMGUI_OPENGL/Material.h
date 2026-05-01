#pragma once

#include<glm.hpp>
#include<string>
#include<iostream>
#include "ResourceManager.h"
#include <variant>
#include "TextureSemantic.h"

using UniformValue = std::variant<
    float,
    int,
    glm::vec2,
    glm::vec3,
    glm::vec4,
    glm::mat4
>;

struct Material
{
    ShaderID m_Shader;
	std::unordered_map<std::string, UniformValue> m_Properties;    // 通用 uniform 属性，shader 里通过名字访问
    std::unordered_map<TextureSemantic, TextureID, TextureSemanticHash> m_Textures;
    bool   m_Transparent = false;
    bool   m_DoubleSided = false;           
    int    m_RenderOrder = 0;


    void SetTexture(TextureSemantic semantic, TextureID textureID) { m_Textures[semantic] = textureID; }
    TextureID GetTexture(TextureSemantic semantic) const
    {
        auto it = m_Textures.find(semantic);
        return it != m_Textures.end() ? it->second : TextureID{ INVALID_ID };
	}

    void Set(const std::string& name, const UniformValue& value) { m_Properties[name] = value; }
    const UniformValue* Get(const std::string& name) const
    {
        auto it = m_Properties.find(name);
        return it != m_Properties.end() ? &it->second : nullptr;
    }
};
