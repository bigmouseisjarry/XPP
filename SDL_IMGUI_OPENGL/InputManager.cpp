#include "InputManager.h"
#include <imgui.h>
#include <algorithm>
#include<iostream>

namespace __Internal
{

    void InputManager::Initialize()
    {
        m_Settings.ResetToDefault();
    }

    void InputManager::Shutdown()
    {

    }

    void InputManager::BeginFrame()
    {
        // 1. 重置 Pressed 和 Released（它们只持续一帧！）
        m_KeyPressed.fill(false);
        m_KeyReleased.fill(false);
        m_MousePressed.fill(false);
        m_MouseReleased.fill(false);

        // 3. 保存上一帧的 Down 状态（关键！用于计算 Pressed/Released）
        m_PrevKeyDown = m_KeyDown;
        m_PrevMouseDown = m_MouseDown;

        // 4. 重置鼠标增量（每帧重新计算）
        m_Mouse.deltaX = 0.0f;
        m_Mouse.deltaY = 0.0f;

        m_Mouse.wheelDelta = 0.0f;
        m_Mouse.wheelUpPressed = false;
        m_Mouse.wheelDownPressed = false;
    }

    void InputManager::ProcessEvent(const SDL_Event& event)
    {

        // 键盘按键
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            SDL_Scancode sc = event.key.scancode;
            if (sc >= 0 && sc < SDL_SCANCODE_COUNT)
            {
                m_KeyDown[sc] = true; // 只要按下就设为 true，不管重复
            }
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            SDL_Scancode sc = event.key.scancode;
            if (sc >= 0 && sc < SDL_SCANCODE_COUNT)
            {
                m_KeyDown[sc] = false; // 松开设为 false
            }
        }
        // 鼠标按键
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            Uint8 btn = event.button.button;
            if (btn >= 1 && btn <= 8)
            {
                m_MouseDown[btn - 1] = true; // SDL_BUTTON_LEFT 是 1，数组从 0 开始
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            Uint8 btn = event.button.button;
            if (btn >= 1 && btn <= 8)
            {
                m_MouseDown[btn - 1] = false;
            }
        }
        // 鼠标移动
        else if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            // 更新位置
            m_Mouse.x = event.motion.x;
            m_Mouse.y = event.motion.y;

            // 计算增量（应用灵敏度和反转）
            float dx = event.motion.xrel;
            float dy = event.motion.yrel;

            if (m_Settings.m_Mouse.invertX) dx = -dx;
            if (m_Settings.m_Mouse.invertY) dy = -dy;

            m_Mouse.deltaX += dx * m_Settings.m_Mouse.sensitivity;
            m_Mouse.deltaY += dy * m_Settings.m_Mouse.sensitivity;

        }

		// 鼠标滚轮
        else if (event.type == SDL_EVENT_MOUSE_WHEEL)
        {
            float y = static_cast<float>(event.wheel.y);
            if (m_Settings.m_Mouse.invertWheel) y = -y;
            m_Mouse.wheelDelta += y * m_Settings.m_Mouse.wheelSensitivity;
            if (m_Mouse.wheelDelta > 0.0f)
                m_Mouse.wheelUpPressed = true;
            else if (m_Mouse.wheelDelta < 0.0f)
                m_Mouse.wheelDownPressed = true;

        }

    }

    void InputManager::EndFrame()
    {
        // Pressed = 当前 Down 且 上一帧 !Down（刚按下）
        // Released = 当前 !Down 且 上一帧 Down（刚松开）

        // 1. 键盘
        for (int i = 0; i < SDL_SCANCODE_COUNT; ++i)
        {
            m_KeyPressed[i] = m_KeyDown[i] && !m_PrevKeyDown[i];
            m_KeyReleased[i] = !m_KeyDown[i] && m_PrevKeyDown[i];
        }

        // 2. 鼠标按钮
        for (int i = 0; i < 8; ++i)
        {
            m_MousePressed[i] = m_MouseDown[i] && !m_PrevMouseDown[i];
            m_MouseReleased[i] = !m_MouseDown[i] && m_PrevMouseDown[i];
        }
    }

    // 切换上下文
    void InputManager::SetContext(InputContext ctx)
    {
        m_CurrentContext = ctx;
    }

    InputContext InputManager::GetContext() const
    {
        return m_CurrentContext;
    }


    bool InputManager::IsActionDown(InputAction action) const
    {
        // 1. 获取当前上下文的绑定
        auto ctxIt = m_Settings.m_Bindings.find(m_CurrentContext);
        if (ctxIt == m_Settings.m_Bindings.end())
            return false;

        // 2. 查找动作的绑定
        auto& actionMap = ctxIt->second;
        auto actIt = actionMap.find(action);
        if (actIt == actionMap.end())
            return false;

        // 3. 遍历所有绑定，只要有一个匹配就返回 true
        for (const auto& binding : actIt->second)
        {
            if (__MatchBindingDown(binding))
                return true;
        }

        return false;
    }

    bool InputManager::IsActionPressed(InputAction action) const
    {
        auto ctxIt = m_Settings.m_Bindings.find(m_CurrentContext);
        if (ctxIt == m_Settings.m_Bindings.end())
            return false;

        auto& actionMap = ctxIt->second;
        auto actIt = actionMap.find(action);
        if (actIt == actionMap.end())
            return false;

        for (const auto& binding : actIt->second)
        {
            if (__MatchBindingPressed(binding))
                return true;
        }

        return false;
    }

    bool InputManager::IsActionReleased(InputAction action) const
    {
        auto ctxIt = m_Settings.m_Bindings.find(m_CurrentContext);
        if (ctxIt == m_Settings.m_Bindings.end())
            return false;

        auto& actionMap = ctxIt->second;
        auto actIt = actionMap.find(action);
        if (actIt == actionMap.end())
            return false;

        for (const auto& binding : actIt->second)
        {
            if (__MatchBindingReleased(binding))
                return true;
        }

        return false;
    }

    // 底层输入查询
    bool InputManager::IsKeyDown(SDL_Scancode scancode) const
    {
        if (scancode < 0 || scancode >= SDL_SCANCODE_COUNT)
            return false;
        return m_KeyDown[scancode];
    }

    bool InputManager::IsKeyPressed(SDL_Scancode scancode) const
    {
        if (scancode < 0 || scancode >= SDL_SCANCODE_COUNT)
            return false;
        return m_KeyPressed[scancode];
    }

    bool InputManager::IsKeyReleased(SDL_Scancode scancode) const
    {
        if (scancode < 0 || scancode >= SDL_SCANCODE_COUNT)
            return false;
        return m_KeyReleased[scancode];
    }

    bool InputManager::IsMouseButtonDown(uint8_t button) const
    {
        if (button < 1 || button > 8)
            return false;
        return m_MouseDown[button - 1];
    }

    bool InputManager::IsMouseButtonPressed(uint8_t button) const
    {
        if (button < 1 || button > 8)
            return false;
        return m_MousePressed[button - 1];
    }

    bool InputManager::IsMouseButtonReleased(uint8_t button) const
    {
        if (button < 1 || button > 8)
            return false;
        return m_MouseReleased[button - 1];
    }

    const MouseState& InputManager::GetMouseState() const
    {
        return m_Mouse;
    }

    // ImGui 输入键盘冲突处理
    // ture代表有效
    bool InputManager::IsGameKeyboardInputEnabled() const
    {
        return !ImGui::GetIO().WantCaptureKeyboard;
    }
    // ImGui 输入鼠标冲突处理
    // ture代表有效
    bool InputManager::IsGameMouseInputEnabled() const
    {
        return !ImGui::GetIO().WantCaptureMouse;
    }

    //  绑定匹配逻辑
    bool InputManager::__MatchBindingDown(const InputBinding& binding) const
    {
        return std::visit([this](auto&& arg) -> bool {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, KeyboardBinding>)
            {
                return IsKeyDown(arg.scancode);
            }
            else if constexpr (std::is_same_v<T, MouseButtonBinding>)
            {
                return IsMouseButtonDown(arg.button);
            }
            else if constexpr (std::is_same_v<T, MouseWheelBinding>)
            {
                // 滚轮没有"持续按住"概念，使用Pressed语义
                if (arg.direction == 0)
                    return m_Mouse.wheelUpPressed || m_Mouse.wheelDownPressed;
                else if (arg.direction > 0)
                    return m_Mouse.wheelUpPressed;
                else
                    return m_Mouse.wheelDownPressed;
            }
            return false;
            }, binding);
    }

    bool InputManager::__MatchBindingPressed(const InputBinding& binding) const
    {
        return std::visit([this](auto&& arg) -> bool {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, KeyboardBinding>)
            {
                return IsKeyPressed(arg.scancode);
            }
            else if constexpr (std::is_same_v<T, MouseButtonBinding>)
            {
                return IsMouseButtonPressed(arg.button);
            }
            else if constexpr (std::is_same_v<T, MouseWheelBinding>)
            {
                if (arg.direction == 0)
                    return m_Mouse.wheelUpPressed || m_Mouse.wheelDownPressed;
                else if (arg.direction > 0)
                    return m_Mouse.wheelUpPressed;
                else
                    return m_Mouse.wheelDownPressed;
            }
            return false;
            }, binding);
    }

    bool InputManager::__MatchBindingReleased(const InputBinding& binding) const
    {
        return std::visit([this](auto&& arg) -> bool {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, KeyboardBinding>)
            {
                return IsKeyReleased(arg.scancode);
            }
            else if constexpr (std::is_same_v<T, MouseButtonBinding>)
            {
                return IsMouseButtonReleased(arg.button);
            }
            else if constexpr (std::is_same_v<T, MouseWheelBinding>)
            {
                // 滚轮没有"释放"概念，返回false或与Pressed相同                                                           
                return false;
            }
            return false;
            }, binding);
    }

}