#pragma once

#include <SDL3/SDL.h>
#include <unordered_map>
#include <string>
#include <array>
#include <imgui.h>
#include "InputStruct.h"
#include <memory>
#include"InputManager.h"
#include"SettingsUI.h"

class GameSettings
{
private:

public:
	GameSettings(const GameSettings&) = delete;
	GameSettings& operator=(const GameSettings&) = delete;

	static GameSettings* Get();


	void Init();
	void Shutdown();
	void BeginFrame();						// 每帧开头调用1次
	void OnEvent(SDL_Event& event);			// 每个SDL事件调用1次
	void EndFrame();						// 每帧结尾调用1次
	void OnUpdate(float deltaTime);			// 每帧更新UI逻辑
	void OnImGuiRender();					// 每帧渲染UI

	void SetContext(InputContext ctx);
	InputContext GetContext() const;

	bool IsActionDown(InputAction action) const;
	bool IsActionPressed(InputAction action) const;
	bool IsActionReleased(InputAction action) const;
	const MouseState& GetMouseState() const;

	void SetUIVisible(bool visible);
	bool GetUIVisible() const;
	void ToggleUIVisible();

	bool LoadFromFile(const std::string& path = "");
	bool SaveToFile(const std::string& path = "") const;
	void ResetAllToDefault();

private:
	GameSettings();
	~GameSettings() = default;

	struct Impl; 
	std::unique_ptr<Impl> m_Impl;
};