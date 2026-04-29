# 未来架构规划

本文档记录项目从技术验证走向引擎化的思考路径，重点展示架构演进的动机和取舍。

## 当前阶段的定位

Phase 1 验证了核心渲染能力（PBR、阴影、后处理、ECS 场景管理），但架构上存在三个根问题：

- **状态不可持久化**：场景、材质、灯光参数全是硬编码，改配置需重新编译
- **模块边界模糊**：Scene 和 Renderer 双向耦合，系统函数散落各处，难以独立测试
- **流程线性僵化**：SceneManager 只支持 `SetActive(name)` 式的切换，无法表达复杂游戏流程

这三个问题分别对应后续两个阶段的改进目标。

## Phase 2：存储与接口

### 存储栈设计

```
[磁盘文件]
    │  JSON / 二进制
    ▼
[资源描述符]   path → type → dependencies → load state
    │
    ▼
[序列化器]     组件 ↔ 二进制流，支持版本号（向前兼容）
    │
    ▼
[运行时对象]   ECS registry / GPU 资源 / 配置表
```

三个子系统各司其职：

**资源数据库**（ResourceDatabase）
- 当前 ResourceManager 只管 GPU 生命周期，不知道"资源的来源"
- 新增中间层：资源路径 → 元数据 → 加载策略 → 运行时 handle
- 为后续异步加载和热重载打地基

**ECS 序列化**
- 每个组件实现 `Serialize(archive)` / `Deserialize(archive)`
- archive 记录版本号 → 组件字段变更时写迁移逻辑
- 场景从 `.scene` 文件加载，不再是 `.cpp` 里的硬编码

**配置系统**
- 管线参数（Bloom 阈值、SSAO 半径、阴影分辨率）从 config 文件读
- 分层 override：base → quality preset → user settings
- 运行时可通过 ImGui 调参 → 导出到文件

### 接口重构

核心原则：**依赖方向统一朝下，层间通过抽象通信**。

```
当前问题                          改进方向
─────────────────────────────────────────────────
ECS System 直接调 Renderer::Submit   → System 产出 RenderCommand 到队列，Renderer 消费队列
InputManager 全局单例                → 输入上下文栈（UI 层消费后 Gameplay 层不可见）
DrawUtils 混合状态设置和绘制逻辑     → 拆分为 PSO 绑定 + Draw 调用 + 资源绑定
Pass 注册硬编码在 InitPipeline       → PipelineBuilder 从描述文件构建 Pass 列表
```

改动后每一层只关心自己的职责和下游接口，不关心上层是谁。

## Phase 3：节点图架构（远景）

建立在 Phase 2 基础设施上——场景文件可加载、组件可序列化、接口边界清晰——之后，SceneManager 演进为图执行器：

- **节点** = 最小可运行单元，拥有自己的 ECS 世界、输入上下文、UI 层
- **边** = 转移条件 + 数据载荷（节点 A 的退出结果 → 节点 B 的进入参数）
- **并行** = 多个节点可同时活跃（GameWorldNode + HUDNode），渲染层合成由执行器处理

和商业引擎的对应关系：UE 的 GameMode/GameState 机制、Godot 的 SceneTree，本质都是节点图——本项目用更轻量的方式实现相同概念。

## 面试叙事

> "这个项目的技术验证阶段完成了渲染管线的完整链路。当前正在推进的是可持久化的存储系统和接口重构，让引擎从'能跑'变成'能用'。长远目标是用节点图替代线性场景切换，让游戏流程变为数据驱动的有向图。整体思路是在正确的时间引入正确的复杂度，不为了架构而架构。"

这条线展示了三个层次的能力：实现（Phase 1）、工程化（Phase 2）、系统设计（Phase 3）。
