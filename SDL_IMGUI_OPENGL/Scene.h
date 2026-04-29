#pragma once
#include <entt.hpp>
#include <SDL3/SDL.h>

enum class SceneName
{
    Battle,  // 战斗场景
    Story    // 剧情场景
};

class Scene
{
protected:
    entt::registry m_Registry;
    SceneName m_name;

public:
    Scene(SceneName sceneName) :m_name(sceneName) {};
    virtual ~Scene() = default;

    entt::registry& GetRegistry() { return m_Registry; }
    const SceneName& GetName() const { return m_name; }

    // 纯虚函数或提供默认实现
    virtual void Enter() = 0;
    virtual void OnEvent(SDL_Event& event)  = 0;
    virtual void OnUpdate(float deltaTime)  = 0;
    virtual void OnImGuiRender() = 0;
    virtual void OnRender() = 0;
    virtual void Quit() { m_Registry.clear(); };
};