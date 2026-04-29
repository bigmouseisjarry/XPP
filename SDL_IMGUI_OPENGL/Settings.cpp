#include"Settings.h"

void __Internal::InputSettings::ResetToDefault()
{
	m_Mouse.ResetToDefault();

	__ResetGameplayToDefault();
	__ResetUIOnlyToDefault();
	__ResetPausedToDefault();
}

void __Internal::InputSettings::__ResetGameplayToDefault()
{
    auto& gameplayMap = m_Bindings[InputContext::Gameplay];
    gameplayMap.clear();

    gameplayMap[InputAction::MoveForward] = { KeyboardBinding{SDL_SCANCODE_W} };
    gameplayMap[InputAction::MoveBackward] = { KeyboardBinding{SDL_SCANCODE_S} };
    gameplayMap[InputAction::MoveIn] = { KeyboardBinding{SDL_SCANCODE_A} };
    gameplayMap[InputAction::MoveOut] = { KeyboardBinding{SDL_SCANCODE_D} };
    gameplayMap[InputAction::Jump] = { KeyboardBinding{SDL_SCANCODE_SPACE} };
    gameplayMap[InputAction::Attack] = { KeyboardBinding{SDL_SCANCODE_J} };
    gameplayMap[InputAction::OpenSettings] = { KeyboardBinding{SDL_SCANCODE_TAB} };
    gameplayMap[InputAction::CameraZoomIn].push_back(MouseWheelBinding{ 1 });                       // 滚轮上滚                           
    gameplayMap[InputAction::CameraZoomOut].push_back(MouseWheelBinding{ -1 });                     // 滚轮下滚                           
	gameplayMap[InputAction::CameraPan].push_back(MouseButtonBinding{ SDL_BUTTON_MIDDLE });         // 鼠标中键按住                 
    gameplayMap[InputAction::CameraOrbitFocus].push_back(MouseButtonBinding{ SDL_BUTTON_RIGHT });   // 鼠标右键按住                
}

void __Internal::InputSettings::__ResetUIOnlyToDefault()
{
    auto& uiMap = m_Bindings[InputContext::UIOnly];
    uiMap.clear();

    uiMap[InputAction::OpenSettings] = { KeyboardBinding{SDL_SCANCODE_TAB} };
    //uiMap[InputAction::Confirm] = {
    //    KeyboardBinding{SDL_SCANCODE_RETURN},
    //    MouseButtonBinding{SDL_BUTTON_LEFT}
    //}; // 回车或鼠标左键确认
    //uiMap[InputAction::Cancel] = { KeyboardBinding{SDL_SCANCODE_ESCAPE} }; // ESC 取消
}

void __Internal::InputSettings::__ResetPausedToDefault()
{
    // 1. 清空或创建 Paused 上下文的绑定
    auto& pausedMap = m_Bindings[InputContext::Paused];
    pausedMap.clear();

    //// 2. 暂停菜单绑定（只响应取消暂停/确认/取消）
    //pausedMap[InputAction::Pause] = { KeyboardBinding{SDL_SCANCODE_ESCAPE} }; // ESC 取消暂停
    //pausedMap[InputAction::Confirm] = { MouseButtonBinding{SDL_BUTTON_LEFT} }; // 鼠标左键点击菜单
    //pausedMap[InputAction::Cancel] = { KeyboardBinding{SDL_SCANCODE_ESCAPE} }; // ESC 关闭菜单
}

bool __Internal::InputSettings::LoadFromFile(const std::string& path)
{
	return true;
}

bool __Internal::InputSettings::SaveToFile(const std::string& path) const
{
	return true;
}
