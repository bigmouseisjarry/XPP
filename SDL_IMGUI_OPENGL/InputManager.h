// InputManager.h
#pragma once

#include "Settings.h"
#include <array>

namespace __Internal
{
    class InputManager
    {
    public:

        InputManager() = default;
        ~InputManager() = default;

        void Initialize();
        void Shutdown();
        void BeginFrame();
        void ProcessEvent(const SDL_Event& event);
        void EndFrame();


        void SetContext(InputContext ctx);
        InputContext GetContext() const;


        InputSettings& GetSettings() { return m_Settings; }
        const InputSettings& GetSettings() const { return m_Settings; }


        bool IsActionDown(InputAction action) const;            // 动作一直按着
        bool IsActionPressed(InputAction action) const;         // 动作刚刚按下
        bool IsActionReleased(InputAction action) const;        // 动作刚刚抬起

        bool IsKeyDown(SDL_Scancode scancode)const;
        bool IsKeyPressed(SDL_Scancode scancode)const;
        bool IsKeyReleased(SDL_Scancode scancode)const;

        bool IsMouseButtonDown(uint8_t button)const;
        bool IsMouseButtonPressed(uint8_t button)const;
        bool IsMouseButtonReleased(uint8_t button)const;

        // 鼠标状态
        const MouseState& GetMouseState() const;


        bool IsGameKeyboardInputEnabled() const;
        bool IsGameMouseInputEnabled() const;

    private:
        
        bool __MatchBindingDown(const InputBinding& binding) const;
        bool __MatchBindingPressed(const InputBinding& binding) const;
        bool __MatchBindingReleased(const InputBinding& binding) const;

    private:
        InputContext m_CurrentContext = InputContext::Gameplay;
        InputSettings m_Settings; // 内部持有设置

        // 输入状态数组
        std::array<bool, SDL_SCANCODE_COUNT> m_KeyDown{};
        std::array<bool, SDL_SCANCODE_COUNT> m_PrevKeyDown{};
        std::array<bool, SDL_SCANCODE_COUNT> m_KeyPressed{};
        std::array<bool, SDL_SCANCODE_COUNT> m_KeyReleased{};

        std::array<bool, 8> m_MouseDown{};
        std::array<bool, 8> m_PrevMouseDown{};
        std::array<bool, 8> m_MousePressed{};
        std::array<bool, 8> m_MouseReleased{};

        MouseState m_Mouse{};
    };

}