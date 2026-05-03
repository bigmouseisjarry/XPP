#include "Scene3D.h"
#include "GameSettings.h"
#include "EntityFactory.h"
#include "ComCamera.h"
#include "Systems.h"
#include "ComRender.h"
#include "ComTransform.h"
#include "ComPhysics.h"
#include "PhysicsSystem.h"
#include "ComGameplay.h"
#include "DebugRenderSystem.h"
#include "ComParticle.h"
#include "ParticleSystem.h"


Scene3D::Scene3D(SceneID sceneID)
	:Scene(sceneID)
{

}

void Scene3D::Enter()
{
	// Light 
	auto lightEntity = EntityFactory::CreateLight3D(m_Registry, glm::vec3(12.0f, 12.0f, 12.0f));
	auto& lightComp = m_Registry.get<Light3DComponent>(lightEntity);
	lightComp.intensity = 3.0f;
	lightComp.color = glm::vec3(1.0f);
	lightComp.target = glm::vec3(0.0f, 0.0f, 0.0f);

	auto light2 = EntityFactory::CreateLight3D(m_Registry, glm::vec3(-10.0f, 8.0f, 15.0f));
	auto& lightComp2 = m_Registry.get<Light3DComponent>(light2);
	lightComp2.intensity = 2.0f;
	lightComp2.color = glm::vec3(1.0f, 0.9f, 0.6f);  
	lightComp2.target = glm::vec3(5.0f, 0.0f, 5.0f);

	// Camera 
	auto camEntity = EntityFactory::CreateCamera3D(m_Registry, 45.0f, 1280.0f / 800.0f, 0.1f, 500.0f,
		"剧情摄像机", std::initializer_list<RenderLayer>{RenderLayer::RQ_WorldObjects, RenderLayer::RQ_WorldBackground});
	
	auto cube = EntityFactory::CreateCube(m_Registry);
	auto& cubeTrans = m_Registry.get<Transform3DComponent>(cube);
	cubeTrans.SetScale(glm::vec3(200.0f, 200.0f, 1.0f));

	auto Axis = EntityFactory::CreateAxis(m_Registry);

	auto modelEntity = EntityFactory::CreateModel3D(m_Registry, "resources/model/3/scene.gltf");
	auto& modelTrans = m_Registry.get<Transform3DComponent>(modelEntity);
	modelTrans.SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));

	//auto modelEntity2 = EntityFactory::CreateModelHierarchy(m_Registry, "resources/model/7/scene.gltf");
	//auto& modelTrans2 = m_Registry.get<Transform3DComponent>(modelEntity2);
	//modelTrans2.SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));

	EntityFactory::CreateModel3DAsync("resources/model/6/scene.gltf");
	//auto& modelTrans3 = m_Registry.get<Transform3DComponent>(modelEntity3);
	//modelTrans3.SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));

	// 设置相机轨道
	auto& physics = m_Registry.get<Physics3DComponent>(modelEntity);
	auto& transform = m_Registry.get<Transform3DComponent>(modelEntity);
	glm::vec3 worldSize = physics.colliderSize * transform.scale;
	float boundingRadius = glm::length(worldSize) * 0.5f;

	OBB obb = ComputeOBB(transform.position, transform.scale,
		transform.rotation, physics.colliderSize, physics.colliderOffset);
	m_Registry.get<Camera3DComponent>(camEntity).SetOrbitTarget(obb.center, boundingRadius);

	auto emitterEntity = m_Registry.create();
	m_Registry.emplace<TagComponent>(emitterEntity, TagComponent{ "ParticleEmitter" });
	m_Registry.emplace<Transform3DComponent>(emitterEntity);

	auto& emitter = m_Registry.emplace<ParticleEmitterComponent>(emitterEntity);
	emitter.is3D = true;
	emitter.m_Material->Set("u_Dimension", (int)1);
	emitter.emitRate = 50.0f;
	emitter.maxParticles = 200;
	emitter.lifetimeMin = 0.5f;
	emitter.lifetimeMax = 1.5f;
	emitter.speedMin = 2.0f;
	emitter.speedMax = 5.0f;
	emitter.direction = { 0, 0, 1 };
	emitter.gravity = { 0, 0, -2.0f };
	emitter.particleTexture = ResourceManager::Get()->GetTextureID("defaultParticle");
	emitter.additiveBlend = true;
	emitter.colorOverLifetime = { {0.0f, {1,0.8f,0.2f,1}}, {1.0f, {1,0.2f,0,0}} };
	emitter.sizeOverLifetime = { {0.0f, 1.0f}, {1.0f, 0.1f} };

}

void Scene3D::OnEvent(SDL_Event& event)
{
}

void Scene3D::OnUpdate(float deltaTime)
{
	// 相机输入 — 从 registry 获取
	auto camView = m_Registry.view<Camera3DComponent, Transform3DComponent>();
	for (auto [entity, cam,tr] : camView.each()) {
		if (GameSettings::Get()->IsActionPressed(InputAction::CameraZoomOut) ||
			GameSettings::Get()->IsActionPressed(InputAction::CameraZoomIn)) {
			cam.Dolly(GameSettings::Get()->GetMouseState().wheelDelta);
		}
		if (GameSettings::Get()->IsActionDown(InputAction::CameraPan)) {
			cam.Pan(glm::vec2(-GameSettings::Get()->GetMouseState().deltaX * 0.01f,
				GameSettings::Get()->GetMouseState().deltaY * 0.01f));
		}
		if (GameSettings::Get()->IsActionDown(InputAction::CameraOrbitFocus)) {
			cam.Orbit(-GameSettings::Get()->GetMouseState().deltaY * 0.3f, -GameSettings::Get()->GetMouseState().deltaX * 0.3f
				);
		}
	}

	PhysicsSystem::Get()->Update3D(m_Registry, deltaTime);

	HierarchySystem::Update(m_Registry);

	LightSystem::Update(m_Registry);

	CameraSystem::Update(m_Registry);

	ParticleSystem::Get()->Update(m_Registry, deltaTime);

	DestroySystem::Update(m_Registry);

	//// 从摄像机位置向前发射射线
	//for (auto [entity, cam, transform] : camView.each()) {
	//	glm::vec3 origin = transform.position;
	//	glm::vec3 dir = cam.GetForward();  // 需要摄像机有前方向量
	//	RaycastHit hit = PhysicsSystem::Get()->Raycast(m_Registry, origin, dir, 100.0f);
	//	if (hit.entity != entt::null) {
	//		auto& tag = m_Registry.get<TagComponent>(hit.entity);
	//		std::cout << "命中: " << tag.name << " 距离: " << hit.distance << std::endl;
	//	}
	//}

}

void Scene3D::OnImGuiRender()
{
	auto entity_tag =  m_Registry.view<TagComponent>();
	entt::entity m_entity;
	for(auto [entity, tag] : entity_tag.each()) {
		if (tag.name == "Model3D") 
		{
			m_entity = entity;
			break;
		}
	}
	ImGui::Begin("Debug");
	ImGui::Checkbox("Show Collider Boxes", &DebugRenderSystem::s_ShowColliders);
	ImGui::Checkbox("Show Light Ranges", &DebugRenderSystem::s_ShowLightRanges);
	ImGui::SliderFloat3("Model Position", &m_Registry.get<Transform3DComponent>(m_entity).position.x, -10.0f, 10.0f);
	int lightIndex = 0;
	auto lightView = m_Registry.view<Light3DComponent, Transform3DComponent>();
	for (auto [entity, light, transform] : lightView.each())
	{
		if (ImGui::CollapsingHeader(("Light " + std::to_string(lightIndex)).c_str()))
		{
			std::string id = "##" + std::to_string(lightIndex);
			ImGui::DragFloat3(("Position" + id).c_str(), &transform.position.x, 0.1f);
			ImGui::ColorEdit3(("Color" + id).c_str(), &light.color.x);
			ImGui::SliderFloat(("Intensity" + id).c_str(), &light.intensity, 0.0f, 10.0f);
			ImGui::Checkbox(("Cast Shadow" + id).c_str(), &light.castShadow);
			ImGui::DragFloat3(("Target" + id).c_str(), &light.target.x, 0.1f);
		}
		lightIndex++;
	}
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Begin("Debug");
	auto view = m_Registry.view<ParticleEmitterComponent>();
	for (auto [entity, emitter] : view.each())
	{
		if (!emitter.isPlaying) continue;
		ImGui::Text("粒子数 %d", (int)emitter.m_Particles.size());
		for (auto& p : emitter.m_Particles)
		{
			//std::cout << " 位置： " << p.position.x << " " << p.position.y << " " << p.position.z << std::endl;
			//std::cout << " 大小：" << p.size << std::endl;
		}
	}
	ImGui::End();
	ImGui::End();
}

void Scene3D::OnRender()
{

	RenderSubmitSystem::SubmitCameras(m_Registry);

	RenderSubmitSystem::SubmitEntities(m_Registry);

	DebugRenderSystem::SubmitColliderBoxes(m_Registry);

	DebugRenderSystem::SubmitLightRanges(m_Registry);

	ParticleSystem::Get()->SubmitRender(m_Registry);

	// 收集 registry 中的 lights
	std::vector<Light3DComponent*> lights;
	auto lightView = m_Registry.view<Light3DComponent>();
	for (auto&& [entity, light] : lightView.each())
		lights.push_back(&light);

	// Scene 自己 Flush
	Renderer::Get()->Flush(lights);
}

void Scene3D::Quit()
{
	Scene::Quit();
}

void Scene3D::BuildPipeline(RenderPipeline& pipeline)
{
	Scene::BuildDefaultPipeline3D(pipeline);
}
