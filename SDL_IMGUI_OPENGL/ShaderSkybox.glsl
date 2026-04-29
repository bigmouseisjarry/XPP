#shader vertex
#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

out vec3 v_RayDir;

uniform mat4 u_InvProjView;
uniform vec3 u_ViewPos;

void main()
{
    // z = w = 1.0，NDC depth = 1.0（最远平面）
    gl_Position = vec4(aPos.xy, 1.0, 1.0);

    // 从裁剪空间重建远平面世界坐标
    vec4 farPoint = u_InvProjView * vec4(aPos.xy, 1.0, 1.0);
    vec3 worldFar = farPoint.xyz / farPoint.w;

    // 视线方向 = 远平面点 - 相机位置（消除平移，只保留旋转）
    v_RayDir = worldFar - u_ViewPos;
}

#shader fragment
#version 450 core

in vec3 v_RayDir;

out vec4 FragColor;

uniform sampler2D u_SkyboxMap;

const float PI = 3.14159265359;

vec2 DirectionToEquirectangular(vec3 dir)
{
    dir = normalize(dir);
    // 水平角（方位角）：atan(z,x) → [-π, π]，映射到 [0, 1]
    float u = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
    // 垂直角（仰角）：asin(y) → [-π/2, π/2]，映射到 [0, 1]
    float v = asin(dir.y) / PI + 0.5;
    return vec2(u, v);
}

void main()
{
    // Z-up → Y-up：equirectangular 纹理是 Y-up 约定
    vec3 yUpDir = vec3(v_RayDir.x, v_RayDir.z, -v_RayDir.y);

    vec2 uv = DirectionToEquirectangular(yUpDir);
    vec3 color = texture(u_SkyboxMap, uv).rgb;

    color = clamp(color, 0.0, 500.0);

    FragColor = vec4(color, 1.0);
}