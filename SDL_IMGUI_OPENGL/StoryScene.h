#pragma once
#include "Scene.h"


class StoryScene : public Scene
{
private:
   

public:
    StoryScene(SceneName sceneName);

    void Enter() override;
    void OnEvent(SDL_Event& event) override;
    void OnUpdate(float deltaTime) override;
    void OnImGuiRender() override;
    void OnRender() override;
    void Quit() override;
};