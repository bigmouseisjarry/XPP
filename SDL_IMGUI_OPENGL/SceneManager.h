#pragma once
#include <unordered_map>
#include <iostream>
#include <string>
#include "GameSettings.h"
#include "Scene.h"

class SceneManager
{
private:
	std::unordered_map<SceneID, std::unique_ptr<Scene>> m_Scenes;
	Scene* m_CurrentScene = nullptr;
public:

	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;
	static SceneManager* Get();

	void RegisterScene(SceneID sceneID, std::unique_ptr<Scene> Scene);
	void SceneChange(SceneID targetScene);
	void Init();

	void BeginFrame();
	void OnEvent(SDL_Event& event);
	void OnUpdate(float deltaTime);
	void EndFrame();
	void OnRender()const;
	void OnImGuiRender();

	entt::registry& GetCurrentRegistry();

private:
	SceneManager() {}
	~SceneManager() {}

};