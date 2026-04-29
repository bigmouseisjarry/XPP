#pragma once
#include <SDL3/SDL.h>
#include <variant>

enum class InputAction
{
    MoveForward,
    MoveBackward,
    MoveIn,
    MoveOut,
    Jump,
    Attack,
    OpenSettings,
    CameraZoomIn,           // 新增：视角前移（滚轮上滚）
    CameraZoomOut,          // 新增：视角后移（滚轮下滚）
    CameraPan,              // 新增：视角平移（按住鼠标右键）
    CameraOrbitFocus,       // 新增：视角环绕焦点
    COUNT 
};

enum class InputContext
{
    Gameplay,   // 正常游戏
    UIOnly,     // 只响应UI
    Paused      // 暂停界面
};

enum class BindingDevice
{
    Keyboard,
    MouseButton,
    MouseWheel,
};

// 键盘按键绑定结构体
struct KeyboardBinding
{
    SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;

    bool operator==(const KeyboardBinding& other) const {
        return scancode == other.scancode;
    }
};

// 鼠标状态
struct MouseState
{
	int x = 0;                      // 当前鼠标位置
	int y = 0;                      // 当前鼠标位置
	float deltaX = 0.0f;            // 鼠标移动增量（每帧更新）
	float deltaY = 0.0f;            // 鼠标移动增量（每帧更新）
	float wheelDelta = 0.0f;        // 滚轮增量（正数上滚，负数下滚）
	bool wheelUpPressed = false;    // 滚轮上滚事件
	bool wheelDownPressed = false;  // 滚轮下滚事件
};

// 鼠标按钮绑定结构体
struct MouseButtonBinding
{
    uint8_t button = 0; // SDL_BUTTON_LEFT / SDL_BUTTON_RIGHT / ...

    bool operator==(const MouseButtonBinding& other) const {
        return button == other.button;
    }
};

// 鼠标滚轮绑定（仅垂直方向）
struct MouseWheelBinding
{
    int direction = 0; // 1=上滚, -1=下滚, 0=任意方向

    bool operator==(const MouseWheelBinding& other) const {
        return direction == other.direction;
    }
};

using InputBinding = std::variant<KeyboardBinding,MouseButtonBinding,MouseWheelBinding>;