#shader vertex
#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

out vec2 vUV;

void main() {
    gl_Position = vec4(aPos, 1.0);
    vUV = aUV;
}

#shader fragment
#version 450 core

in vec2 vUV;

out vec4 FragColor;

uniform sampler2D u_SceneMap;

void main() {
    vec3 color = texture(u_SceneMap, vUV).rgb;
    color = color / (color + vec3(1.0));    // Reinhard 色调映射
    color = pow(color, vec3(1.0 / 2.2));    // Gamma 校正
    FragColor = vec4(color, 1.0);
}