#pragma once

#include<string>
#include <memory>
#include "mesh.h"
#include "GameSettings.h"
#include "Map.h"
#include "Scene.h"

class BattleScene : public Scene
{
private:
	Map m_map;
public:
	BattleScene(SceneName sceneName);

	void Enter() override;
	void OnEvent(SDL_Event& event)override;
	void OnUpdate(float deltaTime)override;
	void OnImGuiRender()override;
	void OnRender() override;
	void Quit()override;
};