# SDL_IMGUI_OPENGL

基于 OpenGL 4.5 的实时 PBR 渲染引擎，使用 ECS 架构管理场景，支持 glTF 2.0 模型加载、IBL 环境光照、多光源阴影、SSAO、Bloom、粒子系统等。

## 效果展示

![PBR 渲染效果](docs/images/pbr_scene.png)
![阴影与 SSAO](docs/images/shadow_ssao.png)
![Bloom 后处理](docs/images/bloom_effect.png)
![2D 游戏场景](docs/images/2d_battle.png)

## 特性

### 渲染

- Cook-Torrance PBR（GGX 法线分布、Smith 几何遮蔽、Schlick 菲涅尔）
- 金属度-粗糙度工作流
- IBL 环境光照：HDR 等距柱状天空盒 → 立方体贴图 → 辐照度图 + 预过滤环境图 + BRDF LUT
- 法线映射（TBN 正交化）
- HDR 渲染（RGBA16F 中间帧缓冲）
- 多光源支持（最多 8 盏，UBO 批量传输）：方向光 / 点光 / 聚光
- 自发光 / 环境遮蔽纹理
- MRT 多渲染目标（场景颜色 + 世界法线）

### 阴影

- 多光源阴影映射（Layered Depth Array，最多 8 层）
- PCF 5×5 软阴影
- 前向剔除缓解 Peter Panning
- 相机视锥体拟合光投影矩阵 + 纹素对齐，优化阴影精度与稳定性

### 后处理

- **Bloom**：4 级渐进式降采样 → 亮度提取（BT.709） → 可分离高斯模糊 → 渐进式上采样叠加
- **SSAO**：64 采样半球核（UBO）+ 4×4 旋转噪声 + 深度重建视空间位置 + 范围检测 + 5×5 盒模糊
- **FXAA**：快速近似抗锯齿
- Reinhard 色调映射
- Gamma 校正（1/2.2）

### 粒子系统

- Instanced Billboard 渲染（2D / 3D）
- 发射器形状：点 / 盒 / 球 / 锥
- 颜色/大小随生命期渐变曲线
- 重力、阻力、自旋转
- 精灵表动画 + 随机起始帧
- 加法混合模式（火焰/光效）
- 爆发 + 持续发射模式，支持子发射器

### 场景与 ECS

- 基于 EnTT 的实体-组件-系统架构
- 2D / 3D 变换组件，带脏标记的父子层级
- 2D 正交相机 / 3D 轨道相机（环绕、推拉、平移）
- 2D 精灵表动画系统
- 玩家状态机（Idle / Walk / Jump / Fall）

### 物理

- **2D**：AABB 网格碰撞、重力、速度积分、地面检测
- **3D**：OBB SAT 碰撞检测（15 轴测试）、穿透深度计算
- 空间哈希网格（128 单元大小）
- 碰撞层 + 掩码（Default / Player / Enemy / Projectile / Trigger / Environment）
- 摩擦 / 弹性系数
- 触发器：进入 / 停留 / 离开状态追踪
- 射线检测（Slab 算法）

### 模型加载

- glTF 2.0：节点层级、TRS 变换、PBR 材质、稀疏访问器、嵌入图片、sRGB 纹理、坐标系转换（Y-up → Z-up）
- OBJ：顶点去重、材质提取、自动计算法线/切线
- Lengyel 切线计算算法

### 渲染管线

- 声明式渲染管线：每个 Pass 声明输入/输出（TextureSemantic），管线自动解析纹理依赖
- 纹理池：基于（语义, 级别）二元组索引，支持跨 Pass 纹理传递与多分辨率 Bloom
- 管线状态对象（PSO）：混合、深度、剔除、视口状态统一管理，增量切换
- SortKey 64 位复合键排序：透明 > 双面 > 着色器 > 反照率 > 网格

### 资源管理

- 强类型资源 ID（TextureID / MeshID / ShaderID / FramebufferID），编译期防混用
- 名称 → ID 双向查找，统一生命周期管理
- 异步资源加载：WorkerPool 线程池 + GLCommandQueue 延迟 GL 操作

## 渲染管线流程

```
IBL Passes（一次性初始化）
  EquirectToCubemap → Irradiance → Prefilter → BRDFLUT
                                                        │
ShadowPass ─────────────────────────────────────────────│────────┐
   │ 8层深度数组                                         │        │
   ▼                                                    ▼        │
SkyboxPass ──► OpaquePass3D ──► TransparentPass3D       │        │
   │               │                                    │        │
   │               │ PBR + IBL + 阴影采样                │        │
   │               ▼                                    │        │
   │          SceneColor (HDR)                          │         │
   │          WorldNormal                                │         │
   │                                                     │        │
   │    ┌────────────────────────────────────────────────┘        │
   │    ▼                                                          │
   │  BloomBright ──► BloomBlur ×4 ──► BloomUp ×3                  │
   │                                                               │
   ▼                                                               ▼
SSAO ──► SSAOBlur ──► FXAA ──► Composite ──────────────────────────┘
                                  │ 色调映射 + Gamma
                                  ▼
                               屏幕输出
```

## 架构概览

```
┌──────────────────────────────────────────────────────────────┐
│                       Application                            │
│    SDL3 窗口 · ImGui 调试面板 · 事件循环                       │
├──────────────┬──────────────┬────────────────────────────────┤
│   Scene      │   Renderer   │   ResourceManager              │
│  EnTT ECS    │  管线构建     │  强类型 ID · 纹理/网格/         │
│  组件/系统    │  UBO 管理     │  着色器/帧缓冲 · 异步加载       │
├──────────────┤              ├────────────────────────────────┤
│  Systems     │              │   ModelLoader                  │
│  相机/灯光/   │              │  glTF 2.0 · OBJ                │
│  物理/动画/   │              │  PBR 材质提取                   │
│  粒子/玩家    │              │                                │
├──────────────┴──────────────┴────────────────────────────────┤
│                    RenderPipeline                            │
│    Pass 有序执行 · 纹理池依赖解析 · PSO 增量状态管理            │
├──────────────────────────────────────────────────────────────┤
│  Shadow │ Skybox │ Opaque3D │ Transparent │ Particle │ Post  │
│  深度数组│ IBL    │ PBR+阴影  │ 透明排序    │ Billboard│ Bloom │
│          │        │          │            │ Instanced│ SSAO  │
│          │        │          │            │          │ FXAA  │
├──────────────────────────────────────────────────────────────┤
│                OpenGL 4.5 · GLEW                             │
└──────────────────────────────────────────────────────────────┘
```

## 项目结构

```
SDL_IMGUI_OPENGL/
├── SDL_IMGUI_OPENGL/            # 源码
│   ├── main.cpp                 # 入口：SDL3 初始化、主循环
│   ├── Renderer.h/cpp           # 渲染协调器：管线构建、UBO、提交
│   ├── RenderPipeline.h/cpp     # 管线执行引擎：Pass 调度、纹理池
│   ├── RenderPass.h             # Pass 抽象基类（声明式 I/O）
│   ├── RenderTypes.h            # 渲染类型：SortKey、UBO 结构、PSO 状态
│   ├── PostProcessPass.h        # 后处理 Pass 基类（全屏四边形）
│   ├── TextureSemantic.h        # 纹理语义枚举与槽位映射
│   ├── PipelineUtils.h          # GL 状态捕获/应用/恢复
│   ├── DrawUtils.h/cpp          # 绘制引擎：排序、UBO 更新、Uniform 反射绑定
│   ├── FullscreenQuad.h         # 全屏四边形绘制
│   │
│   ├── ResourceManager.h/cpp    # 资源注册中心（强类型 ID）
│   ├── ResourceIDs.h            # 强类型资源 ID 定义
│   ├── Shader.h/cpp             # 着色器编译 + Uniform 反射
│   ├── Texture.h/cpp            # 纹理加载（标准/HDR/默认/噪声/立方体/深度）
│   ├── Framebuffer.h/cpp        # 帧缓冲封装（MRT、深度数组、立方体贴图面）
│   ├── Material.h               # 材质描述（着色器 + 纹理映射 + 自定义属性）
│   ├── ModelLoader.h/cpp        # glTF 2.0 / OBJ 模型加载
│   ├── mesh.h/cpp               # 网格数据
│   ├── VertexArray.h/cpp        # VAO 封装
│   ├── VertexBuffer.h/cpp       # VBO 封装
│   ├── IndexBuffer.h/cpp        # IBO 封装
│   ├── UniformBuffer.h/cpp      # UBO 封装（命名绑定点）
│   ├── Vertex.h                 # 顶点格式（2D / 3D / Line / ParticleInstance）
│   ├── GeometryUtils.h          # 法线/切线计算、坐标系转换、条带展开
│   │
│   ├── WorkerPool.h/cpp         # 线程池：异步模型加载
│   ├── GLCommandQueue.h/cpp     # GL 命令队列：延迟 GL 操作同步
│   │
│   ├── ShadowPass.h/cpp         # 多光源阴影 Pass（Layered Depth Array）
│   ├── SkyboxPass.h/cpp         # HDR 天空盒 Pass
│   ├── OpaquePass3D.h/cpp       # 不透明 PBR Pass（IBL + 阴影采样）
│   ├── TransparentPass3D.h/cpp  # 透明物体 Pass
│   ├── OpaquePass2D.h/cpp       # 2D 不透明 Pass
│   ├── TransparentPass2D.h/cpp  # 2D 透明 Pass
│   ├── EquirectToCubemapPass.h/cpp  # IBL：HDR → 立方体贴图
│   ├── IrradiancePass.h/cpp     # IBL：漫反射辐照度图
│   ├── PrefilterPass.h/cpp      # IBL：镜面反射预过滤环境图（mip chain）
│   ├── BRDFLUTPass.h/cpp        # IBL：BRDF 积分查找表
│   │
│   ├── ParticleSystem.h/cpp     # 粒子系统：发射、更新、Instanced 渲染
│   ├── ComParticle.h/cpp        # 粒子发射器组件
│   │
│   ├── PhysicsSystem.h/cpp      # 物理引擎：2D AABB + 3D OBB SAT 碰撞
│   ├── OBB.h                    # 有向包围盒 + SAT 碰撞检测 + 射线检测
│   ├── Systems.h/cpp            # ECS 系统：相机/渲染提交/玩家/动画/层级/灯光/销毁
│   ├── HierarchyUtils.h         # 层级脏标记传播
│   ├── DebugRenderSystem.h      # 调试渲染（OBB 线框、灯光范围视锥）
│   │
│   ├── Scene.h                  # 场景抽象基类
│   ├── Scene2D.h/cpp            # 2D 战斗场景
│   ├── Scene3D.h/cpp            # 3D 展示场景（glTF 查看器）
│   ├── SceneManager.h/cpp       # 场景管理器
│   ├── EntityFactory.h/cpp      # 实体工厂（相机/灯光/模型/玩家/粒子）
│   │
│   ├── ComTransform.h           # 变换组件（2D/3D，脏标记层级）
│   ├── ComCamera.h              # 相机组件（正交/透视轨道）
│   ├── ComLight.h/cpp           # 灯光组件（阴影层、视锥拟合）
│   ├── ComRender.h              # 渲染组件（可见性/模型/动画精灵）
│   ├── ComPhysics.h             # 物理组件（2D AABB / 3D OBB、碰撞层/摩擦/弹性）
│   ├── ComGameplay.h            # 游戏逻辑组件（玩家状态机/标签/销毁）
│   ├── AssistLight.h            # 灯光辅助：视锥拟合 + 纹素对齐
│   │
│   ├── InputManager.h/cpp       # 输入管理
│   ├── InputStruct.h            # 输入动作枚举
│   ├── GameSettings.h/cpp       # 游戏设置
│   ├── Settings.h/cpp           # 设置数据模型
│   ├── SettingsUI.h/cpp         # ImGui 设置编辑器
│   ├── Map.h/cpp                # 2D 地图系统
│   │
│   ├── *.glsl                   # 20 个着色器文件
│   └── resources/               # 模型、纹理、HDR 天空盒资源
├── 扩展库/                       # 第三方依赖
│   ├── SDL3/                    # 窗口与输入
│   ├── GLEW/                    # OpenGL 扩展加载
│   ├── glm/                     # 数学库
│   ├── imgui/                   # Dear ImGui（SDL3 + OpenGL3 后端）
│   └── entt/                    # 实体-组件-系统框架
└── SDL_IMGUI_OPENGL.slnx        # Visual Studio 解决方案
```

## 着色器

| 着色器 | 用途 |
|--------|------|
| Shader3D.glsl | PBR 核心：Cook-Torrance BRDF、IBL 采样、法线映射、多光源、PCF 阴影、MRT 输出 |
| ShaderShadow.glsl | 阴影深度 Pass |
| ShaderSkybox.glsl | 等距柱状 HDR 天空盒 |
| ShaderEquirectToCubemap.glsl | IBL：HDR 等距柱状投影 → 立方体贴图转换 |
| ShaderIrradiance.glsl | IBL：漫反射辐照度图卷积 |
| ShaderPrefilter.glsl | IBL：镜面反射预过滤环境图（roughness mip chain） |
| ShaderBRDFLUT.glsl | IBL：BRDF 积分查找表生成 |
| ShaderParticle.glsl | 粒子 Instanced Billboard（2D/3D、精灵表、颜色/大小渐变） |
| ShaderBloomBright.glsl | Bloom 亮度提取（BT.709） |
| ShaderBloomBlur.glsl | Bloom 可分离高斯模糊 |
| ShaderBloomUp.glsl | Bloom 渐进式上采样叠加 |
| ShaderSSAO.glsl | SSAO：深度重建视空间位置、半球采样、范围检测 |
| ShaderSSAOBlur.glsl | SSAO 5×5 盒模糊 |
| ShaderFXAA.glsl | 快速近似抗锯齿 |
| ShaderComposite.glsl | 最终合成：SSAO 遮蔽 + Bloom 叠加 + Reinhard 色调映射 + Gamma 校正 |
| Shader.glsl / Shader2.glsl | 2D 精灵着色器（不透明/透明裁剪变体） |
| ShaderLine.glsl | 调试线框渲染 |
| ShaderPostProcess.glsl | 通用后处理通道 |

## 依赖

| 库 | 用途 |
|----|------|
| SDL3 | 窗口创建、输入处理、OpenGL 上下文 |
| GLEW | OpenGL 扩展加载 |
| GLM | 向量/矩阵数学 |
| Dear ImGui | 运行时调试面板 |
| EnTT | 实体-组件-系统框架 |
| TinyGLTF | glTF 2.0 模型加载 |
| tinyobjloader | OBJ 模型加载 |
| stb_image | 图片加载（PNG/JPG/HDR） |
| nlohmann/json | JSON 解析 |

## 构建

项目使用 Visual Studio 2022 构建，需确保：

1. 扩展库路径 `扩展库/` 下包含 SDL3、GLEW、GLM、ImGui、EnTT
2. SDL3 DLL 需在可执行文件同目录或系统 PATH 中
3. OpenGL 4.5 兼容显卡

```
打开 SDL_IMGUI_OPENGL.slnx → x64 Release → 生成
```

---

## 深度文档

- [项目流程架构详解](docs/PROJECT_ARCHITECTURE.md) — 从启动到一帧渲染的完整数据流
- [周开发任务清单](docs/FUTURE_ARCHITECTURE.md)

## 许可

本项目为个人学习项目，欢迎一起学习。
