#pragma once
#include"VertexArray.h"
#include"Texture.h"
#include"IndexBuffer.h"
#include"Shader.h"
#include"Vertex.h"
#include "mesh.h"
#include "Material.h"
#include "ResourceManager.h"
#include "ComLight.h"
#include "UniformBuffer.h"
#include "RenderPipeline.h"
#include "RenderTypes.h"

class Renderer
{
private:
	
	std::unordered_map<RenderLayer, std::vector<RenderUnit>> m_RenderUnits;

	std::vector<CameraUnit> m_Camera2DUnits;
	std::vector<CameraUnit> m_Camera3DUnits;

	std::vector<InstancedRenderUnit> m_InstancedUnits;

	std::unique_ptr<UniformBuffer> m_PerFrameUBO;
	std::unique_ptr<UniformBuffer> m_PerObjectUBO;
	std::unique_ptr<UniformBuffer> m_LightUBO;
	std::unique_ptr<UniformBuffer> m_SSAOKernelUBO;
	std::unique_ptr<RenderPipeline> m_Pipeline;  
	ViewportState m_Viewport;

	std::vector<std::pair<FramebufferID, glm::vec2>> m_IntermediateFBOs;

	FramebufferID m_ShadowArrayFBOID = { INVALID_ID };

	TextureID m_SkyboxTexID = { INVALID_ID };
	glm::mat4 m_Projection{ 1.0f };
public:
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	static Renderer* Get();

	FramebufferID GetShadowArrayFBOID() const { return m_ShadowArrayFBOID; }
	TextureID GetSkyboxTexID() const { return m_SkyboxTexID; }

	void SetProjection(const glm::mat4& proj) { m_Projection = proj; }
	const glm::mat4& GetProjection() const { return m_Projection; }
	const ViewportState& GetViewport() const { return m_Viewport; }

	void InitUBO();
	void Clear();
	void SortAll();
	void Flush(const std::vector<Light3DComponent*>& lights);  //TODO:不应该传值

	void SetShadowArrayFBOID(FramebufferID id) { m_ShadowArrayFBOID = id; }
	void SetSkyboxTexID(TextureID id) { m_SkyboxTexID = id; }
	void SetSSAOKernelUBO(std::unique_ptr<UniformBuffer> ubo) { m_SSAOKernelUBO = std::move(ubo); }
	void SetPipeline(std::unique_ptr<RenderPipeline> pipeline) { m_Pipeline = std::move(pipeline); }

	void SetWindowSize(int w, int h);
	void RegisterIntermediateFBO(FramebufferID fboID, float scaleX = 1.0f, float scaleY = 1.0f);

	void SubmitCameraUnits(const glm::mat4& projView, const std::vector<RenderLayer>& layers,RenderMode mode, const glm::vec3& viewPos = glm::vec3(0.0f));
	void SubmitRenderUnits(const MeshID& mesh, const Material* material,const glm::mat4& model,RenderLayer renderlayer = RenderLayer::RQ_WorldBackground);
	void SubmitInstancedUnits(MeshID mesh, const Material* material, RenderLayer layer, unsigned int instanceCount, bool additiveBlend);

private:
	Renderer() {}
	~Renderer() {}

};
