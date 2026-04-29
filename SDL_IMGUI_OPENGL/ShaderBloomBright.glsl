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
uniform float u_Threshold = 1.0;

void main() {
    vec3 color = texture(u_SceneMap, vUV).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));    // 亮度公式ITU-R BT.709

    if (brightness > u_Threshold)
        FragColor = vec4(color, 1.0);
    else
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}