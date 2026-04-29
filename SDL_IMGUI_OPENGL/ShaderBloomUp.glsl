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

uniform sampler2D u_BloomUpMap;
uniform sampler2D u_BloomBrightMap;

void main() {
    vec3 highRes = texture(u_BloomUpMap, vUV).rgb;
    vec3 lowRes = texture(u_BloomBrightMap, vUV).rgb;
    FragColor = vec4(highRes * 0.7 + lowRes * 0.3, 1.0);
}