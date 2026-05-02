#include <GL/glew.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "SceneManager.h"
#include "ResourceManager.h"
#include "GameSettings.h"
#include "DebugRenderSystem.h"
#include<Windows.h>
#include <entt.hpp>
#include "WorkerPool.h"
#include "EntityFactory.h"

static void Init()
{
    SetConsoleOutputCP(65001);
    WorkerPool::Get()->Init(2);
    ResourceManager::Get()->Init();
    SceneManager::Get()->Init();
    GameSettings::Get()->Init();
	Renderer::Get()->InitUBO();
	DebugRenderSystem::Init();
}

int main(int, char**)
{
    // 初始化 SDL 库
    // 窗口|手柄|声音
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // 开启双缓冲
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    // 深度缓冲（3D 用）
    // SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    // 模板缓冲（特效 / 遮罩用）
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    // 获取屏幕缩放（高分屏适配）
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    // 用 OpenGL 绘图|可调整窗口大小|先隐藏，后面再显示|高分屏清晰
    SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    // 创建窗口("窗口标题", 宽, 高, 标记);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+OpenGL3 example", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }
    // 创建 OpenGL 上下文
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return 1;
    }

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        printf("GLEW 失败");
        return 1;
    }

    // 绑定OpenGL上下文到窗口
    SDL_GL_MakeCurrent(window, gl_context);
    // 开启垂直同步
    SDL_GL_SetSwapInterval(1); 
    // 窗口位置居中
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    // 显示窗口
    SDL_ShowWindow(window);

    // 检查 ImGui 版本是否正确
    IMGUI_CHECKVERSION();
    // 创建 ImGui 的全局上下文
    ImGui::CreateContext();
    // 拿到输入管理器
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;           // 允许键盘操作UI
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;            // 允许手柄操作UI

    const char* fontPath = "C:/Windows/Fonts/msyh.ttc"; // 注意是 .ttc（TrueType Collection），不是 .ttf

    // 第 3 个参数是字体大小，第 4 个参数是字符范围（这里用默认的中文范围）
    ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

    // 如果加载失败（比如找不到字体文件），就用回默认字体
    if (!font)
    {
        io.Fonts->AddFontDefault();
    }

    // 设置 ImGui 背景为黑色
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // 拿到 ImGui 的样式对象（按钮大小、圆角、间距、边框…）
    ImGuiStyle& style = ImGui::GetStyle();
    // 根据屏幕缩放，自动放大所有 UI 元素
    style.ScaleAllSizes(main_scale);       
    // 设置字体的缩放
    style.FontScaleDpi = main_scale;       

    // 把 ImGui 绑定到 SDL3 窗口 + OpenGL 显卡
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    // 使用 OpenGL 来绘制 UI
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    Init();

    // 初始化 Renderer 视口
    {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        Renderer::Get()->SetWindowSize(w, h);
    }

    // Main loop
    bool done = false;

    Uint64 lastTime = SDL_GetTicks();
    while (!done)
    {
        // TODO:我的开始帧逻辑执行
        SceneManager::Get()->BeginFrame();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            // TODO:我的事件
            SceneManager::Get()->OnEvent(event);

            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        // 最小化窗口暂停游戏
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        Uint64 now = SDL_GetTicks();
        float deltaTime = (now - lastTime) / 1000.0f;
        lastTime = now;
        //TODO:我的结束帧逻辑执行
        SceneManager::Get()->EndFrame();

        EntityFactory::ProcessAsyncLoadResults();

        // TODO：我的更新
        SceneManager::Get()->OnUpdate(deltaTime);

        // 每帧必须调用的固定开头，开始新的一帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // TODO: 我的 ImGui 渲染
        SceneManager::Get()->OnImGuiRender();

        // 渲染
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        // 清理屏幕
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TODO：我的渲染
        SceneManager::Get()->OnRender();

        // 画UI
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // 显示画面
        SDL_GL_SwapWindow(window);
    }

    WorkerPool::Get()->Shutdown();
    EntityFactory::ShutdownAsync();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
