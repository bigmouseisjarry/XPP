# 项目流程架构详解

---

## 1. 项目定位

**概括**：这是一个基于 OpenGL 4.5 的实时 PBR 游戏渲染引擎，使用 ECS 架构管理场景，实现了从模型加载到后处理的完整渲染管线，包含 IBL 环境光照、粒子系统、物理碰撞等完整游戏功能，意在以节点的方式去构建完整游戏。

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
 │   ├─ Renderer 初始化（创建 UBO、默认 FBO、噪声纹理）
 │   ├─ SceneManager 初始化，注册 Scene2D / Scene3D
 │   ├─ Scene 进入（Enter）
 │   │   ├─ EntityFactory 批量创建实体（相机、灯光、模型、玩家、粒子）
 │   │   ├─ ECS 系统注册（相机、动画、物理、层级、灯光、粒子、渲染提交）
 │   │   └─ BuildPipeline()：场景自主构建渲染管线（注册 Pass、创建 FBO）
 │   └─ InputManager 初始化
 │
 ├─ 2. 主循环 (每帧)
 │   ├─ ProcessEvents()    —— SDL3 事件泵（键盘/鼠标/窗口）
 │   ├─ InputManager::Update() —— 输入状态聚合
 │   ├─ Scene::Update(dt)  —— ECS 系统执行
 │   │   ├─ CameraSystem      —— 轨道相机更新、View/Projection 矩阵
 │   │   ├─ AnimationSystem   —— 精灵表帧更新
 │   │   ├─ PhysicsSystem     —— 碰撞检测（2D AABB / 3D OBB SAT + 射线检测）
 │   │   ├─ HierarchySystem   —— 父子变换脏标记传播
 │   │   ├─ LightSystem       —— 灯光视锥体拟合、阴影矩阵更新
 │   │   ├─ ParticleSystem    —— 粒子发射、速度/重力/阻力积分、生命周期管理
 │   │   ├─ PlayerSystem      —— 玩家状态机（Idle/Walk/Jump/Fall）
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
- **管线构建权下放 Scene**：每个 Scene 通过 `BuildPipeline()` 自主决定渲染管线的组成，不同场景可有完全不同的渲染策略

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
│   灯光系统 · 粒子系统 · 渲染提交系统 · 玩家系统           │
│                                                          │
│   每个 Scene 通过 BuildPipeline() 自主构建渲染管线        │
├──────────────────────────────────────────────────────────┤
│                      Renderer Layer                       │
│                                                          │
│   Renderer 协调器：                                       │
│   ├─ UBO 管理（PerFrame / Lights / SSAOKernel）          │
│   ├─ 相机/灯光/Instanced 渲染队列管理                     │
│   └─ 渲染入口（接收 Scene 提交的队列，驱动管线）          │
│                                                          │
│   ResourceManager：                                       │
│   ├─ 强类型 ID 体系（编译期类型安全）                     │
│   ├─ 名称↔ID 双向查找                                    │
│   └─ 统一生命周期管理                                     │
│                                                          │
│   WorkerPool + GLCommandQueue：                           │
│   ├─ 2 线程池异步模型加载                                 │
│   └─ 延迟 GL 操作同步到主线程                             │
├──────────────────────────────────────────────────────────┤
│                     Pipeline Layer                        │
│                                                          │
│   RenderPipeline 执行引擎：                               │
│   ├─ Pass 有序遍历（Setup → Execute → Teardown）         │
│   ├─ 纹理池依赖解析（(语义, 级别) → GL 纹理 ID）         │
│   └─ PSO 状态管理（Blend/Depth/Cull/Viewport）           │
│                                                          │
│   3D 管线 23 个 Pass：                                    │
│   Shadow → IBL×4 → Skybox → Opaque3D → Transparent3D     │
│   → Bloom(Bright + BlurH×4 + BlurV×4 + Up×4)             │
│   → SSAO + SSAOBlur → Composite                          │
├──────────────────────────────────────────────────────────┤
│                       OpenGL Layer                        │
│                                                          │
│   GPU 资源：VAO · VBO · IBO · UBO · Texture · FBO · PSO  │
│   GL 状态机封装：PipelineUtils（捕获/应用/恢复）          │
│   Shader 反射：Uniform 自动绑定                           │
└──────────────────────────────────────────────────────────┘
```

**层间通信规则**：
- Scene Layer → Renderer Layer：通过渲染队列（实体列表 + 相机 + 灯光 UBO 数据 + Instanced 粒子数据）
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
   │  · （可选）WorkerPool 异步加载 → GLCommandQueue 同步上传
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
   ├─ IBL Passes (一次性初始化，首帧执行后跳过)
   │   ├─ EquirectToCubemapPass: HDR等距柱状图 → 环境立方体贴图
   │   ├─ IrradiancePass: 立方体贴图 → 漫反射辐照度图 (32×32)
   │   ├─ PrefilterPass: 立方体贴图 → 镜面反射预过滤图 (128×128, 5 mip)
   │   └─ BRDFLUTPass: 生成 BRDF 积分查找表 (512×512, RG16F)
   │
   ├─ Pass 5: ShadowPass
   │   对每盏灯光渲染 gl_Layer 到 Layered Depth Array (2048×2048, 8层)
   │   输出: ShadowDepthArray
   │
   ├─ Pass 6: SkyboxPass
   │   渲染 HDR 天空盒到 SceneColor (RGBA16F)
   │   输出: SceneColor, Depth
   │
   ├─ Pass 7: OpaquePass3D
   │   PBR Shader: 采样 Albedo/Normal/MetallicRoughness/AO/Emissive
   │   → TBN 矩阵(含正交化) → 法线映射
   │   → IBL 漫反射采样(IrradianceMap) + IBL 镜面反射采样(PrefilterMap + BRDFLUT)
   │   → 8盏灯光逐盏累加 Cook-Torrance（含阴影因子 PCF 采样）
   │   → MRT 输出: SceneColor(RGBA16F) + WorldNormal(RGBA16F)
   │   输出: SceneColor, WorldNormal, Depth
   │
   ├─ Pass 8: TransparentPass3D
   │   同上但开启 Blend（排序后半透明混合）
   │
   ├─ Pass 9-24: Bloom (Bright + 4级BlurH+BlurV + 4级Up)
   │   BloomBright: dot(color.rgb, lumWeights) > threshold → 提取亮区
   │   BlurH (水平): 1D 高斯核，输入 → 输出 Pong
   │   BlurV (垂直): 1D 高斯核，输入 Pong → 输出 Ping
   │   (重复 L0/L1/L2/L3，每级分辨率减半: 1/2→1/4→1/8→1/16)
   │   Up3: BlurV2 + Up(上一层) → bloomUp3
   │   Up2: BlurV1 + Up3 → bloomUp2
   │   Up1: BlurV0 + Up2 → bloomUp1
   │   Up0: Up1 → BloomResult (全分辨率)
   │   输出: Bloom (全分辨率)
   │
   ├─ Pass 25-26: SSAO (SSAO + SSAOBlur)
   │   SSAO: 从 Depth 逆投影重建视空间位置
   │        dFdx/dFdy 重建视空间法线
   │        → 64个半球方向采样（UBO 传输核）
   │        → 4×4 旋转噪声去带状伪影
   │        → 范围检测限制遮蔽距离
   │   输出: SSAOResult
   │   SSAOBlur: 5×5 盒模糊降噪
   │   输出: SSAOBlurResult
   │
   ├─ Pass 27: Composite
   │   输入: SceneColor + Bloom + SSAO
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
IBL Passes (一次性)
  HDR天空盒 → EquirectToCubemap → Irradiance + Prefilter + BRDFLUT
                                          │                  │
                    ┌─────────────────────┘                  │
                    ▼                                        │
              ┌──────────┐    ┌──────────────┐              │
              │ShadowPass│───▶│ OpaquePass3D │              │
              │8层深度   │    │  IBL+PBR+阴影 │              │
              └──────────┘    └──────┬───────┘              │
                                    │ SceneColor(HDR)      │
                                    │ WorldNormal          │
                                    │ Depth                │
                                    └──────────┬───────────┘
                                               │
              ┌────────────────────────────────┼────────────────────┐
              ▼                                ▼                    ▼
    ┌──────────────────┐              ┌─────────────┐      ┌─────────────┐
    │BloomBright       │              │   SSAO      │      │             │
    │  BlurH×4+BlurV×4 │              │  SSAOBlur   │      │             │
    │  Up×4            │              │             │      │             │
    └────────┬─────────┘              └──────┬──────┘      │             │
             │ Bloom                         │ SSAO        │             │
             └───────────────────────────────┼─────────────┘             │
                                             ▼                           │
                                   ┌─────────────────┐                   │
                                   │   Composite     │◀──────────────────┘
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
| ComParticle | 渲染 | 发射率、最大粒子数、发射器形状(点/盒/球/锥)、生命周期/速度/方向范围、颜色/大小渐变曲线、重力/阻力、旋转、精灵表、加法混合、子发射器 |
| ComPhysics | 物理 | velocity, 2D AABB / 3D OBB, collisionLayer/mask, friction/restitution, isTrigger |
| ComGameplay | 逻辑 | 玩家状态机(Idle/Walk/Jump/Fall), tag标签, pendingDestroy |

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

## 6. 关键文件速查

| 文件 | 职责 | 关键点 |
|------|------|--------|
| `main.cpp` | 入口、主循环 | 程序生命周期 |
| `Scene.h` | 场景抽象基类 | BuildPipeline 接口、默认管线构建 |
| `Scene2D.h/cpp` | 2D 战斗场景 | 2D 管线（2 Pass）、地图、AABB 物理 |
| `Scene3D.h/cpp` | 3D 展示场景 | 3D 管线（27 Pass）、IBL 初始化、PBR+阴影 |
| `SceneManager.h/cpp` | 场景管理器 | 场景注册与切换 |
| `Renderer.h/cpp` | 渲染协调 | UBO 管理、渲染队列、管线驱动 |
| `RenderPipeline.h/cpp` | 管线执行 | 纹理池、Pass 调度、PSO 增量切换 |
| `RenderPass.h` | Pass 抽象 | 声明式 I/O 接口 |
| `RenderTypes.h` | 渲染类型定义 | SortKey(64bit)、UBO 结构、PSO 状态 |
| `TextureSemantic.h` | 纹理语义 | 19 种语义枚举、槽位映射 |
| `EntityFactory.h/cpp` | 实体工厂 | 相机/灯光/模型/玩家/粒子/层级创建，异步加载 |
| `Systems.h/cpp` | ECS 系统 | 相机/动画/层级/灯光/渲染提交/玩家/销毁 |
| `PhysicsSystem.h/cpp` | 物理引擎 | 2D AABB + 3D OBB SAT + 射线检测 + 触发器 |
| `ParticleSystem.h/cpp` | 粒子系统 | 发射/更新/Instanced Billboard 渲染 |
| `WorkerPool.h/cpp` | 线程池 | 异步模型加载 |
| `GLCommandQueue.h/cpp` | GL 命令队列 | 延迟 GL 操作同步 |
| `ResourceManager.h/cpp` | 资源管理 | 强类型 ID、生命周期 |
| `ResourceIDs.h` | 资源 ID 定义 | TextureID/MeshID/ShaderID/FramebufferID |
| `ModelLoader.h/cpp` | 模型加载 | glTF 2.0 + OBJ 解析流程 |
| `GeometryUtils.h` | 几何工具 | Lengyel 切线、Y-up→Z-up、稀疏访问器 |
| `ComLight.h/cpp` | 灯光组件 | 视锥体拟合、阴影矩阵 |
| `AssistLight.h` | 灯光辅助 | 视锥拟合 + 纹素对齐 |
| `PipelineUtils.h` | GL 状态工具 | 捕获/应用/恢复 |
| `DrawUtils.h/cpp` | 绘制引擎 | SortKey 排序、UBO 更新、Uniform 反射绑定 |
| `Material.h` | 材质描述 | 着色器+纹理映射+自定义属性 |
