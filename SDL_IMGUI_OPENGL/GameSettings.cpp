#include "GameSettings.h"

GameSettings* GameSettings::Get() {
	static GameSettings instance;
	return &instance;
}

GameSettings::GameSettings() : m_Impl(std::make_unique<Impl>()) {}


struct GameSettings::Impl
{
    __Internal::InputManager m_Input;
    __Internal::SettingsUI m_UI;
};


void GameSettings::Init()
{
    m_Impl->m_Input.Initialize();
}

void GameSettings::Shutdown()
{
    if (m_Impl)
    {
        m_Impl->m_Input.Shutdown();
    }
}

void GameSettings::BeginFrame()
{
    m_Impl->m_Input.BeginFrame();
}

void GameSettings::OnEvent(SDL_Event& event)
{
    if (!m_Impl->m_UI.OnEvent(event, m_Impl->m_Input))
    {
        m_Impl->m_Input.ProcessEvent(event);
    }
}

void GameSettings::EndFrame()
{
    m_Impl->m_Input.EndFrame();
}

void GameSettings::OnUpdate(float deltaTime)
{
    // 1. 更新 UI（处理提示文字、重绑定超时）
    m_Impl->m_UI.OnUpdate(deltaTime);
}

void GameSettings::OnImGuiRender()
{
    // 直接转发给 UI 绘制
    m_Impl->m_UI.OnImGuiRender(m_Impl->m_Input);
}

void GameSettings::SetContext(InputContext ctx)
{
    m_Impl->m_Input.SetContext(ctx);
}

InputContext GameSettings::GetContext() const
{
    return m_Impl->m_Input.GetContext();
}

bool GameSettings::IsActionDown(InputAction action) const
{
    return m_Impl->m_Input.IsActionDown(action);
}

bool GameSettings::IsActionPressed(InputAction action) const
{
    return m_Impl->m_Input.IsActionPressed(action);
}

bool GameSettings::IsActionReleased(InputAction action) const
{
    return m_Impl->m_Input.IsActionReleased(action);
}

const MouseState& GameSettings::GetMouseState() const
{
    return m_Impl->m_Input.GetMouseState();
}

void GameSettings::SetUIVisible(bool visible)
{
    m_Impl->m_UI.SetVisible(visible);
}

bool GameSettings::GetUIVisible() const
{
    return m_Impl->m_UI.IsVisible();
}

void GameSettings::ToggleUIVisible()
{
    if (GetUIVisible())
        SetContext(InputContext::Gameplay);
    else
        SetContext(InputContext::UIOnly);

    m_Impl->m_UI.ToggleVisible();
}

bool GameSettings::LoadFromFile(const std::string& path)
{
    return m_Impl->m_Input.GetSettings().LoadFromFile(path);
}

bool GameSettings::SaveToFile(const std::string& path) const
{
    return m_Impl->m_Input.GetSettings().SaveToFile(path);
}

void GameSettings::ResetAllToDefault()
{
    m_Impl->m_Input.GetSettings().ResetToDefault();
}
