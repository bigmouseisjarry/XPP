#shader vertex
#version 450 core

layout(location = 0) in vec3 position;

layout(std140) uniform PerObjectData
{
    mat4 u_Model;
    mat4 u_MVP;
    vec4 u_Color;
};

void main()
{
    gl_Position = u_MVP * vec4(position, 1.0);
}

#shader fragment
#version 450 core

void main()
{
    // 深度自动写入，无需输出
}