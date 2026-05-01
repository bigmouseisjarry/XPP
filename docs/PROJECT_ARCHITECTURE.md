# 项目流程架构详解

---

## 1. 项目定位

**概括**：这是一个基于 OpenGL 4.5 的实时 PBR 游戏渲染引擎，使用 ECS 架构管理场景，实现了从模型加载到后处理的完整渲染管线，意在实现以节点的方式去构建完整游戏

**技术选型理由**：

| 选择 | 理由 |
|------|------|
| OpenGL 4.5 | 提供 DSA（直接状态访问），减少状态绑定调用；要求显式管理所有 GPU 资源，体现对图形 API 的深入理解 |
| EnTT | 最快的 C++ ECS 库，header-only，稀疏集合存储天然缓存友好，无虚函数开销 |
| SDL3 | 轻量跨平台窗口库，相比 GLFW 提供更完整的输入/事件系统，学习成本低 |
| Dear ImGui | immediate mode 调试 UI，开发渲染效果时实时调参必备 |
| glTF 2.0 | 现代 PBR 模型标准，物理材质定义完整，替代 FBX/OBJ 等旧格式 |

---

## 2. 程序生命周期

```
main()
 │
 ├─ 1. 初始化阶段
 │   ├─ SDL3 窗口创建 + OpenGL 4.5 Core Context
 │   ├─ GLEW 扩展加载
 │   ├─ ResourceManager 初始化（注册着色器、纹理、网格、帧缓冲）
 │   ├─ ImGui 初始化（SDL3 + OpenGL3 后端）
 │   ├─ Renderer 初始化（构建 RenderPipeline，注册所有 Pass）
 │   ├─ Scene 创建（BattleScene 或 StoryScene）
 │   │   ├─ EntityFactory 批量创建实体（相机、灯光、模型、玩家）
 │   │   └─ ECS 系统注册（相机、动画、物理、层级、灯光、渲染提交）
 │   └─ InputManager 初始化
 │
 ├─ 2. 主循环 (每帧)
 │   ├─ ProcessEvents()    —— SDL3 事件泵（键盘/鼠标/窗口）
 │   ├─ InputManager::Update() —— 输入状态聚合
 │   ├─ Scene::Update(dt)  —— ECS 系统执行
 │   │   ├─ CameraSystem      —— 轨道相机更新、View/Projection 矩阵
 │   │   ├─ AnimationSystem   —— 精灵表帧更新
 │   │   ├─ PhysicsSystem     —— 碰撞检测（2D AABB / 3D SAT）
 │   │   ├─ HierarchySystem   —— 父子变换脏标记传播
 │   │   ├─ LightSystem       —— 灯光视锥体拟合、阴影矩阵更新
 │   │   └─ SubmitSystem      —— 收集可见实体到渲染队列
 │   ├─ Renderer::Render() —— 渲染管线执行
 │   │   └─ RenderPipeline::Execute()
 │   │       └─ for each Pass: Setup → Execute → Teardown
 │   ├─ ImGui 调试面板渲染
 │   └─ SDL_GL_SwapWindow() —— 交换缓冲区
 │
 └─ 3. 清理阶段
     ├─ Scene 销毁（ECS 注册表 + 实体）
     ├─ Renderer 销毁（管线 + FBO + UBO）
     ├─ ResourceManager 销毁（GPU 资源释放）
     ├─ ImGui 销毁
     └─ SDL3 销毁
```

**关键设计点**：
- 初始化顺序严格按依赖关系：窗口 → GL上下文 → 资源 → 管线 → 场景
- 主循环的三个阶段（Input → Update → Render）完全解耦，便于单独测试和调试
- 清理顺序是初始化的逆序，确保 GL 资源在上下文销毁前释放

---

## 3. 核心架构分层

```
┌──────────────────────────────────────────────────────────┐
│                     Application Layer                     │
│                                                          │
│   SDL3 窗口管理  ·  ImGui 调试面板  ·  事件循环           │
│   (平台抽象)        (实时参数调整)       (输入→更新→渲染)  │
├──────────────────────────────────────────────────────────┤
│                       Scene Layer                         │
│                                                          │
│   ┌──────────┐   ┌──────────┐   ┌──────────┐            │
│   │ Entities │   │Components│   │ Systems  │            │
│   │  (ID)    │   │ (纯数据)  │   │ (纯逻辑)  │            │
│   └──────────┘   └──────────┘   └──────────┘            │
│                                                          │
│   相机系统 · 动画系统 · 物理系统 · 层级系统 ·             │
│   灯光系统 · 渲染提交系统 · 玩家系统 · 销毁系统           │
├──────────────────────────────────────────────────────────┤
│                      Renderer Layer                       │
│                                                          │
│   Renderer 协调器：                                       │
│   ├─ UBO 管理（PerFrame / Lights / ShadowMatrices）      │
│   ├─ 管线构建（Pass 注册、顺序编排）                      │
│   └─ 渲染入口（接收 Scene 提交的队列）                    │
│                                                          │
│   ResourceManager：                                       │
│   ├─ 强类型 ID 体系（编译期类型安全）                     │
│   ├─ 名称↔ID 双向查找                                    │
│   └─ 统一生命周期管理                                     │
├──────────────────────────────────────────────────────────┤
│                     Pipeline Layer                        │
│                                                          │
│   RenderPipeline 执行引擎：                               │
│   ├─ Pass 有序遍历（Setup → Execute → Teardown）         │
│   ├─ 纹理池依赖解析（(语义, 级别) → GL 纹理 ID）         │
│   └─ PSO 状态管理（Blend/Depth/Cull/Viewport）           │
│                                                          │
│   20 个 Pass：Shadow → Skybox → Opaque3D → Transparent3D │
│             → Bloom(1+8+3) → SSAO(1+1) → Composite       │
├──────────────────────────────────────────────────────────┤
│                       OpenGL Layer                        │
│                                                          │
│   GPU 资源：VAO · VBO · IBO · UBO · Texture · FBO · PSO  │
│   GL 状态机封装：PipelineUtils（捕获/应用/恢复）          │
│   Shader 反射：Uniform 自动绑定                           │
└──────────────────────────────────────────────────────────┘
```

**层间通信规则**：
- Scene Layer → Renderer Layer：通过渲染队列（实体列表 + 相机 + 灯光 UBO 数据）
- Renderer Layer → Pipeline Layer：Renderer 构建管线时注册 Pass，运行时传递 FBO 尺寸
- Pipeline Layer → OpenGL Layer：每个 Pass 通过 PipelineUtils 管理 GL 状态
- **下层不依赖上层**：OpenGL 层不知道 Pipeline 的存在，Pipeline 层不知道 Scene 的存在

---

## 4. 一帧的渲染旅程

以下追踪一个 glTF 模型的三角形是如何最终呈现在屏幕上的：

```
[磁盘] glTF 文件 (.glb/.gltf)
   │  tinygltf 解析 JSON + 二进制 buffer
   ▼
[CPU 内存] Mesh 数据 (Vertex数组 + Index数组) + Material 描述 + 节点层级
   │  ModelLoader 执行：
   │  · Y-up → Z-up 坐标系转换（glTF 标准是 Y-up，本引擎使用 Z-up）
   │  · Lengyel 算法计算切线（法线映射必需）
   │  · 顶点去重、材质 PBR 参数提取
   ▼
[GPU 内存] VAO + VBO + IBO + 纹理对象
   │  UploadMesh() → glBufferData / glTexImage2D
   ▼
[ECS] Entity(1) + ComTransform(position, rotation, scale) 
     + ComRender(meshID, materialID, visible=true)
   │  HierarchySystem 计算世界矩阵（脏标记传播）
   │  SubmitSystem 收集可见实体 → m_RenderQueue3D
   ▼
[渲染帧] RenderPipeline::Execute()
   │
   ├─ Pass 1: ShadowPass
   │   对每盏灯光渲染 gl_Layer 到 Layered Depth Array
   │   输出: ShadowDepthArray (8层 2D Array Texture)
   │
   ├─ Pass 2: SkyboxPass
   │   渲染 HDR 天空盒到 SceneColor (RGBA16F)
   │   输出: SceneColor, Depth
   │
   ├─ Pass 3: OpaquePass3D
   │   PBR Shader: 采样 Albedo/Normal/MetallicRoughness/AO/Emissive
   │   → TBN 矩阵(含正交化) → 法线映射
   │   → GGX 法线分布 → Smith 几何遮蔽 → Schlick 菲涅尔
   │   → 8盏灯光逐盏累加（含阴影因子 PCF 采样）
   │   → MRT 输出: SceneColor(RGBA16F) + WorldNormal(RGBA16F)
   │   输出: SceneColor, WorldNormal, Depth
   │
   ├─ Pass 4: TransparentPass3D
   │   同上但开启 Blend（排序后半透明混合）
   │
   ├─ Pass 5-13: Bloom (Bright + 4级BlurH/V + 3级Up)
   │   BloomBright: dot(color.rgb, lumWeights) > threshold → 提取亮区
   │   BlurH (水平): 1D 高斯核，输入 SceneColor → 输出 BloomBlurH_L0
   │   BlurV (垂直): 1D 高斯核，输入 BloomBlurH_L0 → 输出 BloomBlur_L0
   │   (重复 L1/L2/L3，每级分辨率减半)
   │   Up L1: BloomBlur_L1 + Up(L2) → BloomResult_L1
   │   Up L0: BloomBlur_L0 + Up(L1) → BloomResult_L0 (最终 Bloom 纹理)
   │   输出: BloomResult (多级分辨率，最高分辨率供 Composite 使用)
   │
   ├─ Pass 14-15: SSAO (SSAO + SSAOBlur)
   │   SSAO: 从 Depth 逆投影重建视空间位置
   │        dFdx/dFdy 重建视空间法线
   │        → 64个半球方向采样（UBO 传输核）
   │        → 4×4 旋转噪声去带状伪影
   │        → 范围检测限制遮蔽距离
   │   输出: SSAOResult
   │   SSAOBlur: 5×5 盒模糊降噪
   │   输出: SSAOBlurResult
   │
   ├─ Pass 16: Composite
   │   输入: SceneColor + BloomResult + SSAOBlurResult
   │   → SSAO 遮蔽值调制场景颜色
   │   → Bloom 光晕叠加
   │   → Reinhard 色调映射: color / (color + 1.0)
   │   → Gamma 校正: color^(1/2.2)
   │   输出: 默认帧缓冲（屏幕）
   │
   └─ [屏幕] 最终像素
```

**关键数据流图**：

```
                    ┌──────────────┐
                    │  ShadowPass  │
                    └──────┬───────┘
                           │ ShadowDepthArray (8层)
                           ▼
┌──────────┐    ┌──────────────┐    ┌──────────────────┐
│SkyboxPass│───▶│ OpaquePass3D │───▶│TransparentPass3D │
│          │    │  MRT:        │    │  (半透明混合)     │
│HDR天空盒 │    │  Color+Normal│    │                  │
└──────────┘    └──────┬───────┘    └────────┬─────────┘
                       │ SceneColor(HDR)     │
                       │ WorldNormal         │
                       │ Depth               │
                       └──────────┬──────────┘
                                  │
              ┌───────────────────┼───────────────────┐
              ▼                   ▼                   ▼
    ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
    │BloomBright  │     │   SSAO      │     │             │
    │  4级Blur    │     │  SSAOBlur   │     │             │
    │  3级Up      │     │             │     │             │
    └──────┬──────┘     └──────┬──────┘     │             │
           │ BloomResult       │ SSAOResult │             │
           └───────────────────┼────────────┘             │
                               ▼                          │
                     ┌─────────────────┐                  │
                     │   Composite     │◄─────────────────┘
                     │ 色调映射+Gamma  │
                     └────────┬────────┘
                              ▼
                         [屏幕输出]
```

---

## 5. ECS 架构设计

### 组件设计原则

```
组件 = 纯数据 (POD struct)
系统 = 纯逻辑 (函数，遍历拥有特定组件的实体)
实体 = 整数 ID (entt::entity)
```

**组件清单**：

| 组件 | 所属领域 | 关键字段 |
|------|---------|---------|
| ComTransform | 空间 | position, rotation, scale (2D/3D 独立结构), parent, dirty flag |
| ComCamera | 渲染 | FOV, near/far, orbit参数(距离/方位角/仰角), 2D/3D 分离 |
| ComLight | 渲染 | color, intensity, type, shadowLayer, 视锥体8角缓存 |
| ComRender | 渲染 | meshID, materialID, visible, 动画精灵(2D) |
| ComPhysics | 物理 | velocity, 2D AABB / 3D OBB |
| ComGameplay | 逻辑 | 玩家状态机, tag标签, pendingDestroy |

### 层级脏标记系统

父节点移动时，子节点的世界矩阵需要重算。传统方案是每帧全量重算，本项目的优化：

```
ComTransform::SetLocalPosition()
  → 标记自身 dirty
  → 递归标记所有子节点 dirty (通过 HierarchyUtils)

CameraSystem::Update()
  → 遍历所有 transform
  → 仅对 dirty 实体重算世界矩阵
  → 清除 dirty 标记
```

### 为什么 2D/3D 共用一套 ECS

- 2D 战斗场景和 3D 展示场景复用相同的变换层级、相机、物理系统
- 组件内部通过 `union` 或独立字段区分 2D/3D 数据（如 ComTransform::m_Transform2D / m_Transform3D）
- 渲染系统通过 Scene 类型决定走 2D Pass 还是 3D Pass

---

## 6. 资源管理设计

### 强类型 ID 体系

```cpp
struct TextureID     { uint32_t value; };
struct MeshID        { uint32_t value; };
struct ShaderID      { uint32_t value; };
struct FramebufferID { uint32_t value; };

// 编译期防混用
static_assert(!std::is_same_v<TextureID, MeshID>);

// 自定义 hash 支持 unordered_map
template<> struct std::hash<TextureID> {
    size_t operator()(TextureID id) const { return id.value; }
};
```

**好处**：
- `void BindTexture(TextureID id)` 不会接受 MeshID 参数，编译期报错
- 避免了 OpenGL 原生 `GLuint` 裸传导致的"用纹理 ID 去绑定着色器"类 bug

### 统一生命周期

所有 GPU 资源通过 ResourceManager 统一管理：
- `Register*()` 创建资源并分配 ID
- `Get*(ID)` 按 ID 查询
- `Find*(name)` 按名称查询（用于调试和序列化）
- `ResourceManager::Cleanup()` 批量释放所有 GPU 资源

不需要 shared_ptr 或引用计数——资源生命周期与 ResourceManager 绑定，符合 RAII。

---

## 7. 渲染器架构演变

### 原始架构 (硬编码管线)

```
Renderer::Flush() {
    // 所有状态切换、纹理绑定、绘制调用混杂在一起
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    for (auto& entity : opaqueQueue) {
        // 手动绑定 shader、纹理、UBO...
        DrawEntity(entity);
    }
    glEnable(GL_BLEND);
    // ...
}
```

**问题**：每加一个效果（阴影、Bloom、SSAO）都要改 Flush()，函数膨胀到数百行，状态管理混乱。

### 当前架构 (声明式管线)

```cpp
// 每个 Pass 自描述
class ShadowPass : public RenderPass {
    void DeclareInputs()  override { /* 无需输入 */ }
    void DeclareOutputs() override { Output(TextureSemantic::ShadowDepth, Level0); }
    void Execute(const RenderContext& ctx) override { /* 渲染各光源深度层 */ }
};

// RenderPipeline 自动解析依赖
void RenderPipeline::Execute(const RenderContext& ctx) {
    for (auto& pass : m_Passes) {
        // 1. 从纹理池解析输入 → 绑定到对应纹理单元
        // 2. 执行 Pass
        // 3. 注册输出到纹理池
    }
}
```

**收益**：新增 Bloom 后处理只需写 BloomBright/BloomBlur/BloomUp 三个 Pass 类，然后在管线中注册——3 行代码，不碰任何现有代码。

---

## 8. 构建与调试流程

### 开发工作流

```
编写/修改代码 → VS2022 编译 (x64 Release) → 
运行程序 → ImGui 面板实时调参 → 
确认效果 → 将参数固化为代码默认值
```

### ImGui 调试面板功能

- 渲染状态可视化（FPS、draw call 数、Pass 耗时）
- 实时参数调节（光照强度、Bloom 阈值、SSAO 半径/强度）
- 纹理查看器（检查中间 Pass 输出：深度图、法线图、Bloom 各级）
- 实体列表（选中查看组件详情）
- 着色器热重载（可选功能）

---

## 9. 关键文件速查

| 文件 | 职责 | 面试重点 |
|------|------|---------|
| `main.cpp` | 入口、主循环 | 程序生命周期 |
| `Renderer.h/cpp` | 渲染协调 | UBO 管理、管线构建 |
| `RenderPipeline.h/cpp` | 管线执行 | 纹理池、Pass 调度 |
| `RenderPass.h` | Pass 抽象 | 声明式 I/O 接口 |
| `Scene.h` | 场景抽象 | ECS 注册、系统绑定 |
| `EntityFactory.h/cpp` | 实体创建 | 工厂模式、批量构造 |
| `Systems.cpp` | 游戏逻辑系统 | 相机/动画/层级/渲染提交 |
| `ResourceManager.h/cpp` | 资源管理 | 强类型 ID、生命周期 |
| `ModelLoader.h/cpp` | 模型加载 | glTF 2.0 解析流程 |
| `ComLight.h/cpp` | 灯光组件 | 视锥体拟合算法 |
| `TextureSemantic.h` | 纹理语义 | 枚举定义、槽位映射 |
