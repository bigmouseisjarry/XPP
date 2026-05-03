#shader vertex
#version 450 core

layout(location = 0) in vec3 a_Position;   // quad 顶点（-0.5~0.5）
layout(location = 1) in vec2 a_TexCoord;   // quad UV

// 实例属性
layout(location = 2) in vec3 a_InstancePos;
layout(location = 3) in vec4 a_InstanceColor;
layout(location = 4) in float a_InstanceSize;
layout(location = 5) in float a_InstanceRotation;
layout(location = 6) in vec2 a_InstanceUVOffset;
layout(location = 7) in vec2 a_InstanceUVScale;

out vec2 v_TexCoord;
out vec4 v_Color;

layout(std140) uniform PerFrameData
{
    mat4 u_ProjView;
    vec3 u_ViewPos;
    float _pad0;
};

uniform int u_Dimension;  // 0=2D, 1=3D

void main()
{
    v_TexCoord = a_TexCoord * a_InstanceUVScale + a_InstanceUVOffset;
    v_Color = a_InstanceColor;

    // Billboard 旋转（绕 Z 轴）
    float c = cos(a_InstanceRotation);
    float s = sin(a_InstanceRotation);
    vec2 rotatedPos = vec2(
        a_Position.x * c - a_Position.y * s,
        a_Position.x * s + a_Position.y * c
    ) * a_InstanceSize;

    if (u_Dimension == 1) {
        // 3D Billboard：在 view space 中偏移，保证面向摄像机
        vec4 viewPos = u_ProjView * vec4(a_InstancePos, 1.0);
        gl_Position = viewPos + vec4(rotatedPos, 0.0, 0.0);
    } else {
        // 2D：直接世界坐标偏移
        gl_Position = u_ProjView * vec4(
            a_InstancePos + vec3(rotatedPos, 0.0), 1.0);
    }
}

#shader fragment
#version 450 core

in vec2 v_TexCoord;
in vec4 v_Color;

layout(location = 0) out vec4 FragColor;

uniform sampler2D u_AlbedoMap;

void main()
{
    vec4 texColor = texture(u_AlbedoMap, v_TexCoord);
    FragColor = texColor * v_Color;
}