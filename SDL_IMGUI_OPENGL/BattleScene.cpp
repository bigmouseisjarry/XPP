#include "BattleScene.h"
#include "PhysicsSystem.h"
#include "Systems.h"
#include "EntityFactory.h"

BattleScene::BattleScene(SceneName sceneName)	
	:Scene(sceneName)
{
	
}

void BattleScene::Enter()
{
	m_map.Init("resources/map", m_Registry);
	EntityFactory::CreatePlayer(m_Registry);
	// Camera — 改用 registry
	EntityFactory::CreateCamera2D(m_Registry, -640.0f, 640.0f, -400.0f, 400.0f, "主游戏摄像机", 
		std::initializer_list<RenderLayer>{RenderLayer::RQ_WorldBackground,RenderLayer::RQ_WorldObjects});

	Renderer::Get()->InitPipeline(false);
}

void BattleScene::OnEvent(SDL_Event& event)
{

}

void BattleScene::OnUpdate(float deltaTime)
{
	if (GameSettings::Get()->IsActionPressed(InputAction::OpenSettings))
		GameSettings::Get()->ToggleUIVisible();
	

	PlayerSystem::Update(m_Registry, deltaTime);

	AnimationSystem::Update(m_Registry, deltaTime);

	CameraSystem::Update(m_Registry);

	PhysicsSystem::Get()->Update2D(m_Registry, deltaTime);

	HierarchySystem::Update(m_Registry);

	DestroySystem::Update(m_Registry);
}

void BattleScene::OnImGuiRender()
{
	// 如果需要 ImGui 调试面板，这里用 registry view
}

void BattleScene::OnRender() 
{
	RenderSubmitSystem::SubmitCameras(m_Registry);

	RenderSubmitSystem::SubmitEntities(m_Registry);

	m_map.OnRender();

	Renderer::Get()->Flush({});
}

void BattleScene::Quit()
{
	Scene::Quit();
}
