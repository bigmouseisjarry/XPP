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

uniform sampler2D u_BloomBrightMap;
uniform vec2 u_Direction;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(u_BloomBrightMap, 0));

    vec3 result = texture(u_BloomBrightMap, vUV).rgb * 6.0;
    result += texture(u_BloomBrightMap, vUV - u_Direction * texelSize).rgb * 4.0;
    result += texture(u_BloomBrightMap, vUV + u_Direction * texelSize).rgb * 4.0;
    result += texture(u_BloomBrightMap, vUV - 2.0 * u_Direction * texelSize).rgb * 1.0;
    result += texture(u_BloomBrightMap, vUV + 2.0 * u_Direction * texelSize).rgb * 1.0;
    FragColor = vec4(result / 16.0, 1.0);
}