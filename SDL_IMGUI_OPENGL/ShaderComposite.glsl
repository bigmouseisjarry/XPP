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
uniform sampler2D u_BloomMap;
uniform sampler2D u_SSAOMap;
uniform float u_BloomIntensity = 0.15;
uniform float u_SSAOIntensity = 0.6;

void main() {
    vec3 scene = texture(u_SceneMap, vUV).rgb;
    vec3 bloom = texture(u_BloomMap, vUV).rgb;
    float ssao = texture(u_SSAOMap, vUV).r;

    // SSAO 遮蔽：ssao=1 不遮挡，ssao=0 完全遮挡
    scene *= mix(vec3(1.0), vec3(ssao), u_SSAOIntensity);

    // Bloom 叠加
    scene += bloom * u_BloomIntensity;

    // Reinhard 色调映射
    scene = scene / (scene + vec3(1.0));
    // Gamma 校正
    scene = pow(scene, vec3(1.0 / 2.2));

    FragColor = vec4(scene, 1.0);
}