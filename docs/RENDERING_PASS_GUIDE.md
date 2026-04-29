# 3D 渲染 Pass 深度解析

本文档面向面试中的图形学深度提问，逐 Pass 分析理论基础、实现细节和设计决策。

---

## 1. 渲染方程与 PBR 基础

### 渲染方程 (Kajiya 1986)

$$L_o(p, \omega_o) = L_e(p, \omega_o) + \int_{\Omega} f_r(p, \omega_i, \omega_o) \cdot L_i(p, \omega_i) \cdot (n \cdot \omega_i) \, d\omega_i$$

- $L_o$：出射 radiance（屏幕像素值）
- $L_e$：自发光（Emissive 纹理直接输出）
- $f_r$：BRDF（描述材质如何反射光）
- $L_i$：入射 radiance（光源 + IBL 间接光）
- $(n \cdot \omega_i)$：Lambert 余弦项

**本引擎的实现范围**：$L_i$ 目前仅包含直接光源（8盏点光/方向光），IBL 间接光为未来计划。

### Cook-Torrance BRDF

$$f_r = f_{diffuse} + f_{specular}$$

**漫反射项 (Lambert)**：

$$f_{diffuse} = \frac{\text{albedo}}{\pi} \cdot (1 - \text{metallic})$$

金属度越高，漫反射越弱（金属不产生漫反射）。

**镜面反射项 (Cook-Torrance)**：

$$f_{specular} = \frac{D \cdot G \cdot F}{4 \cdot (n \cdot \omega_o) \cdot (n \cdot \omega_i)}$$

#### 法线分布函数 D (GGX / Trowbridge-Reitz)

$$D = \frac{\alpha^2}{\pi \cdot ((n \cdot h)^2 \cdot (\alpha^2 - 1) + 1)^2}$$

其中 $\alpha = roughness^2$，$h$ 为半角向量。

**为什么选 GGX**：
- 相比 Beckmann，GGX 的高光拖尾更长（heavy-tailed），更接近真实材质
- 业界标准选择（UE4/Unity 均使用 GGX）

#### 几何遮蔽函数 G (Smith)

$$G = G_1(n \cdot \omega_i) \cdot G_1(n \cdot \omega_o)$$

$$G_1(v) = \frac{n \cdot v}{(n \cdot v)(1 - k) + k}$$

其中 $k = \frac{(roughness + 1)^2}{8}$（直接光照修正）。

**作用**：粗糙表面在掠射角时的微面元互相遮挡，减少反射能量。

#### 菲涅尔方程 F (Schlick 近似)

$$F = F_0 + (1 - F_0) \cdot (1 - (h \cdot v))^5$$

$$F_0 = \text{mix}(0.04, \text{albedo}, \text{metallic})$$

- 金属 $F_0$ 使用 albedo（金属无漫反射，菲涅尔着色来自金属颜色）
- 非金属 $F_0$ 固定为 0.04（大多数电介质的垂直反射率）

### 金属度-粗糙度工作流

```
albedo 纹理      → 基础色（非金属的漫反射色 / 金属的 F0 色）
metallic 纹理    → 金属度 (0=非金属, 1=金属)
roughness 纹理   → 粗糙度 (0=镜面, 1=漫反射)
normal 纹理      → 切线空间法线扰动
ao 纹理          → 环境遮蔽（调制 ambient 项）
emissive 纹理    → 自发光（不受光照影响）
```

---

## 2. Pass 编排原则

### 为什么是这个顺序

```
Shadow → Skybox → Opaque3D → Transparent3D → Bloom → SSAO → Composite
```

| 原则 | 说明 |
|------|------|
| **Shadow 最先** | 后续 Opaque/Transparent 需要采样 ShadowDepthArray。必须先产生深度数据 |
| **Skybox 在 Opaque 前** | 天空盒写入 SceneColor 的远平面区域，OpaquePass 通过深度测试覆盖前景。如果反过来，天空盒会覆盖不透明物体的像素 |
| **不透明在透明前** | 透明物体需要混合已有场景色（OVER 操作），且不写深度。不透明物体必须先完成以保证正确的深度缓冲 |
| **Bloom 在 SSAO 前** | Bloom 从 SceneColor HDR 值提取亮区，不受 SSAO 遮蔽影响。先算光晕再算遮蔽，逻辑独立 |
| **SSAO 在 Composite 前** | SSAO 产生遮蔽因子，Composite 将遮蔽应用到场景色 + Bloom 叠加结果上 |
| **Composite 最后** | 统一色调映射 + Gamma 校正，将 HDR 转换到 sRGB 输出 |

### 替代方案对比

**延迟渲染 (Deferred Shading)**：
- 优点：光照计算与几何复杂度解耦，适合大量动态光源
- 缺点：G-Buffer 带宽大、不支持 MSAA、透明物体需单独前向 Pass
- 本项目选前向渲染理由：最多 8 盏光源，前向 + UBO 批量传输足够；透明物体处理更自然

---

## 3. Pass 逐阶段详解

### Pass 1: ShadowPass

**目标**：为每盏光源生成深度贴图，供后续 Pass 做阴影判定。

**技术方案**：Layered Depth Array
- 使用 `gl_Layer` (geometry shader) 在一次 draw call 中渲染所有光源
- 深度数组格式：`GL_DEPTH_COMPONENT24`，`GL_TEXTURE_2D_ARRAY`，8 层
- 分辨率：2048×2048（可配置）

**GPU Pipeline**：
```
VS: Transform vertex to world space
GS: for each light layer (0..N-1):
      gl_Layer = layer
      gl_Position = lightVPs[layer] * worldPos
FS: (empty, depth-only write)
```

**PCF 5×5 软阴影**（在 OpaquePass3D 的 fragment shader 中执行）：

```glsl
// 对 5×5 核采样 ShadowDepthArray，使用 sampler2DArrayShadow
// GL_LINEAR 双线性插值由硬件完成
float shadow = texture(shadowMap, vec4(uv, layer, receiverDepth));
// shadow 值在 0.0 (全阴影) ~ 1.0 (无阴影) 之间
```

**前向偏移 (Depth Bias)** 缓解 Peter Panning：
```glsl
float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
float receiverDepth = projCoords.z - bias;
```

**动态视锥体拟合**（在 ComLight 中计算）：

```
1. 从相机 VP 矩阵逆推世界空间视锥体 8 角点
2. 将 8 角点变换到光源空间 (lightView)
3. 计算 8 个角点的 AABB (min, max)
4. 用 AABB 的 min/max 构建光源正交投影矩阵

伪代码：
corners[8] = Inverse(camera.VP) * NDC_cube_corners
lightCorners[8] = light.View * corners
aabb = BoundingBox(lightCorners)
lightProj = Ortho(aabb.min.x, aabb.max.x,
                  aabb.min.y, aabb.max.y,
                  aabb.min.z - nearMargin,
                  aabb.max.z + farMargin)
```

**面试可能追问**：
- Q: 为什么不用 CSM (Cascaded Shadow Maps)？
- A: 当前场景规模较小（展厅级别），单一 shadow map + 视锥体拟合已覆盖。CSM 适合大范围户外场景，实现复杂度高，在项目后期可作为优化方向。
- Q: PCF 5×5 的性能？
- A: 25 次纹理采样，但在 GPU 上高度并行，且 `sampler2DArrayShadow` 的硬件 PCF 部分由 GPU 纹理单元加速。实测对帧率影响 <5%。

---

### Pass 2: SkyboxPass

**目标**：渲染 HDR 天空盒背景。

**技术方案**：
- 输入：等距柱状投影 HDR 纹理 (.hdr, RGBA16F)
- 渲染为包围相机的大立方体（远平面区域）
- 深度写入 `gl_FragDepth = 1.0`（最远深度）

**为什么 HDR**：
- LDR 天空盒（如 .png）的高亮区域被截断到 [0,1]
- HDR 天空盒保留真实亮度（太阳区域可达 10+ cd/m²）
- 配合 Bloom 产生自然的大气辉光效果

**注意**：Skybox 不做色调映射，保留 HDR 值写入 SceneColor，供 Bloom 从亮区提取辉光。

---

### Pass 3: OpaquePass3D

**目标**：渲染所有不透明 3D 物体，执行完整 PBR 光照计算。

**MRT 输出**：

| Attachment | 格式 | 内容 |
|------------|------|------|
| Color 0 | RGBA16F | 场景颜色（HDR，线性空间） |
| Color 1 | RGBA16F | 世界空间法线 (SSAO 输入) |
| Depth | Depth24 | 深度缓冲 (可采样，SSAO 输入) |

**Fragment Shader 执行流程**：

```
1. 纹理采样
   albedo     = texture(albedoTex, uv).rgb
   metallic   = texture(metallicRoughnessTex, uv).b
   roughness  = texture(metallicRoughnessTex, uv).g
   ao         = texture(aoTex, uv).r  (或使用 albedo.r 代用)
   emissive   = texture(emissiveTex, uv).rgb

2. 法线映射
   切线空间法线 = texture(normalTex, uv).rgb * 2.0 - 1.0
   TBN = mat3(T, B, N)  // TBN 需正交化
   世界法线 = normalize(TBN * 切线空间法线)

3. 视角方向计算
   V = normalize(cameraPos - worldPos)
   NdotV = max(dot(N, V), 0.0)

4. 逐光源累加
   for each light (0..7):
     L = normalize(lightPos - worldPos)  // 或 -lightDir
     H = normalize(L + V)
     NdotL = max(dot(N, L), 0.0)

     // BRDF
     D = GGX(NdotH, roughness)
     G = Smith(NdotV, NdotL, roughness)
     F = Schlick(F0, HdotV)
     specular = D * G * F / (4 * NdotV * NdotL + 0.0001)
     diffuse = albedo / PI * (1 - metallic)

     // 阴影
     shadow = PCF_5x5(shadowMap, lightLayer, ...)

     Lo += (diffuse + specular) * lightColor * lightIntensity * NdotL * shadow

5. 最终输出
   ambient = albedo * ao * 0.03  // 环境光近似
   Color0 = ambient + Lo + emissive  // HDR 值
   Color1 = worldNormal  // 世界法线 (SSAO 用)
```

**TBN 正交化**：
- 切线空间的 T/B/N 在顶点阶段传入，经过插值后可能失去正交性
- Fragment shader 中重新正交化：`T = normalize(T - N * dot(N, T))`
- 确保法线映射结果的物理正确性

**Uniform 绑定**：

| Binding | 内容 | 更新频率 |
|---------|------|---------|
| UBO 0 | PerFrame（VP矩阵、相机位置、时间） | 每帧 |
| UBO 1 | Lights（8盏灯光颜色/位置/类型） | 每帧 |
| UBO 2 | ShadowMatrices（8个光VP矩阵） | 每帧 |
| tex 0-4 | Albedo, Normal, MetallicRoughness, AO, Emissive | 每材质 |
| tex 5 | ShadowDepthArray | 每帧 |

---

### Pass 4: TransparentPass3D

与 OpaquePass3D 相似，但：
- 开启混合：`GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA`
- 关闭深度写入（透明物体不应阻挡后面的物体）
- 先按距离排序（Painter's Algorithm）

当前透明物体数量少，采用前向渲染直接处理，没有单独光照 Pass。

---

### Pass 5-13: Bloom 后处理链

**理论基础**：Bloom 模拟人眼和摄像机的成像缺陷——强光源在视网膜/传感器上产生溢出的辉光。

**管线架构**：

```
SceneColor (HDR, 全分辨率)
    │
    ▼
BloomBright ────────────────────────────────────────────┐
  dot(rgb, (0.2126, 0.7152, 0.0722)) > threshold         │
  → BloomBright_L0 (全分辨率，仅亮区)                      │
    │                                                     │
    ▼                                                     │
BloomBlur_L0_H ──► BloomBlur_L0_V ──►                           │
  ↓½ 降采样         BloomBlur_L0 (全分辨率，模糊亮区)     │
BloomBlur_L1_H ──► BloomBlur_L1_V ──►                           │
  ↓½ 降采样         BloomBlur_L1 (½分辨率)               │
BloomBlur_L2_H ──► BloomBlur_L2_V ──►                           │
  ↓½ 降采样         BloomBlur_L2 (¼分辨率)               │
BloomBlur_L3_H ──► BloomBlur_L3_V                            │
                     BloomBlur_L3 (⅛分辨率)               │
    │                                                     │
    ▼                                                     │
BloomUp_L3: BloomBlur_L3 → BloomResult_L3 (⅛)              │
    │ 上采样×2 + 叠加                                     │
    ▼                                                     │
BloomUp_L2: BloomBlur_L2 + BloomResult_L3 → BloomResult_L2 (¼)│
    │ 上采样×2 + 叠加                                     │
    ▼                                                     │
BloomUp_L1: BloomBlur_L1 + BloomResult_L2 → BloomResult_L1 (½)│
    │ 上采样×2 + 叠加                                     │
    ▼                                                     │
BloomUp_L0: BloomBlur_L0 + BloomResult_L1 → BloomResult_L0 (全)│
    │                                                     │
    ▼                                                     │
  BloomResult (最终辉光纹理) ◄────────────────────────────┘
```

**可分离高斯模糊原理**：

2D 高斯核卷积：
$$G(x,y) = \frac{1}{2\pi\sigma^2} e^{-\frac{x^2+y^2}{2\sigma^2}}$$

朴素实现需要 $N \times N$ 次采样。利用可分离性：
$$G(x,y) = G(x) \cdot G(y)$$

水平 Pass：$N$ 次采样 → 垂直 Pass：$N$ 次采样 → 总计 $2N$ 次。

对于 9×9 核：81 次 → 18 次，效率提升 4.5×。

**多级降采样的设计理由**：
- 单级全分辨率模糊的核半径有限（覆盖范围小）
- 降采样后，同样核半径覆盖的像素范围翻倍（产生大范围辉光）
- 4 级降采样 = 1/16 分辨率，9×9 核等效覆盖原分辨率 144×144 区域
- 代价：每级降采样像素数减半为 1/4，总开销约为全分辨率单级模糊的 1.3×

**纹理池二元组索引的关键作用**：

Bloom 管线中同时存在 `BloomBlur_L0`、`BloomBlur_L1`、`BloomBlur_L2`、`BloomBlur_L3` 四个不同分辨率的纹理，语义相同都是 "BloomBlur"。如果纹理池只用语义作为 key，后注册的会覆盖先注册的。

`PoolKey = (TextureSemantic, Level)` 方案：
```cpp
PoolKey(TextureSemantic::BloomBlur, 0)  → 1024×768 纹理
PoolKey(TextureSemantic::BloomBlur, 1)  → 512×384 纹理
PoolKey(TextureSemantic::BloomBlur, 2)  → 256×192 纹理
PoolKey(TextureSemantic::BloomBlur, 3)  → 128×96 纹理
```

---

### Pass 14-15: SSAO 后处理链

**理论基础**：环境光遮蔽（Ambient Occlusion）描述几何体对漫反射环境光的遮挡。SSAO 在屏幕空间近似计算，不需要预计算或场景简化。

**核心算法** (Crysis 2007, 改进版)：

```
1. 视空间位置重建
   从深度缓冲采样深度值
   NDC坐标 = (screenUV * 2.0 - 1.0, depth * 2.0 - 1.0)
   视空间位置 = Inverse(Projection) * NDC坐标

2. 视空间法线计算
   dFdx(worldPos) → 屏幕空间切向量
   dFdy(worldPos) → 屏幕空间副切向量
   法线 = normalize(cross(ddx, ddy))

3. 半球采样
   64个采样方向（切线空间随机半球）
   for each sample:
     视空间采样点 = P + sampleDir * radius
     投影采样点 = Projection * 视空间采样点  → NDC → UV + 深度
     检查采样点深度 vs 实际几何深度
     如果采样点在前方 → 贡献遮蔽
```

**切线空间半球采样**：

```glsl
vec3 sampleDir = normalize(TBN * kernel[i]);  // 切线→视空间
// kernel 在 CPU 端生成，半球随机方向 + 衰减权重
// 近中心的采样权重更大（近距离几何贡献更多遮蔽）
```

**4×4 旋转噪声**：

- 采样方向的随机旋转通过预生成的 4×4 噪声纹理实现
- 每个像素的采样方向旋转 `noiseTex.rg * 2.0 - 1.0`
- 打破 64 个固定方向产生的带状伪影
- 4×4 的周期性被随后的 5×5 盒模糊平滑掉

**范围检测**：

```glsl
float rangeCheck = smoothstep(0.0, 1.0, radius / abs(P.z - sampleDepth));
// 如果采样点深度与几何深度差距大于 radius，不贡献遮蔽
// 避免远处不相关几何体误遮挡近处物体
```

**为什么 SSAO 用 UBO 传采样核而非 uniform 数组**：

```
问题：64 个 vec3 = 192 个 float
      GL_MAX_FRAGMENT_UNIFORM_COMPONENTS 最低保证 256 (即 64 个 vec4)
      192 个 float 理论上不超，但在某些驱动上 vec3 会占用 vec4 槽位
      实际 192 → 256 个槽位，刚好到极限
      后续加其他 uniform 直接超限，编译失败

方案：UBO (binding=3)
      UBO 的最小保证大小 = 16KB (GL_MAX_UNIFORM_BLOCK_SIZE)
      64 × vec3 = 768 bytes + padding → ~1KB
      远低于限制，且与 shadow UBO 统一在 Renderer 中管理
```

**5×5 盒模糊**：

对 SSAO 结果做 5×5 平均，消除采样噪声。盒模糊的权重均匀（所有权重 = 1/25），GPU 实现简单高效。

---

### Pass 16: Composite

**目标**：将所有中间结果合成为最终屏幕图像。

**输入**：
- SceneColor (RGBA16F, HDR) — 3D 场景渲染结果
- BloomResult (RGBA16F, HDR) — Bloom 辉光纹理
- SSAOBlurResult (R8) — SSAO 遮蔽因子

**处理流程**：

```glsl
// 1. 应用 SSAO
vec3 color = sceneColor * ssaoFactor;  // 遮蔽调制

// 2. 叠加 Bloom
color += bloomResult;  // 辉光叠加

// 3. Reinhard 色调映射
// 将 HDR [0, ∞) 映射到 LDR [0, 1]
// Reinhard: f(x) = x / (x + 1)
// 特性：保留暗部细节，高亮区自然压缩，无硬截断
color = color / (color + 1.0);

// 4. Gamma 校正
// 线性空间 → sRGB 空间 (显示器期望)
// 使用近似值 1/2.2 ≈ 0.4545
color = pow(color, vec3(0.4545));
```

**为什么在最终阶段统一做色调映射**：
- 之前版本在每个 shader 独立做 tone mapping + gamma
- 问题：传入 BloomBright 的颜色已被 clip 到 [0,1]，亮度提取失效
- 解决方案：所有中间 Pass 保持 HDR，只在 Composite 最后一步做色调映射
- 关键洞察：Gamma 校正不是线性的，必须放在色调映射之后

**色调映射对比**：

| 方法 | 公式 | 特性 |
|------|------|------|
| Reinhard | $x/(x+1)$ | 简单、保留暗部、亮部自然压缩 |
| ACES (大致) | 多项式拟合 | 电影感调色、亮部偏蓝 |
| Uncharted 2 | 多项式拟合 | 风格化、高对比度 |
| **本项目采用** | Reinhard | 简单标准，适合展示 PBR 材质本色 |

---

## 4. 性能分析

### 各 Pass GPU 时间估算 (以 1920×1080 为准)

| Pass | 主要开销 | 预计时间 |
|------|---------|---------|
| ShadowPass | N×8层深度写入，2048² | ~2-3ms |
| SkyboxPass | 全屏立方体 + HDR 采样 | ~0.2ms |
| OpaquePass3D | PBR BRDF + 逐光源循环 + PCF 25采样 | ~3-5ms |
| TransparentPass3D | 同上 + 混合 | ~0.5ms |
| Bloom (13 Pass) | 多数低分辨率操作 | ~1-2ms |
| SSAO + Blur | 64 采样/像素 + 25 采样/像素 | ~2-3ms |
| Composite | 简单叠加 + pow | ~0.1ms |
| **总计** | | **~10-15ms (66-100 FPS)** |

### 当前瓶颈

1. **OpaquePass3D 的逐光源循环**：每个 fragment 遍历 8 盏灯，即使灯光不在范围内。优化方向：Tile-based light culling（分块剔除）、clustered forward rendering（3D 网格剔除）。
2. **ShadowPass**：每个模型渲染 8 次（geometry shader 展开）。优化方向：CSM（减少光源数但增加级联）、或限制只有关键光源投射阴影。
3. **SSAO 每像素 64 次采样**：可降为 32 或 16 配合时间累积（TAA）。

### 与商业引擎的性能对比思路

| 维度 | 本项目 | UE5/Unity |
|------|--------|-----------|
| 渲染方式 | Forward+ | Deferred/Forward+ |
| 阴影 | 单一 Shadow Array + PCF | CSM + Ray Traced Shadow |
| AO | SSAO (屏幕空间) | SSAO + GTAO + RTAO |
| 全局光照 | 无 | Lumen/Baked Lightmap |
| Draw Call 优化 | 手动排序 | GPU Driven + Visibility Buffer |

---

## 5. 着色器文件速查

| 着色器 | 文件名 | 核心功能 |
|--------|--------|---------|
| PBR 核心 | Shader3D.glsl | Cook-Torrance BRDF、法线映射、多光源、PCF 阴影、MRT 输出 |
| 阴影深度 | ShaderShadow.glsl | GS 展开 gl_Layer、FS 空写深度 |
| 天空盒 | ShaderSkybox.glsl | 等距柱状 HDR 采样、gl_FragDepth=1.0 |
| Bloom 亮度提取 | ShaderBloomBright.glsl | 亮度加权 + 阈值截断 |
| Bloom 模糊 | ShaderBloomBlur.glsl | 可分离高斯、权重预计算 |
| Bloom 上采样 | ShaderBloomUp.glsl | 双线性上采样 + 叠加混合 |
| SSAO | ShaderSSAO.glsl | 深度重建、半球采样、范围检测 |
| SSAO 模糊 | ShaderSSAOBlur.glsl | 5×5 盒模糊 |
| 最终合成 | ShaderComposite.glsl | SSAO+Bloom 叠加 + Reinhard + Gamma |
| 2D 精灵 | Shader.glsl / Shader2.glsl / Shader3.glsl | 精灵纹理、透明度裁剪 |
| 线框 | ShaderLine.glsl | 调试 OBB 线框 |
| 通用后处理 | ShaderPostProcess.glsl | 纹理拷贝、格式转换 |
