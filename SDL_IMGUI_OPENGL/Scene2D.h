#pragma once

#include<string>
#include <memory>
#include "mesh.h"
#include "GameSettings.h"
#include "Map.h"
#include "Scene.h"

class Scene2D : public Scene
{
private:
	Map m_map;
public:
	Scene2D(SceneID sceneID);

	void Enter() override;
	void OnEvent(SDL_Event& event)override;
	void OnUpdate(float deltaTime)override;
	void OnImGuiRender()override;
	void OnRender() override;
	void Quit()override;

	void BuildPipeline(RenderPipeline& pipeline) override;
};