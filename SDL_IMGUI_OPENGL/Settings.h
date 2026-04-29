#pragma once

#include<string>
#include<unordered_map>

#include "InputStruct.h"

namespace __Internal
{
    struct MouseSettings
    {
		float sensitivity = 1.0f;        // 鼠标灵敏度
        float wheelSensitivity = 1.0f;   // 滚轮灵敏度
        bool invertX = false;            // x轴翻转   
        bool invertY = false;            // y轴翻转
        bool invertWheel = false;        // 滚轮反转
        bool lockInWindow = false;       // 锁定鼠标在窗口内
        void ResetToDefault() { *this = MouseSettings{}; }
    };

    struct InputSettings
    {
        MouseSettings m_Mouse;
        std::unordered_map<InputContext, std::unordered_map<InputAction, std::vector<InputBinding>>> m_Bindings;

        void ResetToDefault();
        void __ResetGameplayToDefault();
        void __ResetUIOnlyToDefault();
        void __ResetPausedToDefault();
        bool LoadFromFile(const std::string& path);
        bool SaveToFile(const std::string& path) const;
    };
}