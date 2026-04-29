#include "SceneManager.h"
#include "StoryScene.h"

SceneManager* SceneManager::Get()
{
	static SceneManager instance;
	return &instance;
}

void SceneManager::RegisterScene(SceneName sceneName, std::unique_ptr<Scene> Scene)
{
	m_Scenes[sceneName] = std::move(Scene);
}

void SceneManager::SceneChange(SceneName targetScene)
{
	if (m_Scenes.find(targetScene) != m_Scenes.end())
	{
		if (m_CurrentScene != nullptr)
			m_CurrentScene->Quit();
		m_CurrentScene = m_Scenes[targetScene].get();
		m_CurrentScene->Enter();
	}
	else
	{
		std::cout << "目标场景不存在 :" << static_cast<int>(targetScene) << std::endl;
	}
}

void SceneManager::Init()
{
	RegisterScene(SceneName::Battle, std::make_unique<BattleScene>(SceneName::Battle));
	RegisterScene(SceneName::Story, std::make_unique<StoryScene>(SceneName::Story));
	SceneChange(SceneName::Story);
}

void SceneManager::BeginFrame()
{
	GameSettings::Get()->BeginFrame();
}

void SceneManager::OnEvent(SDL_Event& event)
{
	GameSettings::Get()->OnEvent(event);

	if (m_CurrentScene == nullptr)
		std::cout << "当前场景指针为空" << std::endl;
	else
		m_CurrentScene->OnEvent(event);

	if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
	{
		int w = event.window.data1;
		int h = event.window.data2;
		Renderer::Get()->SetWindowSize(w, h);
	}
}

void SceneManager::OnUpdate(float deltaTime)
{
	GameSettings::Get()->OnUpdate(deltaTime);

	if (m_CurrentScene == nullptr)
		std::cout << "当前场景指针为空" << std::endl;
	else
		m_CurrentScene->OnUpdate(deltaTime);
	
}

void SceneManager::EndFrame()
{
	GameSettings::Get()->EndFrame();
}

void SceneManager::OnRender() const
{
	if (m_CurrentScene == nullptr)
		std::cout << "当前场景指针为空" << std::endl;
	else
		m_CurrentScene->OnRender();

}

void SceneManager::OnImGuiRender()
{
	if (m_CurrentScene == nullptr)
		std::cout << "当前场景指针为空" << std::endl;
	else
		m_CurrentScene->OnImGuiRender();

	GameSettings::Get()->OnImGuiRender();
}
