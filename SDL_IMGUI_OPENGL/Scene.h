#pragma once
#include <entt.hpp>
#include <SDL3/SDL.h>

enum class SceneID
{
    Mode2D,  // 战斗场景
    Mode3D    // 剧情场景
};

class RenderPipeline;

class Scene
{
protected:
    entt::registry m_Registry;
    SceneID m_ID;

public:
    Scene(SceneID sceneName) :m_ID(sceneName) {};
    virtual ~Scene() = default;

    entt::registry& GetRegistry() { return m_Registry; }
    const SceneID& GetName() const { return m_ID; }

    // 纯虚函数或提供默认实现
    virtual void Enter() = 0;
    virtual void OnEvent(SDL_Event& event)  = 0;
    virtual void OnUpdate(float deltaTime)  = 0;
    virtual void OnImGuiRender() = 0;
    virtual void OnRender() = 0;
    virtual void Quit() { m_Registry.clear(); };

    virtual void BuildPipeline(RenderPipeline& pipeline) = 0;

    static void BuildDefaultPipeline2D(RenderPipeline& pipeline);
    static void BuildDefaultPipeline3D(RenderPipeline& pipeline);
};