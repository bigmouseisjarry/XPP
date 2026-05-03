#include "Scene2D.h"
#include "PhysicsSystem.h"
#include "Systems.h"
#include "EntityFactory.h"
#include "ComGameplay.h"
#include "ComParticle.h"
#include "ParticleSystem.h"

Scene2D::Scene2D(SceneID sceneID)	
	:Scene(sceneID)
{
	
}

void Scene2D::Enter()
{
	m_map.Init("resources/map", m_Registry);
	EntityFactory::CreatePlayer(m_Registry);
	// Camera — 改用 registry
	EntityFactory::CreateCamera2D(m_Registry, -640.0f, 640.0f, -400.0f, 400.0f, "主游戏摄像机", 
		std::initializer_list<RenderLayer>{RenderLayer::RQ_WorldBackground,RenderLayer::RQ_WorldObjects});

	auto emitterEntity = m_Registry.create();
	m_Registry.emplace<TagComponent>(emitterEntity, TagComponent{ "ParticleEmitter" });

	auto& emitter = m_Registry.emplace<ParticleEmitterComponent>(emitterEntity);
	emitter.is3D = false;
	emitter.emitRate = 20.0f;
	emitter.maxParticles = 100;
	emitter.lifetimeMin = 1.0f;
	emitter.lifetimeMax = 2.0f;
	emitter.speedMin = 30.0f;
	emitter.speedMax = 60.0f;
	emitter.direction = { 0, -1, 0 };
	emitter.particleTexture = ResourceManager::Get()->GetTextureID("defaultParticle");
	emitter.colorOverLifetime = { {0.0f, {1,1,1,1}}, {1.0f, {1,1,1,0}} };
	emitter.sizeOverLifetime = { {0.0f, 16.0f}, {1.0f, 4.0f} };

}

void Scene2D::OnEvent(SDL_Event& event)
{

}

void Scene2D::OnUpdate(float deltaTime)
{
	if (GameSettings::Get()->IsActionPressed(InputAction::OpenSettings))
		GameSettings::Get()->ToggleUIVisible();
	

	PlayerSystem::Update(m_Registry, deltaTime);

	AnimationSystem::Update(m_Registry, deltaTime);

	CameraSystem::Update(m_Registry);

	PhysicsSystem::Get()->Update2D(m_Registry, deltaTime);

	HierarchySystem::Update(m_Registry);

	ParticleSystem::Get()->Update(m_Registry, deltaTime);

	DestroySystem::Update(m_Registry);
}

void Scene2D::OnImGuiRender()
{
	// 如果需要 ImGui 调试面板，这里用 registry view
	ImGui::Begin("Debug");
	auto view = m_Registry.view<ParticleEmitterComponent>();
	for (auto [entity, emitter] : view.each())
	{
		if (!emitter.isPlaying) continue;
		ImGui::Text("粒子数 %d", (int)emitter.m_Particles.size());
	}
	ImGui::End();

}

void Scene2D::OnRender() 
{
	RenderSubmitSystem::SubmitCameras(m_Registry);

	RenderSubmitSystem::SubmitEntities(m_Registry);

	m_map.OnRender();

	ParticleSystem::Get()->SubmitRender(m_Registry);

	Renderer::Get()->Flush({});
}

void Scene2D::Quit()
{
	Scene::Quit();
}

void Scene2D::BuildPipeline(RenderPipeline& pipeline)
{
	Scene::BuildDefaultPipeline2D(pipeline);
}
