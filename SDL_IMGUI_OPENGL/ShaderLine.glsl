#shader vertex
#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 a_Color;

out vec4 v_Color;

layout(std140) uniform PerObjectData
{
    mat4 u_Model;
    mat4 u_MVP;
    vec4 u_Color;
};

void main()
{
	gl_Position = u_MVP * vec4(position,1.0);
    // v_Color = a_Color * u_Color; // 将顶点颜色与统一颜色相乘
    v_Color = a_Color;
};

#shader fragment
#version 450 core

in vec4 v_Color;

layout(location = 0) out vec4 color;

void main()
{
	color = v_Color;
};