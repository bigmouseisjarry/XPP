#include "SettingsUI.h"
#include "imgui.h"
#include<iostream>

namespace __Internal
{

    void SettingsUI::OnUpdate(float dt)
    {
        if (m_TipTimer > 0)
            m_TipTimer -= dt;

        if (m_Rebinding.has_value())
        {
            m_RebindTimeout -= dt;
            if (m_RebindTimeout <= 0)
            {
                m_Rebinding = std::nullopt;
                ShowTip("重绑定已取消");
            }
        }
    }

    static const std::unordered_map<InputAction, std::string> g_ActionDisplayName = {
    { InputAction::MoveForward,          "前进" },
    { InputAction::MoveBackward,         "后退" },
    { InputAction::MoveIn,               "向前进入另一层" },
    { InputAction::MoveOut,              "向后进入另一层" },
    { InputAction::Jump,                 "跳跃" },
    { InputAction::Attack,               "攻击" },
    { InputAction::CameraZoomIn,         "视角前移" },
    { InputAction::CameraZoomOut,        "视角后移" },
    { InputAction::CameraPan,            "视角平移" }
    };

    void SettingsUI::OnImGuiRender(InputManager& input)
    {
        if (!m_Visible)
            return;

        ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("输入设置", &m_Visible))
        {
            ImGui::End();
            return;
        }

        if (!m_Visible)
            input.SetContext(InputContext::Gameplay);

        // 提示文字
        if (m_TipTimer > 0)
        {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s", m_TipText.c_str());
            ImGui::Separator();
        }

        DrawContextSelector(input);
        ImGui::Separator();

        DrawBindingsTable(input);
        ImGui::Separator();

        DrawMouseSettings(input);
        ImGui::Separator();

        DrawBottomButtons(input);

        ImGui::End();
    }


    void SettingsUI::DrawContextSelector(InputManager& input)
    {
        const char* items[] = { "游戏中", "仅UI", "暂停" };
        int current = (int)m_SelectedContext;

        if (ImGui::Combo("当前场景", &current, items, 3))
        {
            m_SelectedContext = (InputContext)current;
        }
    }


    void SettingsUI::DrawMouseSettings(InputManager& input)
    {
        auto& settings = input.GetSettings();

        ImGui::Text("鼠标设置");
        ImGui::SliderFloat("灵敏度", &settings.m_Mouse.sensitivity, 0.1f, 3.0f);
        ImGui::SliderFloat("滚轮灵敏度", &settings.m_Mouse.wheelSensitivity, 0.1f, 3.0f);
        ImGui::Checkbox("反转滚轮", &settings.m_Mouse.invertWheel);
    }

    
    void SettingsUI::DrawBottomButtons(InputManager& input)
    {
        auto& settings = input.GetSettings();

        float w = (ImGui::GetContentRegionAvail().x - 20) / 3;

        if (ImGui::Button("保存设置", ImVec2(w, 0)))
        {
            if (settings.SaveToFile(""))
                ShowTip("保存成功！");
            else
                ShowTip("保存失败！");
        }

        ImGui::SameLine();
        if (ImGui::Button("加载设置", ImVec2(w, 0)))
        {
            if (settings.LoadFromFile(""))
                ShowTip("加载成功！");
            else
                ShowTip("加载失败！");
        }

        ImGui::SameLine();
        if (ImGui::Button("重置默认", ImVec2(w, 0)))
        {
            settings.ResetToDefault();
            ShowTip("已重置！");
        }

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    }

    void SettingsUI::DrawBindingsTable(InputManager& input)
    {
        auto& settings = input.GetSettings();
        auto& ctxBindings = settings.m_Bindings[m_SelectedContext];

        ImGui::Text("点击【修改】开始重绑定，按 ESC 取消");
        ImGui::Spacing();

        if (ImGui::BeginTable("Bindings", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("动作", ImGuiTableColumnFlags_WidthFixed, 120);
            ImGui::TableSetupColumn("绑定", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableHeadersRow();

            for (const auto& [action, name] : g_ActionDisplayName)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", name.c_str());

                ImGui::TableNextColumn();

                // 显示当前绑定
                auto it = ctxBindings.find(action);
                bool isBindingThis = m_Rebinding.has_value() &&
                    m_Rebinding->first == m_SelectedContext &&
                    m_Rebinding->second == action;

                if (isBindingThis)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), ">>> 请按下新按键 <<<");
                }
                else if (it != ctxBindings.end() && !it->second.empty())
                {
                    std::string bindStr = BindingToString(it->second[0]);
                    ImGui::Text("%s", bindStr.c_str());
                }
                else
                {
                    ImGui::TextDisabled("未绑定");
                }

                ImGui::TableNextColumn();
                std::string btnLabel = "修改##" + std::to_string((int)action);
                if (ImGui::Button(btnLabel.c_str(), ImVec2(-1, 0)))
                {
                    m_Rebinding = { m_SelectedContext, action };
                    m_RebindTimeout = 5.0f;
                }
            }

            ImGui::EndTable();
        }
    }


    void SettingsUI::ToggleVisible()
    {
        if (m_Visible)
        {
            m_RebindTimeout = 0;
        }
        m_Visible = !m_Visible;
    }

    bool SettingsUI::OnEvent(const SDL_Event& e, InputManager& input)
    {
        if (!m_Rebinding.has_value())
            return false;

        auto [ctx, action] = m_Rebinding.value();
        auto& settings = input.GetSettings();

        // 按 ESC 取消
        if (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_ESCAPE)
        {
            m_Rebinding = std::nullopt;
            ShowTip("已取消");
            return true;
        }

        // 捕获键盘
        if (e.type == SDL_EVENT_KEY_DOWN)
        {
            settings.m_Bindings[ctx][action].clear();
            settings.m_Bindings[ctx][action].push_back(KeyboardBinding{ e.key.scancode });
            m_Rebinding = std::nullopt;
            ShowTip("绑定成功！");
            return true;
        }

        // 捕获鼠标按钮
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            settings.m_Bindings[ctx][action].clear();
            settings.m_Bindings[ctx][action].push_back(MouseButtonBinding{ e.button.button });
            m_Rebinding = std::nullopt;
            ShowTip("绑定成功！");
            return true;
        }

		// 捕获鼠标滚轮
        if (e.type == SDL_EVENT_MOUSE_WHEEL)
        {
            settings.m_Bindings[ctx][action].clear();
            // 仅处理垂直滚轮                                                                                         
            int dir = (e.wheel.y > 0) ? 1 : -1;
            settings.m_Bindings[ctx][action].push_back(MouseWheelBinding{ dir });
            m_Rebinding = std::nullopt;
            ShowTip("绑定成功！");
            return true;
        }

        return false;
    }

    void SettingsUI::ShowTip(const std::string& text, float duration)
    {
        m_TipText = text;
        m_TipTimer = duration;
    }

    std::string SettingsUI::BindingToString(const InputBinding& binding)
    {
        return std::visit([](auto&& arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;

            // 1. 键盘按键
            if constexpr (std::is_same_v<T, KeyboardBinding>)
            {
                const char* name = SDL_GetScancodeName(arg.scancode);
                if (name && strlen(name) > 0)
                    return std::string("键盘: ") + name;
                return "键盘: 未知";
            }

            // 2. 鼠标按钮
            else if constexpr (std::is_same_v<T, MouseButtonBinding>)
            {
                switch (arg.button)
                {
                case SDL_BUTTON_LEFT:   return "鼠标: 左键";
                case SDL_BUTTON_RIGHT:  return "鼠标: 右键";
                case SDL_BUTTON_MIDDLE: return "鼠标: 中键";
                case SDL_BUTTON_X1:     return "鼠标: 侧键1";
                case SDL_BUTTON_X2:     return "鼠标: 侧键2";
                default:                return "鼠标: 按钮" + std::to_string(arg.button);
                }
            }

			// 3. 鼠标滚轮
            else if constexpr (std::is_same_v<T, MouseWheelBinding>)
            {
                if (arg.direction > 0) return "滚轮: 上滚";
                if (arg.direction < 0) return "滚轮: 下滚";
                return "滚轮: 任意滚动";
            }

            //// 3. 鼠标滚轮
            //else if constexpr (std::is_same_v<T, MouseWheelBinding>)
            //{
            //    if (arg.isHorizontal)
            //    {
            //        return arg.direction > 0 ? "滚轮: 右滚" : "滚轮: 左滚";
            //    }
            //    else
            //    {
            //        return arg.direction > 0 ? "滚轮: 上滚" : "滚轮: 下滚";
            //    }
            //}

            return "未知绑定";
            }, binding);
    }


    std::string SettingsUI::ActionToString(InputAction action)
    {
        auto it = g_ActionDisplayName.find(action);
        if (it != g_ActionDisplayName.end())
            return it->second;
        return "未知动作";
    }

}