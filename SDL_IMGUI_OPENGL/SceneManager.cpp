#include "SceneManager.h"
#include "Scene2D.h"
#include "Scene3D.h"
#include "Renderer.h"

SceneManager* SceneManager::Get()
{
	static SceneManager instance;
	return &instance;
}

void SceneManager::RegisterScene(SceneID sceneID, std::unique_ptr<Scene> Scene)
{
	m_Scenes[sceneID] = std::move(Scene);
}

void SceneManager::SceneChange(SceneID targetScene)
{
	if (m_Scenes.find(targetScene) != m_Scenes.end())
	{
		if (m_CurrentScene != nullptr)
			m_CurrentScene->Quit();

		// 重置 Renderer 中上一场景的管线状态
		Renderer::Get()->SetShadowArrayFBOID({ INVALID_ID });
		Renderer::Get()->SetSkyboxTexID({ INVALID_ID });
		Renderer::Get()->SetSSAOKernelUBO(nullptr);

		// 创建空管线，让场景填充
		auto pipeline = std::make_unique<RenderPipeline>();
		m_CurrentScene = m_Scenes[targetScene].get();
		m_CurrentScene->Enter();                         // 先初始化场景实体
		m_CurrentScene->BuildPipeline(*pipeline);        // 再构建管线
		Renderer::Get()->SetPipeline(std::move(pipeline));
	}
	else
	{
		std::cout << "目标场景不存在 :" << static_cast<int>(targetScene) << std::endl;
	}
}

void SceneManager::Init()
{
	RegisterScene(SceneID::Mode2D, std::make_unique<Scene2D>(SceneID::Mode2D));
	RegisterScene(SceneID::Mode3D, std::make_unique<Scene3D>(SceneID::Mode3D));
	SceneChange(SceneID::Mode3D);
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

entt::registry& SceneManager::GetCurrentRegistry()
{
	return m_CurrentScene->GetRegistry();
}
