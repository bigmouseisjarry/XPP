#pragma once
#include <unordered_map>
#include <iostream>
#include <string>
#include "GameSettings.h"
#include "Scene.h"
#include "BattleScene.h"

class SceneManager
{
private:
	std::unordered_map<SceneName, std::unique_ptr<Scene>> m_Scenes;
	Scene* m_CurrentScene = nullptr;
public:

	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;
	static SceneManager* Get();

	void RegisterScene(SceneName sceneName, std::unique_ptr<Scene> Scene);
	void SceneChange(SceneName targetScene);
	void Init();

	void BeginFrame();
	void OnEvent(SDL_Event& event);
	void OnUpdate(float deltaTime);
	void EndFrame();
	void OnRender()const;
	void OnImGuiRender();

private:
	SceneManager() {}
	~SceneManager() {}

};