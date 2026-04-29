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

uniform sampler2D u_SSAOMap;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(u_SSAOMap, 0));
    float result = 0.0;
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(u_SSAOMap, vUV + offset).r;
        }
    }
    result /= 25.0;
    FragColor = vec4(result, result, result, 1.0);
}