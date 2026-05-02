# 多线程改造方案：异步加载 + 渲染解耦

## 总览

| Phase | 目标 | 效果 | 复杂度 |
|-------|------|------|--------|
| Phase 1 | 异步资源加载 | 消除场景加载卡顿 | 中 |
| Phase 2 | 游戏逻辑/渲染双缓冲解耦 | CPU Update 与 GPU 渲染并行 | 中高 |

**核心约束：OpenGL 是单线程的。所有 `gl*` 调用必须留在主线程。CPU 工作可以移走。**

---

## Phase 1：异步资源加载

### 问题

- `ResourceManager::Init()` 同步加载 ~16 个 Shader、~17 个 Texture、~8 个 Mesh，阻塞启动
- `EntityFactory::CreateModel3D()` 同步调用 `ModelLoader::LoadModel()`（glTF JSON解析+顶点处理）+ `ResourceManager::LoadTexture()`（stb_image解码+GL上传），阻塞主线程数百毫秒到数秒
- `EntityFactory::CreateModelHierarchy()` 同上

### 方案：WorkerPool + GLCommandQueue

```
工作线程:  读文件 → 解析JSON → stb_image解码 → 计算法线/切线 → 组装顶点
              ↓ (推送 GLCommand 到队列)
主线程:    GLCommandQueue::Drain() → glGenTextures → glTexImage2D → glGenBuffers
              ↓
           ResourceManager 注册完成 → 回调创建 Entity/Component
```

### 新增文件

#### 1. `WorkerPool.h / WorkerPool.cpp`

简单的线程池，单例，支持提交任务和等待全部完成。

```cpp
class WorkerPool {
public:
    static WorkerPool* Get();
    void Init(int numThreads = 2);
    void Shutdown();
    void Enqueue(std::function<void()> job);
    void WaitAll();   // 阻塞直到所有 job 完成
    bool HasWork();   // 是否还有未完成的工作
private:
    std::vector<std::thread> m_Threads;
    std::queue<std::function<void()>> m_Jobs;
    std::mutex m_Mutex;
    std::condition_variable m_CV;
    std::atomic<int> m_PendingCount{0};
    bool m_Stop = false;
};
```

#### 2. `GLCommandQueue.h / GLCommandQueue.cpp`

线程安全队列，存储待主线程执行的 GL 操作。

```cpp
// GPU 资源创建的延迟命令
struct PendingTexture {
    std::string name;
    int width, height, channels;
    std::vector<unsigned char> pixels;  // stb_image 解码后的原始数据
    int minFilter, magFilter, wrapS, wrapT;
    bool sRGB;
    std::function<void(TextureID)> onComplete;  // 创建完成后回调
};

struct PendingMesh {
    std::string name;
    std::vector<uint8_t> vertexBlob;  // vertices.data() 的字节拷贝
    size_t vertexSize;                // sizeof(Vertex3D)
    std::vector<unsigned int> indices;
    VertexType vertexType;
    std::function<void(MeshID)> onComplete;
};

class GLCommandQueue {
public:
    static GLCommandQueue* Get();
    void PushTexture(PendingTexture&& tex);
    void PushMesh(PendingMesh&& mesh);
    void Drain();         // 主线程每帧调用，执行所有待处理的 GL 命令
    bool HasPending();
private:
    std::mutex m_Mutex;
    std::vector<PendingTexture> m_PendingTextures;
    std::vector<PendingMesh> m_PendingMeshes;
};
```

### 改造文件

#### 3. `ModelLoader.h` — 新增拆分版加载函数

```cpp
// 新增：纯 CPU 的 glTF 解析（不含 GPU 调用）
// 纹理数据用 stb_image 解码到内存，而非创建 GL 纹理
struct RawTextureData {
    std::string name;
    unsigned char* pixels;
    int width, height, channels;
    TextureSemantic semantic;
    SamplerInfo sampler;
};

struct CPUSubMeshData {
    std::vector<Vertex3D> vertices;
    std::vector<unsigned int> indices;
    std::vector<RawTextureData> textures;  // 已解码的纹理像素
    glm::vec4 diffuseColor;
    float metallicFactor, roughnessFactor;
    glm::vec3 emissiveFactor;
    int alphaMode;
    float alphaCutoff;
    bool doubleSided;
};

struct CPUModelData {
    std::vector<CPUSubMeshData> subMeshes;
};

struct CPUGLTFScene {
    std::vector<NodeData> nodes;  // 复用现有 NodeData
    std::vector<int> rootNodes;
    // 每个节点可能有多个 submesh（CPU 数据）
    std::vector<std::vector<CPUSubMeshData>> nodeSubMeshes;  // index 对齐 nodes
};

namespace ModelLoader {
    // 现有同步版本保持不变
    ModelData LoadGLTF(const std::string& filepath);
    GLTFScene LoadGLTFScene(const std::string& filepath);
    
    // 新增 CPU-only 版本（在工作线程调用，不做任何 GL 操作）
    // 纹理用 stb_image 解码到内存，不创建 OpenGL 纹理
    CPUModelData LoadGLTF_CPU(const std::string& filepath);
    CPUGLTFScene LoadGLTFScene_CPU(const std::string& filepath);
};
```

**实现要点：**
- `LoadGLTF_CPU` 和现有的 `LoadGLTF` 共用解析逻辑（TinyGLTF），区别在于纹理处理：
  - 现有版：遍历完顶点后，纹理路径留在 `TextureInfo::path`，由 `EntityFactory` 调 `ResourceManager::LoadTexture(path)` 同步加载
  - CPU版：纹理路径用 stb_image 直接解码到 `RawTextureData::pixels`（`stbi_load` 返回 `unsigned char*`），不碰 GL
- 复用 `SubMeshData` → `CPUSubMeshData` 的映射关系，`NodeData` 不变

#### 4. `EntityFactory.h / EntityFactory.cpp` — 新增异步版

```cpp
// 新增异步创建函数
// 立即创建 entity + 占位组件，后台加载完成后设置完整组件
void CreateModel3DAsync(entt::registry& registry, const std::string& filepath);
void CreateModelHierarchyAsync(entt::registry& registry, const std::string& filepath);
```

**CreateModel3DAsync 实现流程：**

```cpp
void EntityFactory::CreateModel3DAsync(entt::registry& registry, const std::string& filepath) {
    auto entity = registry.create();
    registry.emplace<TagComponent>(entity, TagComponent{.name = "Model3D(loading)"});
    registry.emplace<Transform3DComponent>(entity);
    // 先不放 Model3DComponent，等加载完成

    WorkerPool::Get()->Enqueue([&registry, entity, filepath] {
        // ---- 工作线程 ----
        CPUModelData cpuData = ModelLoader::LoadGLTF_CPU(filepath);
        
        // 计算包围盒
        glm::vec3 boundsMin(FLT_MAX), boundsMax(-FLT_MAX);
        for (auto& sub : cpuData.subMeshes)
            for (auto& v : sub.vertices) {
                boundsMin = glm::min(boundsMin, v.Position);
                boundsMax = glm::max(boundsMax, v.Position);
            }
        
        // 推送 GPU 命令到主线程
        for (size_t i = 0; i < cpuData.subMeshes.size(); i++) {
            auto& sub = cpuData.subMeshes[i];
            
            // 推送 Mesh 创建
            std::string meshName = filepath + "_sub" + std::to_string(i);
            std::vector<uint8_t> blob(sub.vertices.size() * sizeof(Vertex3D));
            memcpy(blob.data(), sub.vertices.data(), blob.size());
            GLCommandQueue::Get()->PushMesh({
                meshName, std::move(blob), sizeof(Vertex3D),
                sub.indices, VertexType::Vertex3D,
                [](MeshID) {}  // 回调留空，Phase 1 简单处理
            });
            
            // 推送纹理创建
            for (auto& tex : sub.textures) {
                GLCommandQueue::Get()->PushTexture({
                    tex.name, tex.width, tex.height, tex.channels,
                    std::vector<unsigned char>(tex.pixels, tex.pixels + tex.width * tex.height * tex.channels),
                    tex.sampler.minFilter, tex.sampler.magFilter,
                    tex.sampler.wrapS, tex.sampler.wrapT, tex.sampler.sRGB,
                    [](TextureID) {}
                });
            }
        }
        
        // TODO Phase 2: 主线程 Drain 后，通过回调创建 Model3DComponent
    });
}
```

> **Phase 1 简化策略：** 第一版可以先不做完美的异步回调。工作线程解析完数据后，设置一个 `std::atomic<bool> m_LoadingComplete`，主线程在 `OnUpdate` 中轮询检测，完成后调用一次同步创建（此时 CPU 数据已在内存，只是补做 GL 调用）。这样 Phase 1 的改动最小，Phase 2 双缓冲时再完善回调机制。

**推荐的简化方案（先做这个）：**

```
Scene::Enter():
  1. 创建工作线程：解析 glTF → CPU 数据暂存到全局 LoadingContext
  2. 设置 m_LoadingComplete = false
  
Scene::OnUpdate():
  if (m_LoadingComplete && !m_EntityCreated):
      GLCommandQueue::Drain()             // 创建所有 GPU 资源
      从 LoadingContext 取出数据，组装 Model3DComponent
      m_EntityCreated = true
  
工作线程:
  1. CPUModelData = LoadGLTF_CPU(path)
  2. 把 CPUModelData 放入 LoadingContext（加锁）
  3. 为每个 submesh/texture 推送 PendingMesh/PendingTexture 到 GLCommandQueue
  4. m_LoadingComplete = true
```

#### 5. `main.cpp` — 主循环增加 Drain

```cpp
// main.cpp Init() 之前
WorkerPool::Get()->Init(2);  // 2个工作线程

// 主循环开头（处理事件之前或之后）
GLCommandQueue::Get()->Drain();
```

#### 6. `ResourceManager.h` — 增加线程安全的方法

由于 `ResourceManager` 的 `GetTextureID`、`GetShaderID` 在 `AnimationSystem` 等工作线程可能调用的代码中只读使用，当前已是只读操作，无需加锁。

**但如果异步加载期间需要写 ResourceManager（注册新资源），必须在主线程的 Drain 中调用，不能在工作线程直接调用。**

所以新增一个约束：**`ResourceManager` 的写操作（LoadShader/LoadTexture/CreateMesh/CreateFramebuffer）只能在主线程调用。** `GLCommandQueue::Drain()` 在主线程执行，它内部调用这些写操作。

#### 7. `AnimatedSpriteComponent` — 使用只读 ResourceManager

`AnimationSystem` 当前只在 `OnUpdate` 中运行，读 `Texture` 指针获取 cols/rows。改为工作线程运行后，仍然是只读操作，安全。

---

## Phase 2：游戏逻辑与渲染解耦

### 问题

当前帧是严格的 Update → Render 串行。GPU 在 CPU Update 期间空闲，CPU 在 GPU 渲染期间等待。

### 方案：双缓冲 RenderCommand + 工作线程跑 Update

```
主线程 (Frame N):                    工作线程:
  DrainGLCommands()                    
  SwapBuffers()  ←─── Front = N ──── 
  ProcessEvents()                     
  OnImGuiRender()    [读 registry]    
  OnRender() / Flush [读 Front][GL]   
  KickWorker ─────────────────→     OnUpdate(N+1) [写 registry]
  SDL_GL_SwapWindow()               SubmitCameras(N+1) → Back
                                    SubmitEntities(N+1) → Back
                                    Signal(m_Done)

  // 下一帧开始
  WaitWorkerDone() ←──────────────── (done)
  SwapBuffers()    ←─── Front = N+1
```

**关键同步点：**
1. `OnImGuiRender` 读 registry → Worker 必须在 ImGui 之前完成
2. `Flush` 读 Front buffer → Worker 写完 Back buffer 后 Swap 才发生
3. Worker 写 registry → 与 ImGui 读 registry 互斥

### 改造文件

#### 8. `Renderer.h / Renderer.cpp` — 双缓冲

```cpp
// Renderer.h 新增
class Renderer {
private:
    struct FrameData {
        std::unordered_map<RenderLayer, std::vector<RenderUnit>> renderUnits;
        std::vector<CameraUnit> camera2DUnits;
        std::vector<CameraUnit> camera3DUnits;
        
        void Clear() {
            for (auto& [_, v] : renderUnits) v.clear();
            camera2DUnits.clear();
            camera3DUnits.clear();
        }
    };
    
    std::array<FrameData, 2> m_FrameData;
    std::atomic<int> m_FrontIdx{0};   // 主线程读的 buffer 索引
    // 工作线程写 m_FrameData[1 - m_FrontIdx]
    
    std::mutex m_BackBufferMutex;      // 保护 Back buffer 的写入
    
public:
    // 工作线程调用（写 Back buffer）
    void SubmitCameraUnits_Back(const glm::mat4& projView, 
                                 const std::vector<RenderLayer>& layers,
                                 RenderMode mode, const glm::vec3& viewPos = {});
    void SubmitRenderUnits_Back(const MeshID& mesh, const Material* material,
                                 const glm::mat4& model, RenderLayer layer);
    
    // 主线程调用
    void SwapBuffers();  // 原子交换 Front/Back
    void Flush(const std::vector<Light3DComponent*>& lights);  // 读 Front，不变
    void ClearFront();   // 渲染后清空 Front（不是 Back）
};
```

**实现：**

```cpp
void Renderer::SubmitCameraUnits_Back(...) {
    int backIdx = 1 - m_FrontIdx.load(std::memory_order_acquire);
    std::lock_guard lock(m_BackBufferMutex);
    // 写入 m_FrameData[backIdx]
}

void Renderer::SwapBuffers() {
    // Front 已经在上一帧被 Flush 消费完并 Clear
    m_FrontIdx.store(1 - m_FrontIdx.load(), std::memory_order_release);
}

void Renderer::ClearFront() {
    int frontIdx = m_FrontIdx.load();
    m_FrameData[frontIdx].Clear();
}

// Flush 改为读 Front
void Renderer::Flush(const std::vector<Light3DComponent*>& lights) {
    int frontIdx = m_FrontIdx.load();
    SortAll(m_FrameData[frontIdx].renderUnits);  // SortAll 接受参数
    
    RenderContext ctx{
        m_FrameData[frontIdx].renderUnits,
        m_FrameData[frontIdx].camera2DUnits,
        m_FrameData[frontIdx].camera3DUnits,
        lights, *m_LightUBO, *m_PerFrameUBO, *m_PerObjectUBO, m_Viewport
    };
    m_Pipeline->Execute(ctx);
    
    ClearFront();  // 只清 Front，不影响 Back
}
```

#### 9. `Systems.h / Systems.cpp` — 提交函数改为双缓冲

当前 `RenderSubmitSystem::SubmitCameras` 和 `SubmitEntities` 都调用 `Renderer::Get()->SubmitCameraUnits()` / `SubmitRenderUnits()`。

需要新增 Back-buffer 版本：

```cpp
// Systems.h 新增
namespace RenderSubmitSystem {
    // 原有（Phase 2 后仅工作线程调用）
    void SubmitCameras(entt::registry& registry);      // → 改调 _Back
    void SubmitEntities(entt::registry& registry);     // → 改调 _Back
}
```

在 `SubmitCameras` 和 `SubmitEntities` 内部，将 `Renderer::Get()->SubmitCameraUnits(...)` 改为 `SubmitCameraUnits_Back(...)`。

**为了可以让 Phase 1 和 Phase 2 独立开启，建议用条件编译或运行时开关：**

```cpp
// GameSettings 中新增
bool GameSettings::s_MultiThreadedUpdate = false;  // Phase 2 开关

// Systems.cpp
void RenderSubmitSystem::SubmitCameras(entt::registry& registry) {
    auto& renderer = *Renderer::Get();
    // ...
    if (GameSettings::Get()->IsMultiThreadedUpdate()) {
        renderer.SubmitCameraUnits_Back(projView, layers, mode, viewPos);
    } else {
        renderer.SubmitCameraUnits(projView, layers, mode, viewPos);
    }
}
```

#### 10. `Scene.h` — 新增异步 Update 方法

```cpp
class Scene {
public:
    // 新增：在工作线程运行的更新（Phase 2）
    // 完成所有 System 更新 + 渲染提交
    virtual void OnUpdateAsync(float deltaTime) {
        OnUpdate(deltaTime);
        // 子类覆盖以添加 SubmitCameras + SubmitEntities
    }
};
```

**BattleScene 新增：**
```cpp
void BattleScene::OnUpdateAsync(float deltaTime) {
    OnUpdate(deltaTime);
    RenderSubmitSystem::SubmitCameras(m_Registry);
    RenderSubmitSystem::SubmitEntities(m_Registry);
}
```

#### 11. `SceneManager.h / SceneManager.cpp` — 管理多线程生命周期

```cpp
class SceneManager {
private:
    std::thread m_UpdateThread;
    std::mutex m_UpdateMutex;
    std::condition_variable m_UpdateCV;
    std::atomic<bool> m_UpdateReady{false};   // Worker 完成
    std::atomic<bool> m_UpdateKick{false};    // Main 让 Worker 开始
    std::atomic<bool> m_StopWorker{false};
    
    float m_DeltaTimeForWorker = 0.0f;
    
public:
    void KickUpdateThread(float deltaTime);   // 主线程调用，启动异步 Update
    void WaitUpdateThread();                   // 主线程调用，等待完成
    void StopUpdateThread();                   // 退出时调用
};
```

**实现：**

```cpp
// SceneManager 初始化时启动工作线程
void SceneManager::Init() {
    m_UpdateThread = std::thread([this] {
        while (!m_StopWorker.load()) {
            // 等待主线程 kick
            {
                std::unique_lock lock(m_UpdateMutex);
                m_UpdateCV.wait(lock, [this] { 
                    return m_UpdateKick.load() || m_StopWorker.load(); 
                });
            }
            if (m_StopWorker.load()) break;
            
            // 执行异步 Update + Submit
            if (m_CurrentScene) {
                m_CurrentScene->OnUpdateAsync(m_DeltaTimeForWorker);
            }
            
            // 通知主线程完成
            m_UpdateKick.store(false);
            m_UpdateReady.store(true);
        }
    });
}

void SceneManager::KickUpdateThread(float dt) {
    m_DeltaTimeForWorker = dt;
    m_UpdateReady.store(false);
    m_UpdateKick.store(true);
    m_UpdateCV.notify_one();
}

void SceneManager::WaitUpdateThread() {
    // 等待 Worker 完成（或超时）
    while (!m_UpdateReady.load() && !m_StopWorker.load()) {
        std::this_thread::yield();  // 或 sleep(0)
    }
}
```

#### 12. `main.cpp` — 新版主循环

```cpp
// main loop (Phase 2)
while (!done)
{
    // 1. 等待上一帧的异步 Update 完成
    SceneManager::Get()->WaitUpdateThread();
    
    // 2. 消费 GPU 命令 + 交换渲染缓冲
    GLCommandQueue::Get()->Drain();
    Renderer::Get()->SwapBuffers();
    
    // 3. 处理事件
    SceneManager::Get()->BeginFrame();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        SceneManager::Get()->OnEvent(event);
        if (event.type == SDL_EVENT_QUIT) done = true;
        // ...
    }
    if (minimized) continue;
    
    // 4. 计算 deltaTime
    Uint64 now = SDL_GetTicks();
    float deltaTime = (now - lastTime) / 1000.0f;
    lastTime = now;
    SceneManager::Get()->EndFrame();
    
    // 5. ImGui（读 registry，Worker 已停止，安全）
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    SceneManager::Get()->OnImGuiRender();
    
    // 6. 渲染当前帧（读 Front buffer，GL 调用）
    ImGui::Render();
    glClear(...);
    SceneManager::Get()->OnRender();  // Flush 读 Front buffer
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    // 7. 启动下一帧的异步 Update
    if (GameSettings::Get()->IsMultiThreadedUpdate()) {
        SceneManager::Get()->KickUpdateThread(deltaTime);
    } else {
        SceneManager::Get()->OnUpdate(deltaTime);
    }
    
    // 8. 显示
    SDL_GL_SwapWindow(window);
    
    // GPU 在渲染时，Worker 在跑 Update+Submit（重叠）
}

// 退出
SceneManager::Get()->StopUpdateThread();
```

**关键：** `OnRender()` 也要改造 —— 如果 Phase 2 开启，`OnRender` 不再调用 `SubmitCameras/SubmitEntities`（因为 Worker 已经做了），只做 `Flush`。

```cpp
// StoryScene::OnRender 改造
void StoryScene::OnRender() {
    if (!GameSettings::Get()->IsMultiThreadedUpdate()) {
        // 串行模式：提交 + 渲染
        RenderSubmitSystem::SubmitCameras(m_Registry);
        RenderSubmitSystem::SubmitEntities(m_Registry);
        DebugRenderSystem::SubmitColliderBoxes(m_Registry);
        DebugRenderSystem::SubmitLightRanges(m_Registry);
    }
    // 多线程模式：Worker 已经提交到 Back buffer，SwapBuffer 后直接 Flush
    
    std::vector<Light3DComponent*> lights;
    // ...收集 lights...
    Renderer::Get()->Flush(lights);
}
```

---

## 文件清单

### Phase 1 新增文件

| 文件 | 说明 |
|------|------|
| `WorkerPool.h` | 线程池头文件 |
| `WorkerPool.cpp` | 线程池实现（~60行） |
| `GLCommandQueue.h` | GL 命令队列头文件 |
| `GLCommandQueue.cpp` | GL 命令队列实现（~80行） |

### Phase 1 改造文件

| 文件 | 改动 |
|------|------|
| `ModelLoader.h` | 新增 `CPUModelData`, `CPUSubMeshData`, `RawTextureData` 结构体；新增 `LoadGLTF_CPU`, `LoadGLTFScene_CPU` |
| `ModelLoader.cpp` | 实现 CPU-only 加载（纹理用 stb_image 解码到内存） |
| `EntityFactory.h` | 新增 `CreateModel3DAsync`, `CreateModelHierarchyAsync` |
| `EntityFactory.cpp` | 暂用简化方案：Worker 解析→存 LoadingContext→主线程轮询创建 |
| `ResourceManager.h` | 新增 `CreateTextureFromMemory` 重载（接受已解码像素） |
| `main.cpp` | `Init()` 前初始化 WorkerPool，主循环中 `Drain()` |
| `GameSettings.h/cpp` | 新增 `s_AsyncLoading` 开关 |

### Phase 2 改/增文件

| 文件 | 改动 |
|------|------|
| `Renderer.h` | 新增 `FrameData` 结构，双缓冲数组，`SubmitXxx_Back` 方法，`SwapBuffers`, `ClearFront` |
| `Renderer.cpp` | 实现双缓冲逻辑，`SortAll` 接受参数 |
| `Systems.cpp` | `SubmitCameras/SubmitEntities` 改为调用 `_Back` 版本 |
| `Scene.h` | 新增 `OnUpdateAsync` 虚函数 |
| `BattleScene.cpp/h` | 实现 `OnUpdateAsync` |
| `StoryScene.cpp/h` | 实现 `OnUpdateAsync`；`OnRender` 改为仅在串行模式下提交 |
| `SceneManager.h/cpp` | 新增工作线程管理（`m_UpdateThread`, `KickUpdateThread`, `WaitUpdateThread`） |
| `main.cpp` | 新版主循环（Wait → Drain → Swap → ImGui → Render → Kick） |
| `GameSettings.h/cpp` | 新增 `s_MultiThreadedUpdate` 开关 |

---

## 实现顺序建议

```
1. WorkerPool.h/cpp          — 最基础，无依赖
2. GLCommandQueue.h/cpp      — 依赖 WorkerPool 的概念（但实现独立）
3. ModelLoader CPU 版        — 在现有 LoadGLTF 基础上拆分
4. EntityFactory Async 版    — 简化方案：Work→Context→主线程轮询
5. main.cpp 集成 Phase 1     — 启动时用 WorkerPool 加载 Init 资源
6. 测试 Phase 1
7. Renderer 双缓冲           — FrameData + Front/Back + SwapBuffers
8. Systems.cpp _Back 版本    — 改调
9. Scene/SceneManager 工作线程 — UpdateThread 生命周期
10. main.cpp Phase 2 循环    — 新主循环
11. 测试 Phase 2             — 对比串行/多线程性能
```

---

## 风险与注意事项

1. **stb_image 线程安全：** `stbi_set_flip_vertically_on_load` 等全局设置是 thread_local，工作线程加载前需各自设置
2. **EnTT registry 并发：** Phase 2 中 Worker 写 registry、Main 读 registry（ImGui），通过同步点保证互斥（Worker 完成后 Main 才读）
3. **Material 指针：** `RenderUnit` 存储 `const Material*`，指向 ECS 中的 `unique_ptr<Material>`。Worker 提交时 Material 必须存活。当前 Material 生命周期与 Entity 绑定，没问题
4. **退化保证：** 如果 Worker 比 GPU 慢，主循环在 `WaitUpdateThread` 处阻塞，自动退化为串行性能，不会更差
5. **2D BattleScene 流程相同：** `OnUpdateAsync` 包含 PlayerSystem → AnimationSystem → CameraSystem → PhysicsSystem → HierarchySystem → DestroySystem → SubmitCameras → SubmitEntities
