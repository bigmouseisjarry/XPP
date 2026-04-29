#include "StoryScene.h"
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


StoryScene::StoryScene(SceneName sceneName)
	:Scene(sceneName)
{

}

void StoryScene::Enter()
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
	cubeTrans.SetScale(glm::vec3(20.0f, 20.0f, 1.0f));

	auto Axis = EntityFactory::CreateAxis(m_Registry);

	auto modelEntity = EntityFactory::CreateModel3D(m_Registry, "resources/model/3/scene.gltf");
	auto& modelTrans = m_Registry.get<Transform3DComponent>(modelEntity);
	modelTrans.SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));

	auto modelEntity3 = EntityFactory::CreateModel3D(m_Registry, "resources/model/6/scene.gltf");
	auto& modelTrans3 = m_Registry.get<Transform3DComponent>(modelEntity3);
	modelTrans3.SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));

	// 设置相机轨道
	auto& physics = m_Registry.get<Physics3DComponent>(modelEntity);
	auto& transform = m_Registry.get<Transform3DComponent>(modelEntity);
	glm::vec3 worldSize = physics.colliderSize * transform.scale;
	float boundingRadius = glm::length(worldSize) * 0.5f;

	OBB obb = ComputeOBB(transform.position, transform.scale,
		transform.rotation, physics.colliderSize, physics.colliderOffset);
	m_Registry.get<Camera3DComponent>(camEntity).SetOrbitTarget(obb.center, boundingRadius);

	Renderer::Get()->InitPipeline(true);
}

void StoryScene::OnEvent(SDL_Event& event)
{
}

void StoryScene::OnUpdate(float deltaTime)
{
	// 相机输入 — 从 registry 获取
	auto camView = m_Registry.view<Camera3DComponent>();
	for (auto [entity, cam] : camView.each()) {
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

	DestroySystem::Update(m_Registry);
}

void StoryScene::OnImGuiRender()
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
	ImGui::End();
}

void StoryScene::OnRender()
{

	RenderSubmitSystem::SubmitCameras(m_Registry);

	RenderSubmitSystem::SubmitEntities(m_Registry);

	DebugRenderSystem::SubmitColliderBoxes(m_Registry);

	DebugRenderSystem::SubmitLightRanges(m_Registry);

	// 收集 registry 中的 lights
	std::vector<Light3DComponent*> lights;
	auto lightView = m_Registry.view<Light3DComponent>();
	for (auto&& [entity, light] : lightView.each())
		lights.push_back(&light);

	// Scene 自己 Flush
	Renderer::Get()->Flush(lights);
}

void StoryScene::Quit()
{
	Scene::Quit();
}
