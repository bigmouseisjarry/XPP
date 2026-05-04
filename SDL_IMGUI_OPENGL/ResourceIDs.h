#pragma once
#include <cstdint>
#include <functional>

constexpr uint32_t INVALID_ID = UINT32_MAX;

// 强类型 ID
struct TextureID { uint32_t value = INVALID_ID; };
struct MeshID { uint32_t value = INVALID_ID; };
struct ShaderID { uint32_t value = INVALID_ID; };
struct FramebufferID { uint32_t value = INVALID_ID; };

// hash
struct TextureIDHash { size_t operator()(TextureID id) const { return std::hash<uint32_t>{}(id.value); } };
struct MeshIDHash { size_t operator()(MeshID id) const { return std::hash<uint32_t>{}(id.value); } };
struct ShaderIDHash { size_t operator()(ShaderID id) const { return std::hash<uint32_t>{}(id.value); } };
struct FramebufferIDHash { size_t operator()(FramebufferID id) const { return std::hash<uint32_t>{}(id.value); } };

// 比较
inline bool operator<(TextureID lhs, TextureID rhs) { return lhs.value < rhs.value; }
inline bool operator<(MeshID lhs, MeshID rhs) { return lhs.value < rhs.value; }
inline bool operator<(ShaderID lhs, ShaderID rhs) { return lhs.value < rhs.value; }
inline bool operator<(FramebufferID lhs, FramebufferID rhs) { return lhs.value < rhs.value; }

static_assert(!std::is_same_v<TextureID, MeshID>, "TextureID and MeshID should be different types");