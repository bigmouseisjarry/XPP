#pragma once
#include "Scene.h"


class Scene3D : public Scene
{
private:
   

public:
    Scene3D(SceneID sceneID);

    void Enter() override;
    void OnEvent(SDL_Event& event) override;
    void OnUpdate(float deltaTime) override;
    void OnImGuiRender() override;
    void OnRender() override;
    void Quit() override;

    void BuildPipeline(RenderPipeline& pipeline) override;
};