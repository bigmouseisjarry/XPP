#pragma once
#include <SDL3/SDL.h>
#include "InputManager.h"
#include <optional>

namespace __Internal
{
    class SettingsUI
    {
    public:
        void SetVisible(bool v) { m_Visible = v; }
        bool IsVisible() const { return m_Visible; }
        void ToggleVisible();                                                   // 翻转ui显示值

        bool OnEvent(const SDL_Event& e, InputManager& input);                  // 捕获重绑定的按键，返回值true表示输入被当作绑定新按键
        void OnUpdate(float dt);
        void OnImGuiRender(InputManager& input);

    private:

        bool m_Visible = false;                                                 // 面板是否显示（默认隐藏）
        InputContext m_SelectedContext = InputContext::Gameplay;                // 当前选中的按键场景
        std::optional<std::pair<InputContext, InputAction>> m_Rebinding;        // 正在改哪个键


        float m_RebindTimeout = 0.0f;    // 改键超时计时器
        std::string m_TipText;           // 提示文字（如“请按下新按键”）
        float m_TipTimer = 0.0f;         // 提示显示时间


        void DrawContextSelector(InputManager& input);                              // 画：场景选择（游戏/菜单）
        void DrawBindingsTable(InputManager& input);                                // 画：按键列表 + 改键按钮
        void DrawMouseSettings(InputManager& input);                                // 画：鼠标灵敏度设置
        void DrawBottomButtons(InputManager& input);                                // 画：底部按钮（重置/应用）
        void ShowTip(const std::string& text, float duration = 3.0f);               // 显示提示
        static std::string BindingToString(const InputBinding& binding);            // 按键→文字
        static std::string ActionToString(InputAction action);                      // 动作→文字
    };
}